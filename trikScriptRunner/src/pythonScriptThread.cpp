#include "pythonScriptThread.h"

#include <QsLog.h>

#include <trikKernel/paths.h>

using namespace trikScriptRunner;

PythonScriptThread::PythonScriptThread(trikControl::BrickInterface &brick, const QString &script)
	: mBrick(brick)
	, mScript(script)
{}

void PythonScriptThread::run()
{
	initPythonQt();
	initTrik();

	mMainContext.evalScript(mScript);
	emit finished();
}

void PythonScriptThread::initPythonQt()
{
	PythonQt::init(PythonQt::IgnoreSiteModule);
	PythonQt_QtAll::init();
	mMainContext = PythonQt::self()->getMainModule();
}

void PythonScriptThread::initTrik()
{
	PythonQt_init_PyTrikControl(mMainContext);
	mMainContext.addObject("brick", &mBrick);
	evalSystemPy();
}

void PythonScriptThread::evalSystemPy()
{
	const QString systemPyPath = trikKernel::Paths::systemScriptsPath() + "system.py";

	if (QFile::exists(systemPyPath)) {
		mMainContext.evalFile(systemPyPath);
	} else {
		QLOG_ERROR() << "system.py not found, path:" << systemPyPath;
	}
}
