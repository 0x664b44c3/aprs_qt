#ifndef PACKETHUB_H
#define PACKETHUB_H

#include <QObject>

class PacketHub : public QObject
{
	Q_OBJECT
public:
	explicit PacketHub(QObject *parent = nullptr);

signals:

};

#endif // PACKETHUB_H
