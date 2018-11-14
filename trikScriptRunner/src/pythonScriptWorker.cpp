#include "pythonScriptWorker.h"

#include <QsLog.h>

#include <trikKernel/paths.h>

using namespace trikScriptRunner;

PythonScriptWorker::PythonScriptWorker(trikControl::BrickInterface &brick, const QString &script)
	: mBrick(brick)
	, mScript(script)
{}

void trikScriptRunner::PythonScriptWorker::evalScript()
{
	initPythonQt();
	initTrik();

	mMainContext.evalScript(mScript);
	emit finished();
}

void PythonScriptWorker::stopScript()
{
	mMainContext.evalScript("script.killScript()");
}

void PythonScriptWorker::initPythonQt()
{
	PythonQt::init(PythonQt::IgnoreSiteModule);
	PythonQt_QtAll::init();
	mMainContext = PythonQt::self()->getMainModule();
}

void PythonScriptWorker::initTrik()
{
	PythonQt_init_PyTrikControl(mMainContext);
	mMainContext.addObject("brick", &mBrick);

	evalSystemPy();
}

void PythonScriptWorker::evalSystemPy()
{
	const QString systemPyPath = trikKernel::Paths::systemScriptsPath() + "system.py";

	if (QFile::exists(systemPyPath)) {
		mMainContext.evalFile(systemPyPath);
	} else {
		QLOG_ERROR() << "system.py not found, path:" << systemPyPath;
	}
}
