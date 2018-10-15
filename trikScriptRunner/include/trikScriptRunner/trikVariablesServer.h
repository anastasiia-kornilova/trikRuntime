#pragma once

#include <QtCore/QObject>
#include <QtNetwork/QTcpServer>

namespace trikScriptRunner {

/// Class for script variables observing, based on HTTP server
class TrikVariablesServer : public QObject
{
	Q_OBJECT
public:
	/// Constructor
	TrikVariablesServer();

signals:
	/// Emitted when there is a new connection with a HTTP-request of variables values
	/// @param propertyName - name of variables prefix, i.e prefix "web" for variable "web.light"
	void getVariables(const QString &propertyName);

public slots:
	/// Sends HTTP response with JSON data about variables
	/// @param json - JSON container for variables values
	void sendHTTPResponse(const QJsonObject &json);

private slots:
	/// Appends new connection for handling it
	void onNewConnection();

	/// Process incoming HTTP request which was sent through current connection
	void processHTTPRequest();

private:
	QScopedPointer<QTcpServer> mTcpServer;
	QTcpSocket *mCurrentConnection; // deleted with QTcpServer object

	constexpr static int port = 10000;
};

}
