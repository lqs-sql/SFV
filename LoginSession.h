#ifndef LOGINSESSION_H
#define LOGINSESSION_H


#include <QString>
#include <QDateTime>
#include "SshCmd.h"
#include "SshCmdResult.h"
#include "LoginUser.h"
#include <QMap>

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>
#include <libssh2.h>
#include <libssh2_sftp.h>
#pragma comment(lib, "ws2_32.lib")


class LoginSession {
public:
    LoginUser loginUser;
    QString loginSessionId;//连接的主键id值
    int index; //所绑定对应的Tab所在索引
    QDateTime createTime; //连接的时间
    QMap<SshCmd,SshCmdResult> cmdMap;//执行的命令Map，key是命令，value对应结果
    int m_socket;//创建的ssh的socket对象（创建LIBSSH2_SESSION* session会话连接需要使用）
    LIBSSH2_SESSION* session;//连接的session对象（LIBSSH2_SESSION*可以跨线程传递，LIBSSH2_CHANNEL*不行，一个线程只能有一个channel）
    LIBSSH2_CHANNEL* channel;//连接的channel对象（LIBSSH2_SESSION*可以跨线程传递，LIBSSH2_CHANNEL*不行，一个线程只能有一个channel）



    int code; //连接状态码
    QString msg;//连接新
    LoginSession(){}

    LoginSession(const LoginUser loginUser,const QString loginSessionId,const int index,const QDateTime createTime,const int m_socket,const int code,const QString msg,
                 const QMap<SshCmd,SshCmdResult> cmdMap = QMap<SshCmd, SshCmdResult>()
                 ,LIBSSH2_SESSION* session=nullptr
            ,LIBSSH2_CHANNEL* channel=nullptr){
        this->loginUser = loginUser;
        this->loginSessionId = loginSessionId;
        this->index = index;
        this->createTime = createTime;
        this->cmdMap = cmdMap;
        this->m_socket = m_socket;
        this->session = session;
        this->channel = channel;


        this->code = code;
        this->msg = msg;
    }

    void connectSuccess(const int code,QString msg,int m_socket){
        this->m_socket = m_socket;

        this->code = code;
        this->msg = msg;
    }

    void connectSuccess(QString msg,int m_socket){
        connectSuccess(200,msg,m_socket);
    }
    void connectFail(QString msg,int m_socket){
        connectSuccess(500,msg,-1);
    }
    void connectFail(QString msg){
        connectSuccess(500,msg,-1);
    }

    void sessionSuccess(const int code,QString msg,LIBSSH2_SESSION* session){
        this->session = session;

        this->code = code;
        this->msg = msg;
    }

    void sessionSuccess(QString msg,LIBSSH2_SESSION* session){
        sessionSuccess(200,msg,session);
    }
    void sessionFail(QString msg,LIBSSH2_SESSION* session){
        sessionSuccess(500,msg,nullptr);
    }
    void sessionFail(QString msg){
        sessionFail(msg,nullptr);
    }

    void channelSuccess(const int code,QString msg,LIBSSH2_SESSION* session,LIBSSH2_CHANNEL* channel){
        this->session = session;
        this->channel = channel;
        this->code = code;
        this->msg = msg;
    }

    void channelSuccess(QString msg,LIBSSH2_SESSION* session,LIBSSH2_CHANNEL* channel){
        channelSuccess(200,msg,session,channel);
    }
    void channelFail(QString msg,LIBSSH2_SESSION* session,LIBSSH2_CHANNEL* channel){
        channelSuccess(500,msg,nullptr,nullptr);
    }
    void channelFail(QString msg){
        channelFail(msg,nullptr,nullptr);
    }


    //复制新值并返回
    LoginSession copyNewLoginSession(){
        LoginSession newLoginSession =* new LoginSession();

        LoginUser newLoginUser=*new LoginUser();
        newLoginSession.loginSessionId = this->loginSessionId;
        newLoginSession.index = this->index;
        newLoginSession.createTime = this->createTime;
        newLoginSession.m_socket = this->m_socket;
        newLoginSession.session = this->session;
        newLoginSession.channel = this->channel;
        newLoginSession.code = this->code;
        newLoginSession.msg = this->msg;

        LoginUser oldLoginUser=this->loginUser;
        newLoginUser.loginUserId = oldLoginUser.loginUserId;
        newLoginUser.loginUserName = oldLoginUser.loginUserName;
        newLoginUser.loginUserPassword = oldLoginUser.loginUserPassword;
        newLoginUser.loginIp = oldLoginUser.loginIp;
        newLoginUser.loginPort = oldLoginUser.loginPort;
        newLoginUser.loginUserTime = oldLoginUser.loginUserTime;
        newLoginSession.loginUser = newLoginUser;

        newLoginSession.cmdMap = this->cmdMap;

        return newLoginSession;
    }
    //析构函数必须按照规范，自己定义delete this会导致野指针问题：19:49:38: 程序异常结束。 19:49:38: The process was ended forcefully.
    ~LoginSession(){}

};

#endif // LOGINSESSION_H
