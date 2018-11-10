#pragma once

#include <QThread>

#include <trikControl/brickInterface.h>

#include "PythonQt_QtAll.h"
#include "PyTrikControl0.h"

void PythonQt_init_PyTrikControl(PyObject* module);

namespace trikScriptRunner {


/// Class-thread for Python script execution.
/// It was created as a way to kill executed infinite programs.
class PythonScriptThread : public QThread
{
	Q_OBJECT
public:
	/// Constructor
	/// @param brick - reference to trikControl::Brick instance
	/// @param script - script for execution
	PythonScriptThread(trikControl::BrickInterface &brick, const QString &script);

protected:
	void run() override;

signals:
	/// Emits when script finishes execution
	void finished();

private:

	/// Inits PythonQt libraries, creates main context
	void initPythonQt();

	/// Adds Trik object to main Python context
	void initTrik();

	/// Evaluates "system.py" file in the current context
	void evalSystemPy();

	PythonQtObjectPtr mMainContext;

	trikControl::BrickInterface &mBrick;

	QString mScript;
};

}
