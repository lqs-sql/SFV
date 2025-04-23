#ifndef SFTPCONNECTTHREAD_H
#define SFTPCONNECTTHREAD_H

#include "QSshSocket.h"
#include <QDebug>
#include "LoginUser.h"
#include "LoginSession.h"
#include <QThread>
#include <QUuid>

class SftpConnectThread : public QThread {
    Q_OBJECT
public:
    //在 C++ 里，如果要为参数设置默认值，必须从右向左连续设置。也就是说，一旦某个参数有了默认值，它右边的所有参数都必须有默认值。
    SftpConnectThread(LoginSession loginSession,QObject *parent = nullptr)
        : QThread(parent), loginSession(loginSession){}

signals:
    //发送追加内容信号：index给哪个窗口发送、content发送的追加内容
    void updateSftpSession(SftpSession* sftpSession);

protected:
    //子线程的具体运行内容
    void run() override {
        QSshSocket sshSocket;
        SftpSession* sftpSession = sshSocket.createSftp(this->loginSession);
        //发送更新后的sftp连接信号
        emit updateSftpSession(sftpSession);

    }
public:
    LoginSession loginSession; //操作的loginSession
};


#endif // SFTPCONNECTTHREAD_H
