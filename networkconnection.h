#ifndef NETWORKCONNECTION_H
#define NETWORKCONNECTION_H

#include <QObject>
#include <QElapsedTimer>
#include <QAbstractSocket>
#include "aprs.h"
class QTcpSocket;
class QTimer;
namespace APRS {
class NetworkConnection : public QObject
{
	Q_OBJECT
public:
	explicit NetworkConnection(QObject *parent = nullptr);
	void setCallsign(QString callsign, int passcode=-1);

	bool isDebugOn() const;
	void setDebug(bool enable);

private:
	int mStatus;
	QString mServer;
	quint16 mPort;
	QTcpSocket * mSocket;
	QElapsedTimer mLastMessage;
	QString mCallsign;
	int mPassCode;
	QByteArray mBuffer;
	void processLine(QByteArray);
	void sendLogon();
	bool mDebug;
	QTimer * mCheckTimer;
signals:
	void serverMessage(QString);
	void packet(APRS::Frame);
	void connectionClosed();
public slots:
	void connectToServer(QString address, quint16 port);
	void disconnect();
private slots:
	void onConnected();
	void onClosed();
	void onReadyRead();
	void onSocketError(QAbstractSocket::SocketError);
	void onCheckTimer();
};
}
#endif // NETWORKCONNECTION_H
