#pragma once

#include <QtCore/QObject>

#include <trikControl/brickInterface.h>

#include "PythonQt_QtAll.h"
#include "PyTrikControl0.h"

void PythonQt_init_PyTrikControl(PyObject* module);

namespace trikScriptRunner {

class PythonScriptWorker : public QObject
{
	Q_OBJECT
public:
	/// Constructor
	/// @param brick - reference to trikControl::Brick instance.
	/// @param script - script for evaluation
	PythonScriptWorker(trikControl::BrickInterface &brick, const QString &script);

public slots:
	/// Starts script evaluation.
	void evalScript();

	/// Stops script evaluation.
	void stopScript();

signals:

	/// Emitted when script evaluation finished
	void finished();

private:

	/// Init PythonQt objects.
	void initPythonQt();

	/// Adds trik object to main Python context.
	void initTrik();

	/// Evaluates "system.py" file in the current context.
	void evalSystemPy();

	trikControl::BrickInterface &mBrick;

	QString mScript;

	PythonQtObjectPtr mMainContext;
};

}
