QT 	*= sql
CONFIG 	*= c++11
INCLUDEPATH *= $$PWD/Database

SOURCES += \
        $$PWD/Database/AsyncQuery.cpp \
        $$PWD/Database/AsyncQueryResult.cpp \
        $$PWD/Database/ConnectionManager.cpp \
        $$PWD/Database/AsyncQueryModel.cpp \
        $$PWD/Database/AsyncQueryQMLModel.cpp

HEADERS += \
        $$PWD/Database/AsyncQuery.h \
        $$PWD/Database/AsyncQueryResult.h \
        $$PWD/Database/ConnectionManager.h \
        $$PWD/Database/AsyncQueryModel.h \
        $$PWD/Database/AsyncQueryQMLModel.h
