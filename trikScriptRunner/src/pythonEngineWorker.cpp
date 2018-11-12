/* Copyright 2018 Iakov Kirilenko, CyberTech Labs Ltd.
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

#include <QsLog.h>

#include <trikNetwork/mailboxInterface.h>
#include <trikKernel/paths.h>

#include "pythonEngineWorker.h"

using namespace trikScriptRunner;

PythonEngineWorker::PythonEngineWorker(trikControl::BrickInterface &brick
		, trikNetwork::MailboxInterface * const mailbox
		)
	: mBrick(brick)
	, mMailbox(mailbox)
	, mState(ready)
{}

void PythonEngineWorker::resetBrick()
{
	QLOG_INFO() << "Stopping robot";

	if (mMailbox) {
		mMailbox->stopWaiting();
		mMailbox->clearQueue();
	}

	mBrick.reset();
}

void PythonEngineWorker::brickBeep()
{
	mBrick.playSound(trikKernel::Paths::mediaPath() + "media/beep_soft.wav");
}

void PythonEngineWorker::stopScript()
{
	QMutexLocker locker(&mScriptStateMutex);

	if (mState == stopping) {
		// Already stopping, so we can do nothing.
		return;
	}

	if (mState == ready) {
		// Engine is ready for execution.
		return;
	}

	QLOG_INFO() << "PythonEngineWorker: stopping script";

	mState = stopping;

	if (mMailbox) {
		mMailbox->stopWaiting();
	}

	/// Very dangerous approach. Looking for a safer one.
	mScriptThread.terminate();

	mState = ready;

	QLOG_INFO() << "PythonEngineWorker: stopping complete";
}

void PythonEngineWorker::run(const QString &script)
{
	QMutexLocker locker(&mScriptStateMutex);
	mState = starting;
	emit startedScript(0);
	QMetaObject::invokeMethod(this, "doRun", Q_ARG(const QString &, script));
}

void PythonEngineWorker::doRun(const QString &script)
{
	/// When starting script execution (by any means), clear button states.
	mBrick.keys()->reset();

	qDebug() << __FUNCTION__ << __LINE__;

	PythonScriptWorker *scriptWorker = new PythonScriptWorker(mBrick, script);
	scriptWorker->moveToThread(&mScriptThread);
	connect(&mScriptThread, SIGNAL(finished()), scriptWorker, SLOT(deleteLater()));
	connect(scriptWorker, SIGNAL(finished()), this, SLOT(emitCompleted()));
	connect(this, SIGNAL(startScript()), scriptWorker, SLOT(evalScript()));
	mScriptThread.start();
	emit startScript();

	mState = running;
	QLOG_INFO() << "PythonEngineWorker: evaluation ended";
}

void PythonEngineWorker::runDirect(const QString &command)
{
	qDebug() << "PythonEngineWorker::runDirect";

	QMutexLocker locker(&mScriptStateMutex);
	QMetaObject::invokeMethod(this, "doRunDirect", Q_ARG(const QString &, command));
}

void PythonEngineWorker::doRunDirect(const QString &command)
{
	throw "Not implemented";
}

void PythonEngineWorker::emitCompleted()
{
	emit completed("", 0);
}

void PythonEngineWorker::onScriptRequestingToQuit()
{
	throw "Not implemented";
}
