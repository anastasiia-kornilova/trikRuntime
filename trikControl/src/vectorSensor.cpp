/* Copyright 2013 - 2015 Matvey Bryksin, Yurii Litvinov and CyberTech Labs Ltd.
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

#include "vectorSensor.h"

#include <trikKernel/configurer.h>
#include <trikKernel/timeVal.h>
#include <QsLog.h>

#include "vectorSensorWorker.h"

using namespace trikControl;

VectorSensor::VectorSensor(const QString &deviceName, const trikKernel::Configurer &configurer
		, const trikHal::HardwareAbstractionInterface &hardwareAbstraction)
	: mState(deviceName)
{
	mVectorSensorWorker.reset(new VectorSensorWorker(configurer.attributeByDevice(deviceName, "deviceFile"), mState
			, hardwareAbstraction, mWorkerThread));

	if (!mState.isFailed()) {
		qRegisterMetaType<trikKernel::TimeVal>("trikKernel::TimeVal");
		connect(mVectorSensorWorker.data(), SIGNAL(newData(QVector<int>,trikKernel::TimeVal))
				, this, SLOT(countAngle(QVector<int>, trikKernel::TimeVal)));

		connect(mVectorSensorWorker.data(), SIGNAL(inited())
				, this, SLOT(init()));

		QLOG_INFO() << "Starting VectorSensor worker thread" << &mWorkerThread;

		mState.ready();
	}
}

VectorSensor::~VectorSensor()
{
	if (mWorkerThread.isRunning()) {
		QMetaObject::invokeMethod(mVectorSensorWorker.data(), "deinitialize");
		mWorkerThread.quit();
		mWorkerThread.wait();
	}
}

VectorSensor::Status VectorSensor::status() const
{
	return mState.status();
}

QVector<int> VectorSensor::read() const
{
	QVector<int> vec= mVectorSensorWorker->read();
	vec[6] = getPitch(mQ) * 100;
	vec[7] = getRoll(mQ) * 100;
	vec[8] = getYaw(mQ) * 100;

	return vec;
}

void VectorSensor::calibrate(int msec)
{
	mVectorSensorWorker->calibrate(msec);
}

bool VectorSensor::isCalibrated() const
{
	return mVectorSensorWorker->isCalibrated();
}

void VectorSensor::countAngle(QVector<int> gyro, trikKernel::TimeVal t)
{
	static bool timeInited = false;
	if (!timeInited) {
		timeInited = true;
		mLastUpdate = t;
	} else {

		const double dt = (t - mLastUpdate).toMcSec() / 10000000.0;

		double x = (gyro[0] - gyro[3]) * mGyroRadToDeg * dt;
		double y = (gyro[1] - gyro[4]) * mGyroRadToDeg * dt;
		double z = (gyro[2] - gyro[5]) * mGyroRadToDeg * dt;

		const double c1 = cos(x / 2);
		const double s1 = sin(x / 2);
		const double c2 = cos(y / 2);
		const double s2 = sin(y / 2);
		const double c3 = cos(z / 2);
		const double s3 = sin(z / 2);

		QQuaternion deltaQ;
		deltaQ.setScalar(c1 * c2 * c3 + s1 * s2 * s3);
		deltaQ.setX(s1 * c2 * c3 - c1 * s2 * s3);
		deltaQ.setY(c1 * s2 * c3 + s1 * c2 * s3);
		deltaQ.setZ(c1 * c2 * s3 - s1 * s2 * c3);

		mQ *= deltaQ;
		mQ.normalize();

		mLastUpdate = t;
	}
}

double VectorSensor::getPitch(const QQuaternion &q) const
{
	return atan2(2 * q.y()*q.z() + 2 * q.scalar() * q.x()
				 , 1 - 2 * q.x() * q.x() - 2 * q.y() * q.y())
			* mRadToDeg;
}

double VectorSensor::getRoll(const QQuaternion &q) const
{
	return asin(2 * q.scalar() * q.y() - 2 * q.x() * q.y()) * mRadToDeg;
}

double VectorSensor::getYaw(const QQuaternion &q) const
{
	return atan2(2 * q.x() * q.y() + 2 * q.scalar() * q.z()
				 , 1 - 2 * q.y() * q.y() - 2 * q.z() * q.z())
			* mRadToDeg;
}

void VectorSensor::init()
{
	mQ = QQuaternion(1, 0, 0, 0);
}
