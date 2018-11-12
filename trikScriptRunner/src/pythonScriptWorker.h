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
	PythonScriptWorker(trikControl::BrickInterface &brick, const QString &script);

public slots:
	void evalScript();

signals:
	void finished();

private:

	void initPythonQt();

	/// Adds trik object to main Python context
	void initTrik();

	/// Evaluates "system.py" file in the current context.
	void evalSystemPy();

	trikControl::BrickInterface &mBrick;

	QString mScript;

	PythonQtObjectPtr mMainContext;
};

}
