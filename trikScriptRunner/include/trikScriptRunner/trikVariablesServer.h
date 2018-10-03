#pragma once
#include <QObject>

class TrikVariablesServer : public QObject
{
	Q_OBJECT
public:
	TrikVariablesServer();

signals:
	void getVariables();
};
