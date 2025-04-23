#ifndef MONITORPATHCHANGETHREAD_H
#define MONITORPATHCHANGETHREAD_H

#include "QSshSocket.h"
#include <QDebug>
#include "LoginUser.h"
#include "LoginSession.h"
#include <QThread>
#include <QUuid>

class MonitorPathChangeThread : public QThread {
    Q_OBJECT
public:
    //在 C++ 里，如果要为参数设置默认值，必须从右向左连续设置。也就是说，一旦某个参数有了默认值，它右边的所有参数都必须有默认值。
    MonitorPathChangeThread(LoginSession loginSession,QString signalStr,QObject *parent = nullptr)
        : QThread(parent), loginSession(loginSession),signalStr(signalStr){}

signals:
//    //发送追加内容信号：index给哪个窗口发送、content发送的追加内容
//    void appendContent(int index,QString content);

protected:
    //子线程的具体运行内容
    void run() override {
        // 获取子线程ID
        qint64 subThreadId = reinterpret_cast<qint64>(QThread::currentThreadId());


        QSshSocket sshSocket;
        qDebug() <<"标志串："<<signalStr;
        //执行一条pwd命令监控路径变化，让文件回显
        // 构建完整命令
        QString command = QString("echo \" "+signalStr+" $(pwd) \"; \n");
        qDebug() <<"执行串："<<command;
        sshSocket.executeShellCommand(command,loginSession.session,loginSession.channel);
    }
public:
    LoginSession loginSession; //操作的loginSession
    QString signalStr; //监控文件路径变化的标志符号字符串
};

#endif // MONITORPATHCHANGETHREAD_H
