TEMPLATE 			 = lib
TARGET 				 = asyncsql
QT 					+= core sql
QT 					-= gui
CONFIG 				+= c++11 static
OBJECTS_DIR			 = tmp
MOC_DIR 				 = tmp
DEFINES 				+=	QT_MESSAGELOGCONTEXT

SOURCES 				+= \
							Database/AsyncQuery.cpp \
							Database/AsyncQueryResult.cpp \
							Database/ConnectionManager.cpp \
							Database/AsyncQueryModel.cpp \
							Database/AsyncQueryQMLModel.cpp

HEADERS 				+= \
							Database/AsyncQuery.h \
							Database/AsyncQueryResult.h \
							Database/ConnectionManager.h \
							Database/AsyncQueryModel.h \
							Database/AsyncQueryQMLModel.h
