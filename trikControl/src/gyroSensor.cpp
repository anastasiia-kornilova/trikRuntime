/* Copyright 2016 Anastasiya Kornilova.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License. */

#include "gyroSensor.h"

#include <trikKernel/configurer.h>
#include <trikKernel/timeVal.h>
#include <QsLog.h>
#include <cmath>

#include "vectorSensorWorker.h"

using namespace trikControl;

static constexpr float GYRO_250DPS = 8.75;
static constexpr double pi() { return 3.14159265358979323846;}
static constexpr float RAD_TO_MDEG = 1000 * 180 / pi();

GyroSensor::GyroSensor(const QString &deviceName, const trikKernel::Configurer &configurer
		, const trikHal::HardwareAbstractionInterface &hardwareAbstraction, VectorSensorInterface *accelerometer)
	: mState(deviceName)
	, mIsCalibrated(false)
	, mQ(QQuaternion(1, 0, 0, 0))
	, mGyroCounter(0)
	, mLastUpdate(trikKernel::TimeVal(0, 0))
	, mAccelerometer(accelerometer)
{
	mVectorSensorWorker.reset(new VectorSensorWorker(configurer.attributeByDevice(deviceName, "deviceFile"), mState
			, hardwareAbstraction, mWorkerThread));

	mBias.resize(3);
	mGyroSum.resize(3);
	mResult.resize(7);
	mRawData.resize(4);

	mAccelerometerVector.resize(3);
	mAccelerometerSum.resize(3);
	mAccelerometerCounter = 0;

	qDebug() << "constr";

	mCalibrationTimer.moveToThread(&mWorkerThread);
	mCalibrationTimer.setSingleShot(true);

	if (!mState.isFailed()) {
		qRegisterMetaType<trikKernel::TimeVal>("trikKernel::TimeVal");

		connect(mVectorSensorWorker.data(), SIGNAL(newData(QVector<int>,trikKernel::TimeVal))
				, this, SLOT(countTilt(QVector<int>,trikKernel::TimeVal)));

		connect(&mCalibrationTimer, SIGNAL(timeout()), this, SLOT(initParams()));

		QLOG_INFO() << "Starting VectorSensor worker thread" << &mWorkerThread;

		mState.ready();
	}
}

GyroSensor::~GyroSensor()
{
	if (mWorkerThread.isRunning()) {
		QMetaObject::invokeMethod(mVectorSensorWorker.data(), "deinitialize");
		mWorkerThread.quit();
		mWorkerThread.wait();
	}
}

GyroSensor::Status GyroSensor::status() const
{
	return mState.status();
}

const QQuaternion &GyroSensor::Q() const
{
	return mQ;
}

QVector<int> GyroSensor::read() const
{
	return mResult;
}

QVector<int> GyroSensor::readRawData() const
{
	return mRawData;
}

void GyroSensor::calibrate(int msec)
{
	qDebug() << "calib";
	mCalibrationTimer.start(msec);

	connect(mVectorSensorWorker.data(), SIGNAL(newData(QVector<int>,trikKernel::TimeVal))
			, this, SLOT(sumBias(QVector<int>,trikKernel::TimeVal)));

	connect(mAccelerometer, SIGNAL(newData(QVector<int>,trikKernel::TimeVal))
			, this, SLOT(sumAccelerometer(QVector<int>,trikKernel::TimeVal)));

	mIsCalibrated = false;
}

bool GyroSensor::isCalibrated() const
{
	return mIsCalibrated;
}

void GyroSensor::countTilt(QVector<int> gyroData, trikKernel::TimeVal t)
{
	mRawData[0] = gyroData[1];
	mRawData[1] = -gyroData[0];
	mRawData[2] = gyroData[2];
	mRawData[3] = t.packedUInt32();

	static bool timeInited = false;
	if (!timeInited) {
		timeInited = true;
		mLastUpdate = t;
	} else {

		const auto r0 = (mRawData[0] - mBias[0]) * GYRO_250DPS;
		const auto r1 = (mRawData[1] - mBias[1]) * GYRO_250DPS;
		const auto r2 = (mRawData[2] - mBias[2]) * GYRO_250DPS;

		mResult[0] = r0;
		mResult[1] = r1;
		mResult[2] = r2;
		mResult[3] = t.packedUInt32();

		constexpr auto deltaConst = pi() / 180 / 1000 / 1000000;
		const auto dt = (t - mLastUpdate) * deltaConst;

		const auto x = r0 * dt;
		const auto y = r1 * dt;
		const auto z = r2 * dt;

		const auto c1 = std::cos(x / 2);
		const auto s1 = std::sin(x / 2);
		const auto c2 = std::cos(y / 2);
		const auto s2 = std::sin(y / 2);
		const auto c3 = std::cos(z / 2);
		const auto s3 = std::sin(z / 2);

		QQuaternion deltaQ;
		deltaQ.setScalar(c1 * c2 * c3 + s1 * s2 * s3);
		deltaQ.setX(s1 * c2 * c3 - c1 * s2 * s3);
		deltaQ.setY(c1 * s2 * c3 + s1 * c2 * s3);
		deltaQ.setZ(c1 * c2 * s3 - s1 * s2 * c3);

		mQ *= deltaQ;
		mQ.normalize();

		mLastUpdate = t;

		QQuaternion loc = mDeltaQ * mQ;
		mResult[4] = getPitch<float>(loc) * RAD_TO_MDEG;
		mResult[5] = getRoll<float>(loc) * RAD_TO_MDEG;
		mResult[6] = getYaw<float>(loc) * RAD_TO_MDEG;

		emit newData(mResult, t);
	}
}

void GyroSensor::sumBias(QVector<int> gyroData, trikKernel::TimeVal)
{
//	qDebug() << "b";
	mGyroSum[0] += gyroData[1];
	mGyroSum[1] -= gyroData[0];
	mGyroSum[2] += gyroData[2];
	mGyroCounter++;
}

void GyroSensor::initParams()
{
	disconnect(mVectorSensorWorker.data(), SIGNAL(newData(QVector<int>,trikKernel::TimeVal))
			, this, SLOT(sumBias(QVector<int>,trikKernel::TimeVal)));

	disconnect(mAccelerometer, SIGNAL(newData(QVector<int>,trikKernel::TimeVal))
			, this, SLOT(sumAccelerometer(QVector<int>,trikKernel::TimeVal)));

	if (mGyroCounter != 0) {
		for (int i = 0; i < 3; i++) {
			mBias[i] = mGyroSum[i] / mGyroCounter;
			mGyroSum[i] = 0;
		}
	}

	mGyroCounter = 0;
	mIsCalibrated = true;
	mQ = QQuaternion(1, 0, 0, 0);

	if (mAccelerometerCounter != 0) {
		for (int i = 0; i < 3; i++) {
			mAccelerometerVector[i] = mAccelerometerSum[i] / mAccelerometerCounter;
			qDebug() << mAccelerometerVector[i];
			mAccelerometerSum[i] = 0;
		}
	}
	mAccelerometerCounter = 0;

	QVector3D acc(mAccelerometerVector[0], mAccelerometerVector[1], -mAccelerometerVector[2]);
	acc.normalize();

	QVector3D gravity(0, 0, -1);
	QVector3D half = acc + gravity;
	half.normalize();

	auto dot = QVector3D::dotProduct(acc, half);
	QVector3D cross = QVector3D::crossProduct(half, acc);

	mDeltaQ = QQuaternion::fromAxisAndAngle(cross, acos(dot));
}

void GyroSensor::sumAccelerometer(const QVector<int> &accelerometerData, const trikKernel::TimeVal &)
{
//	qDebug() << "a";
	mAccelerometerSum[0] += accelerometerData[0];
	mAccelerometerSum[1] += accelerometerData[1];
	mAccelerometerSum[2] += accelerometerData[2];
	mAccelerometerCounter++;
}
