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

#pragma once

#include <QtCore/QScopedPointer>
#include <QtCore/QThread>
#include <QQuaternion>
#include <trikKernel/timeVal.h>

#include "vectorSensorInterface.h"
#include "deviceState.h"

namespace trikKernel {
class Configurer;
}

namespace trikHal {
class HardwareAbstractionInterface;
}

namespace trikControl {

class VectorSensorWorker;

/// Sensor that returns a vector.
class VectorSensor : public VectorSensorInterface
{
	Q_OBJECT

public:
	/// Constructor.
	/// @param port - port on which this sensor is configured.
	/// @param configurer - configurer object containing preparsed XML files with sensor parameters.
	VectorSensor(const QString &deviceName, const trikKernel::Configurer &configurer
			, const trikHal::HardwareAbstractionInterface &hardwareAbstraction);

	~VectorSensor() override;

	Status status() const override;

public slots:
	QVector<int> read() const override;

	void calibrate(int msec) override;

	bool isCalibrated() const override;

private slots:

	void countAngle(QVector<int> gyro, trikKernel::TimeVal t);

	double getPitch(const QQuaternion &q) const;
	double getRoll(const QQuaternion &q) const;
	double getYaw(const QQuaternion &q) const;

	void init();

private:
	/// Device state, shared with worker.
	DeviceState mState;

	QScopedPointer<VectorSensorWorker> mVectorSensorWorker;
	QThread mWorkerThread;

	QQuaternion mQ;
	trikKernel::TimeVal mLastUpdate;

	const double mRadToDeg = 180.0 / M_PI;
	const double mGyroRadToDeg = 0.07 / mRadToDeg;
};

}
