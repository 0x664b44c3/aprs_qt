#ifndef APRS_H
#define APRS_H
#include <QString>
#include <QByteArray>
#include <QStringList>
#include <QDateTime>
#include <QtGlobal>
namespace APRS {

struct Frame {
	bool isValid;
	QString source;
	QString destination;
	QStringList via;
	QByteArray payload;
};

enum LocatorType
{
	LocatorInvalid=-1,
	LocatorNull=0,
	LocatorGPS=1,
	LocatorGpsCompressed=2,
	LocatorMaidenHead4
};

struct Locator
{
	double lat;
	double lon;
	double alt;
	LocatorType type;
	int accuracy;
	quint16 symbol;
};

struct ObjectReport {
	QString name;
	QString comment;
	bool alive;
	QDateTime timestamp;
	Locator position;
	quint16 symbol;
};

ObjectReport decodeObjectReport(const Frame &);

QDateTime decodeTime(QString time);
QDebug operator<<(QDebug debug, const Frame &f);


unsigned char getType(const Frame & f);
Locator decodeLocator(const QString &);
int calcPasscode(QString callsign);
Frame decodeFrame(const QByteArray &);
}
#endif // APRS_H
