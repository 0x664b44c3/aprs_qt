#include "networkconnection.h"
#include <QTcpSocket>
#include <QCoreApplication>
#include "aprs.h"
#include <iostream>
#include <QTimer>
//at least one second delay before trying to reconnect
#define MIN_RECONNECT_TIME 1000
#define MAX_IDLE_TIME 30000
enum connectionStates
{
	connReset=-1,
	connWaitBanner=0, //wait for at least one line of "line noise" starting with #
	connLogonSent=1,
	connLogonVerified,
	connLogonFailed,
	connActive,
	connClose,
	connTerminated,
	connReconnect
};

namespace APRS {
NetworkConnection::NetworkConnection(QObject *parent) :
    QObject(parent), mSocket(new QTcpSocket(this)),
    mCallsign("NOSIGN"), mPassCode(-1), mDebug(false),
    mCheckTimer(new QTimer(this))
{
	connect(mSocket, SIGNAL(connected()), SLOT(onConnected()));
	connect(mSocket, SIGNAL(readyRead()), SLOT(onReadyRead()));
	connect(mSocket, SIGNAL(disconnected()), SLOT(onClosed()));
	connect(mSocket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)),
	        SLOT(onSocketError(QAbstractSocket::SocketError)));
	connect(mCheckTimer, SIGNAL(timeout()), SLOT(onCheckTimer()));
	mCheckTimer->start(1000);
}

void NetworkConnection::setCallsign(QString callsign, int passcode)
{
	mCallsign = callsign;
	mPassCode = passcode;
}

QRegExp rxLogResp("#\\s*logresp.*", Qt::CaseInsensitive);
QRegExp rxLogRespSplit("[\\s,]", Qt::CaseInsensitive);
void NetworkConnection::processLine(QByteArray line)
{
	mLastMessage.restart();
	if (line.startsWith('#'))
	{
		QString lineText = QString::fromLatin1(line);
		switch (mStatus)
		{
			case connWaitBanner:
				sendLogon();
				mStatus = connLogonSent;
				break;
			case connLogonSent:
				if (rxLogResp.exactMatch(lineText))
				{
					int sep = lineText.indexOf(',');
					if (sep>0)
						lineText = lineText.left(sep);
					if (mDebug)
						qDebug()<<"logon response"<<lineText;
					QStringList fields = lineText.mid(1).split(' ', QString::SkipEmptyParts);
//					qDebug()<<fields;
					if (fields.size()>2)
					{
						if (fields.at(2).toLower() == "verified")
							mStatus = connLogonVerified;
						else
							mStatus = connLogonFailed;
					}
				}
				break;
			default:
				break;
		}
		emit serverMessage(lineText);
		if (mDebug)
			std::cout << line.data() << "\n";
	}
	else
	{
		APRS::Frame frame = APRS::decodeFrame(line);
		if (frame.isValid)
			mStatus = connActive;
		emit packet(frame);
		if (mDebug)
			std::cout << line.data() << "\n";
	}
	if (mDebug)
		std::cout << std::flush;
}

void NetworkConnection::sendLogon()
{
	QString cmd("user %1 pass %2");
	cmd = cmd.arg(mCallsign).arg(mPassCode);

	QString appName = QCoreApplication::instance()->applicationName();
	QString appVer = QCoreApplication::instance()->applicationVersion();
	if (appName.size() && appVer.size()) { // add application info
		appName = appName.replace(QRegExp("\\s+"), "_");
		appVer = appVer.replace(QRegExp("\\s+"), "_");
		cmd+= " ver " + appName + " " + appVer;
	}
	cmd+="\r\n";
	mSocket->write(cmd.toLatin1());
}

bool NetworkConnection::isDebugOn() const
{
	return mDebug;
}

void NetworkConnection::setDebug(bool debug)
{
	mDebug = debug;
}

void NetworkConnection::connectToServer(QString address, quint16 port)
{
	mServer = address;
	mPort = port;
	mSocket->connectToHost(address, port);
}

void NetworkConnection::disconnect()
{
	mStatus = connClose;
	mSocket->close();
}

void NetworkConnection::onConnected()
{
	mStatus = connWaitBanner;
	qDebug()<<"Connection established";
}

void NetworkConnection::onClosed()
{
	qDebug()<<"Connection closed";
	mLastMessage.restart();
	if (
	        (mStatus == connLogonVerified) ||
	        (mStatus == connActive) ||
	        (mStatus == connReconnect)
	        )
	{
		mStatus = connReconnect;
	}
	else
	{
		mStatus = connTerminated;
	}
	emit connectionClosed();
}

void NetworkConnection::onReadyRead()
{
	while(!mSocket->atEnd())
	{
		mBuffer.append(mSocket->readAll());
	}
	int start=0;
	while (1)
	{
		int sep = mBuffer.indexOf("\n", start);
		if (sep<0)
			sep = mBuffer.indexOf("\r", start);
		if (sep<0)
			break;
		QByteArray line = mBuffer.mid(start, sep-start);
		start=sep+1;
		processLine(line);
	}
	mBuffer = mBuffer.mid(start);
}

void NetworkConnection::onSocketError(QAbstractSocket::SocketError e)
{
	qDebug()<<e;
}

void NetworkConnection::onCheckTimer()
{
	if ((mStatus == connReconnect)) {
		if (mLastMessage.elapsed() > MIN_RECONNECT_TIME)
		{
			qDebug()<<"trying to reconnect to"<<mServer;
			connectToServer(mServer, mPort);
		}
	}
	if ((mStatus == connActive) && (mLastMessage.elapsed() > 5000))
	{
		mLastMessage.restart();
		mSocket->write("# ping\r\n");
	}
}


} //namespace
