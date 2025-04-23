#ifndef GLOBAL_H
#define GLOBAL_H
#include <QString>
#include <QObject>
#include<QQueue>
#include<QStringList>


//定义全局变量
extern QString EXE_APPLICATION_PATH;
extern QString EXE_APPLICATION_NAME;

extern QQueue <QByteArray> message;
extern   int readNum;
extern QStringList onLine;
#endif // GLOBAL_H
