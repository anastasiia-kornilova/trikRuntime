#include "trikVariablesServer.h"

#include <QTimer>
#include <QDebug>

TrikVariablesServer::TrikVariablesServer()
{
	QTimer *mTimer = new QTimer();
	mTimer->setInterval(500);
	connect(mTimer, SIGNAL(timeout()), this, SIGNAL(getVariables()));
	mTimer->start();
}
