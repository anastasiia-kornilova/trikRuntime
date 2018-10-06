#include "trikVariablesServer.h"

#include <QTimer>
#include <QDebug>
#include <QtNetwork/QTcpSocket>
#include <QDataStream>
#include <QRegExp>
#include <QJsonDocument>

TrikVariablesServer::TrikVariablesServer() :
	mTcpServer(new QTcpServer(this))
{
	connect(mTcpServer, SIGNAL(newConnection()), this, SLOT(onNewConnection()));
	bool res = mTcpServer->listen(QHostAddress::LocalHost, 10000);
	qDebug() << res;
	qDebug() << "created server";
}

void TrikVariablesServer::onVariablesReady(const QJsonObject &json)
{
	QJsonDocument doc(json);
	QByteArray bytes = doc.toJson();

#define NL "\r\n"
	QString header = "HTTP/1.0 200 OK" NL
					 "Connection: close" NL
					 "Content-type: text/plain, charset=us-ascii" NL
					 "Content-length: " + QString::number(bytes.size()) + NL
					 NL;

	mCurrentConnection->write(header.toLatin1());
	mCurrentConnection->write(bytes);
#undef NL

	mCurrentConnection->close();
}

void TrikVariablesServer::onNewConnection()
{
	mCurrentConnection = mTcpServer->nextPendingConnection();

	connect(mCurrentConnection, SIGNAL(readyRead()), this, SLOT(readData()));
	qDebug() << "new connection";
}

void TrikVariablesServer::readData()
{
	qDebug() << "new data";
	QStringList list;
	while (mCurrentConnection->canReadLine())
	{
		QString data = QString(mCurrentConnection->readLine());
		list.append(data);
	}

	QString resultString = list.join("").remove(QRegExp("[\\n\\t\\r]"));
	QStringList words = resultString.split(QRegExp("\\s+"), QString::SkipEmptyParts);

	if (words[1] == "/web/") {
		emit getVariables("web");
	}
}
