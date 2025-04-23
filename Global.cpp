// global.cpp
#include "global.h"

QString EXE_APPLICATION_PATH = ""; // 全局变量定义和初始化

QString EXE_APPLICATION_NAME = "局域网聊天";


QQueue <QByteArray> message;
int readNum = -1;
QStringList onLine;
