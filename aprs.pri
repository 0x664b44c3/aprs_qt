QT+=network
INCLUDEPATH+=$$PWD

HEADERS += \
	$$PWD/aprs.h \
	$$PWD/networkconnection.h \
	$$PWD/packethub.h

SOURCES += \
	$$PWD/aprs.cpp \
	$$PWD/networkconnection.cpp \
	$$PWD/packethub.cpp

DISTFILES += \
	$$PWD/LICENSE \
	$$PWD/README.md
