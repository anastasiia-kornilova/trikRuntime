#pragma once
#include <QObject>
#include <QtNetwork/QTcpServer>

class TrikVariablesServer : public QObject
{
	Q_OBJECT
public:
	TrikVariablesServer();

signals:
	void getVariables();

private slots:

	void onNewConnection();

	void readData();

private:
	QTcpServer *mTcpServer;
	QTcpSocket *mCurrentConnection;
};
