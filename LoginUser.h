#ifndef LOGINUSER_H
#define LOGINUSER_H

#include <QString>
#include <QDateTime>
#include "SshCmd.h"
#include "SshCmdResult.h"

class LoginUser {
public:
    QString loginUserId; //已登录用户的用户id
    QString loginUserName; //已登录用户的用户名
    QString loginUserPassword;  //已登录用户的密码
    QString loginIp; //已登录的ip
    int loginPort; //已登录的端口
    QDateTime loginUserTime; //已登录用户的登录时间

    LoginUser(){}
    LoginUser(const QString &loginUserId, const QString & loginUserName, const QString & loginUserPassword,
              const QString & loginIp,const int loginPort,
              const QDateTime &loginUserTime){
        this->loginUserId = loginUserId;
        this->loginUserName = loginUserName;
        this->loginUserPassword = loginUserPassword;
        this->loginIp = loginIp;
        this->loginPort = loginPort;
        this->loginUserTime = loginUserTime;
    }

    //析构函数必须按照规范，自己定义delete this会导致野指针问题：19:49:38: 程序异常结束。 19:49:38: The process was ended forcefully.
    ~LoginUser(){}

};
#endif // LOGINUSER_H
