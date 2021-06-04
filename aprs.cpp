#include "aprs.h"
#include <QByteArray>
#include <QDebug>
#include <QRegExp>
#include <QDateTime>
#include <QTimeZone>
namespace APRS {

int calcPasscode(QString callsign)
{
	int dashPos = callsign.indexOf('-');
	if (dashPos>0)
		callsign = callsign.left(dashPos);
	//uppercase, max 10 chars
	callsign=callsign.toUpper().left(10);
	quint16 hash = 0x73e2;
	QByteArray callsignData=callsign.toLatin1();

	for (int i=0;i<callsign.size(); i+=2)
	{
		hash ^= ((quint8)callsignData.at(i)) << 8;
		if ((i+1)<callsign.size())
			hash ^= ((quint8)callsignData.at(i+1));
	}
	return hash & 0x7fff;
}

Frame decodeFrame(const QByteArray & in)
{
	Frame ret;
	ret.isValid = 0;
	//find the right chevron
	int endSource = in.indexOf('>');
	if (in.startsWith('#')||(endSource<0))
	{ //invalid frame;
		return ret;
	}
	ret.source = QString::fromLatin1(in.left(endSource));
	int payLoadOffset = in.indexOf(':', endSource+1);
	if (payLoadOffset<0)
		return ret;
	ret.isValid = true;
	ret.payload = in.mid(payLoadOffset + 1);
	QString dest = QString::fromLatin1(in.mid(endSource+1), payLoadOffset - endSource - 1);
	QStringList hopList = dest.split(',');
	ret.destination = hopList.takeFirst();
	ret.via = hopList;
	return ret;
}

QDebug operator<<(QDebug debug, const Frame &f)
{
	QDebugStateSaver saver(debug);
	Q_UNUSED(saver);
	debug.nospace().noquote() << "APRS::Frame(";
	if (!f.isValid)
		debug.nospace() << "invalid ";
	debug.nospace().noquote() << "from: '"
	                          << f.source
	                          << "', to: '"
	                          << f.destination<<"', ";
	if (f.via.size())
	{
		debug.nospace().noquote() << "via: [";
		bool firstHop=true;
		foreach(QString hop, f.via)
		{
			if (!firstHop)
				debug.nospace()<< ", ";
			firstHop = false;
			debug.nospace().noquote() << "'"<<hop<<"'";
		}
		        debug.nospace()<<"], ";
	}
	debug.nospace().noquote() << "payload: '" << f.payload;
	debug.nospace() <<"')";
	return debug;
	
}

QDateTime decodeTime(QString time)
{
	QDate cd = QDateTime::currentDateTimeUtc().date();
	QRegExp validTime7("\\d{6}\\w");
	QRegExp validTime8("\\d{8}");
	QDateTime ts;
	if (time.length() == 7)
	{
		if (validTime7.exactMatch(time))
		{
			int p1 = time.mid(0, 2).toInt();
			int p2 = time.mid(2, 2).toInt();
			int p3 = time.mid(4, 2).toInt();
			char type = time.at(6).toLatin1();
			switch (type)
			{

				case 'z':
				case '/':
					ts = QDateTime(
					            QDate(cd.year(), cd.month(), p1),
					            QTime(p2, p3, 0));
				case 'h':
					ts = QDateTime(
					            cd,
					            QTime(p1, p2, p3));
			}
			return ts;
		}
	}
	if ((time.length() == 8) && (validTime8.exactMatch(time))){
		int p1 = time.mid(0, 2).toInt();
		int p2 = time.mid(2, 2).toInt();
		int p3 = time.mid(4, 2).toInt();
		int p4 = time.mid(6, 2).toInt();
		ts = QDateTime(
		            QDate(cd.year(), p1, p2),
		            QTime(p3, p4, 0));
		return ts;
	}
	return QDateTime();
}

unsigned char getType(const Frame &f)
{
	if (f.payload.size()==0)
		return 0;
	return f.payload.at(0);
}

ObjectReport decodeObjectReport(const Frame & frame)
{
	ObjectReport obj;
	QByteArray payload = frame.payload;
	if ((APRS::getType(frame) != ';')
	    || (payload.size() <37))
		return ObjectReport();

	obj.name = QString::fromLatin1(frame.payload.mid(1, 9)).trimmed();
	obj.alive = (payload[10]=='*');
	QString payloadText = QString::fromLatin1(payload.mid(11));
	if (payloadText.isEmpty())
		return ObjectReport();

	obj.timestamp = decodeTime(payloadText.left(7));
	payloadText = payloadText.mid(7);
	qDebug()<<payloadText;
	if (payload.isEmpty())
		return obj;
	if (payloadText.at(0).isDigit())
	{
		obj.position = decodeLocator(payloadText.left(19));
		payloadText = payloadText.mid(19);
	}
	obj.comment = payloadText;
	return obj;
}

Locator decodeLocator(const QString & loc)
{
	Locator locator;
	locator.type = LocatorInvalid;
	if (loc.size()==0)
		return locator;
	locator.accuracy = -1;
	int lonAccuracy=3;
	int latAccuracy=3;
	QRegExp validLat("\\d{2}[\\d\\s]{2}\\.[\\d\\s]{2}[NS]", Qt::CaseInsensitive);
	QRegExp validLon("\\d{3}[\\d\\s]{2}\\.[\\d\\s]{2}[EW]", Qt::CaseInsensitive);
	if ((loc.length()>=19) //long enaugh for uncompressed coordinates
	        && (loc[0].isDigit())) // uncompressed starts with a digit
	{ //expect uncompressed coordinates

		QString latData = loc.left(8);
		if (validLat.exactMatch(latData)) {
			locator.lat  = latData.mid(0,2).toInt();
			locator.lat += latData.mid(2,1).toDouble() * (10.0 /   60.0);
			locator.lat += latData.mid(3,1).toDouble() * ( 1.0 /   60.0);
			locator.lat += latData.mid(5,1).toDouble() * (10.0 / 3600.0);
			locator.lat += latData.mid(6,1).toDouble() * ( 1.0 / 3600.0);
			if (latData.at(7) == 'S')
				locator.lat *= -1.0;
			for (int i=2; i<7;++i)
				if (latData.at(i).isDigit())
					latAccuracy+=1;
		}
		else
		{
			return locator;
		}
		QString lonData = loc.mid(9,9);
		if (validLon.exactMatch(lonData)) {
			locator.lon  = lonData.mid(0,3).toInt();
			locator.lon += lonData.mid(3,1).toDouble() * (10.0 /   60.0);
			locator.lon += lonData.mid(4,1).toDouble() * ( 1.0 /   60.0);
			locator.lon += lonData.mid(6,1).toDouble() * (10.0 / 3600.0);
			locator.lon += lonData.mid(7,1).toDouble() * ( 1.0 / 3600.0);
			if (lonData.at(8) == 'W')
				locator.lon *= -1.0;
			for (int i=3; i<8;++i)
				if (lonData.at(i).isDigit())
					lonAccuracy+=1;
		}
		else
		{
			return locator;
		}
		locator.accuracy = qMin(lonAccuracy, latAccuracy);
		locator.type = LocatorGPS;
		locator.symbol =   ((quint8) loc.at(8).toLatin1()) * 256
		                 + ((quint8) loc.at(18).toLatin1());
		return locator;
	}

}


} //</namespace>
