#-------------------------------------------------
#
# Project created by QtCreator 2016-02-16T07:23:31
#
#-------------------------------------------------

QT += core gui sql

CONFIG += c++11

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = QtThreadedAsyncSql
TEMPLATE = app


SOURCES += main.cpp\
	mainwindow.cpp \
	Database/AsyncQuery.cpp \
	Database/AsyncQueryResult.cpp \
	Database/ConnectionManager.cpp \
        Database/AsyncQueryModel.cpp

HEADERS += mainwindow.h \
	Database/AsyncQuery.h \
	Database/AsyncQueryResult.h \
	Database/ConnectionManager.h \
        Database/AsyncQueryModel.h

FORMS += mainwindow.ui

