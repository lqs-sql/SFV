#ifndef WORKTHREAD_H
#define WORKTHREAD_H

#include "QSshSocket.h"
#include <QDebug>
#include "LoginUser.h"
#include "LoginSession.h"
#include <QThread>
#include <QUuid>

//工作线程(子线程，满足子线程处理任务，主线程更新ui的规则)
class WorkThread : public QThread {
    Q_OBJECT
public:
    //在 C++ 里，如果要为参数设置默认值，必须从右向左连续设置。也就是说，一旦某个参数有了默认值，它右边的所有参数都必须有默认值。
    WorkThread(QString ipAddr, int port, QString userName, QString password, int index, QObject *parent = nullptr)
        : QThread(parent), ipAddr(ipAddr), port(port), userName(userName), password(password), index(index) {}
signals:
    // 修正信号参数类型，去掉多余的类型名（因为已经在前面声明了类型）
    updateUI(LoginSession loginSession,
             bool loginStatus);//更新ui信号（loginStatus为true则添加新连接，false则显示报错信息）

protected:
    //子线程的具体运行内容
    void run() override {

        QSshSocket sshSocket;
        LoginSession loginSession = sshSocket.createConnection(ipAddr, port);

        qDebug() << "子线程ID: " << reinterpret_cast<qint64>(QThread::currentThreadId());

        // 假设 LoginSession 构造函数的最后一个参数应该是合适的类型（这里假设是 QMap<SshCmd, SshCmdResult>），根据实际情况修改
        QMap<SshCmd, SshCmdResult> emptyCmdMap;
        // 修正动态分配对象的方式，使用智能指针或者直接创建对象（这里使用直接创建对象）
        LoginUser loginUser(QUuid::createUuid().toString(), userName, password,ipAddr,port, QDateTime::currentDateTime());
        loginSession.loginUser = loginUser;
        loginSession.loginSessionId = QUuid::createUuid().toString();
        loginSession.index = index;
        loginSession.createTime = QDateTime::currentDateTime();
        loginSession.loginUser = loginUser;
        if (loginSession.code == 200) {
            qDebug() << "创建连接成功";
//                loginSession = sshSocket.createSession(userName, password,loginSession);
                  loginSession = sshSocket.createChannel(userName,password,loginSession);

            if (loginSession.code == 200) {
                qDebug() << "认证成功";
                //session连接成功（session和connect不同，属于开启的会话）
                //发送更新UI信号
                emit updateUI(loginSession, true);

            }else{
                qDebug() << "认证失败";
                // 修正动态分配对象的方式，使用智能指针或者直接创建对象（这里使用直接创建对象）
                loginSession.code = 500;
                loginSession.msg = "Authentication Failed,Reason：" + loginSession.msg + "\n";
                emit updateUI(LoginSession(), false);
            }
        }else{
            qDebug() << "创建连接失败："<<loginSession.msg;
            qDebug()<<"发送updateUI信号";
            emit updateUI(loginSession, false);

//            //构造Process测试ip和端口是否通
//            QString result = QProcessUtil::executeCommand("ping "+ipAddr);
//            if(result.contains("FailSign：")){//命令执行shi'b2标志
//                qDebug()<<"测试ping命令报错！"<<result;
//            }else if(result.contains("Request timed out")){
//                newTextEdit->insertPlainText("无法到达当前IP地址，请检查配置"+"\n");
//            }
//            result = QProcessUtil::executeCommand("telnet " +ipAddr+" "+port);
//            if(result.contains("FailSign：")){//命令执行shi'b2标志
//                qDebug()<<"测试telnet命令报错！"<<result;
//            }else if(result.contains("Could not open connection to the host")){
//                newTextEdit->insertPlainText("当前端口不通，请检查配置"+"\n");
//            }
        }


    }
private:
    //主线程开启子线程的方法，需要传入的参数（主线程提供给子线程的参数）
    QString ipAddr;//本次连接的ip地址
    int port;//本次连接的端口号
    QString userName;//本次连接的用户名
    QString password;//本次连接的密码
    int index;//当前窗口所在的索引
};

#endif // WORKTHREAD_H
