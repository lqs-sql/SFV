#include "QSshSocket.h"
#include <QDebug>

#include "LoginSession.h"
#include "Result.h"
#include "ExecuteCommandResult.h"
#include "StringUtil.h"


#include <QMutex>
#include <QMutexLocker>


#include <QUuid>
#include <QDateTime>
#include <QThread>
#include <QTreeView>
#include <QStandardItemModel>
#include <QStandardItem>
#include <QFileIconProvider>
#include <QDir>

QSshSocket::QSshSocket() {
//    QMutexLocker locker(&mutex); //在这里自动锁定mutex（在这个区域内对共享数据的操作是线程安全的）
//    m_socket=-1;
//    m_session=nullptr;
    m_winsockInitialized = false;
    qDebug() << "Initializing QSshSocket";
    // 初始化 libssh2 库
    if (libssh2_init(0) != 0) {
        qDebug() << "libssh2 initialization failed";
    }
    // 初始化 Winsock 库
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) == 0) {
        m_winsockInitialized = true;
    } else {
        qDebug() << "Winsock initialization failed";
    }
}

QSshSocket::~QSshSocket() {
//    closeSession();
//    // 清理 libssh2 库
//    libssh2_exit();
//    // 清理 Winsock 库
//    if (m_winsockInitialized) {
//        WSACleanup();
//    }
}
/**
 * 创建连接
 * @brief QSshSocket::createConnection
 * @param ip
 * @param port
 * @return
 */
LoginSession QSshSocket::createConnection(const QString &ip, int port) {
    LoginSession connectResult = *new LoginSession();
    if (!m_winsockInitialized) {
        qDebug() << "Winsock is not initialized";
        connectResult.connectFail("Winsock is not initialized");
        return connectResult;
    }
    // 创建 socket
    int socketId = socket(AF_INET, SOCK_STREAM, 0);
    if (socketId == INVALID_SOCKET) {
        qDebug() << "Socket creation failed";
        connectResult.connectFail("Socket creation failed");
        return connectResult;
    }
    // 设置服务器地址和端口
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    int result = InetPton(AF_INET, ip.toUtf8().constData(), &sin.sin_addr);
    if (result != 1) {
        qDebug() << "IP address conversion failed";
        closesocket(socketId);
        socketId = -1;
        connectResult.connectFail("IP address conversion failed");
        return connectResult;
    }
    // 有阻塞连接到服务器，使用 ::connect 明确调用全局命名空间的 connect 函数（缺点：在无法连接时会一直阻断在这，等待连接成功，导致程序卡死）
//    if (::connect(socketId, (struct sockaddr*)&sin, sizeof(sin)) == SOCKET_ERROR) {
//        qDebug() << "Connect failed";
//        closesocket(socketId);
//        socketId = -1;
//        return -1;
//    }
    // 设置套接字为非阻塞模式
    u_long mode = 1;
    if (ioctlsocket(socketId, FIONBIO, &mode) == SOCKET_ERROR) {
        qDebug() << "ioctlsocket failed";
        closesocket(socketId);
        connectResult.connectFail("ioctlsocket failed");
        return connectResult;
    }
    // 尝试连接
    result = ::connect(socketId, (struct sockaddr *)&sin, sizeof(sin));
    if (result == SOCKET_ERROR) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            qDebug() << "Connect failed";
            closesocket(socketId);
            connectResult.connectFail("Connect failed");
            return connectResult;
        }
    }
    // 使用 select 函数检查连接状态
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(socketId, &writefds);
    struct timeval timeout;
    timeout.tv_sec = 3;  // 设置超时时间为 3 秒
    timeout.tv_usec = 0;
    result = select(0, NULL, &writefds, NULL, &timeout);
    if (result == 0) {
        qDebug() << "Connect timed out";
        closesocket(socketId);
        connectResult.connectFail("Connect timed out");
        return connectResult;
    } else if (result == SOCKET_ERROR) {
        qDebug() << "Select failed";
        closesocket(socketId);
        connectResult.connectFail("Select failed");
        return connectResult;
    }
    // 检查套接字是否可写
    if (FD_ISSET(socketId, &writefds)) {
        int error;
        socklen_t len = sizeof(error);
        if (getsockopt(socketId, SOL_SOCKET, SO_ERROR, (char *)&error, &len) == SOCKET_ERROR) {
            qDebug() << "getsockopt failed";
            closesocket(socketId);
            connectResult.connectFail("getsockopt failed");
            return connectResult;
        }
        if (error != 0) {
            qDebug() << "Connect failed";
            closesocket(socketId);
            connectResult.connectFail("Connect failed");
            return connectResult;
        }
        qDebug() << "连接成功";
        connectResult.connectSuccess("Connect success", socketId);
        return connectResult;
    }
    qDebug() << "连接成功";
    connectResult.connectSuccess("Connect success", socketId);
    return connectResult;
}
void QSshSocket::closeConnection(int socketId) {
    if (socketId != -1) {
        closesocket(socketId);
        socketId = -1;
    }
}
/**
 * 创建session
 * @brief QSshSocket::createSession
 * @param username
 * @param password
 * @return
 */
LoginSession QSshSocket::createSession(const QString &username, const QString &password, LoginSession &loginSession) {
    int socketId = loginSession.m_socket;
    if (socketId == -1) {
        qDebug() << "Connection not established";
        loginSession.sessionFail("Connection not established");
        return loginSession;
    }
    // 创建 libssh2 会话
    LIBSSH2_SESSION *sessionId = libssh2_session_init();
    if (!sessionId) {
        qDebug() << "Session initialization failed";
        loginSession.sessionFail("Session initialization failed");
        return loginSession;
    }
    // 开始 SSH 会话
    if (libssh2_session_handshake(sessionId, socketId) != 0) {
        qDebug() << "Session startup failed";
        libssh2_session_free(sessionId);
        sessionId = nullptr;
        loginSession.sessionFail("Session startup failed");
        return loginSession;
    }
    // 设置会话超时时间（单位：毫秒）
    libssh2_session_set_timeout(sessionId, 3000); // 设置为 3 秒，必须在libssh2_userauth_password方法前执行，否则认证失败会一直卡死
    // 验证用户身份
    if (authenticate(sessionId, username, password) == false) {
        qDebug() << "Authentication failed";
        libssh2_session_free(sessionId);
        sessionId = nullptr;
        loginSession.sessionFail("Authentication failed");
        return loginSession;
    }
    //创建连接成功
    loginSession.sessionSuccess("连接成功", sessionId);
    return loginSession;
}


/**
 * 创建session
 * @brief QSshSocket::createSession
 * @param username
 * @param password
 * @return
 */
LoginSession QSshSocket::createChannel(const QString &username, const QString &password, LoginSession &loginSession) {
    int socketId = loginSession.m_socket;
    if (socketId == -1) {
        qDebug() << "Connection not established";
        loginSession.sessionFail("Connection not established");
        return loginSession;
    }
    // 创建 libssh2 会话
    LIBSSH2_SESSION *sessionId = libssh2_session_init();
    if (!sessionId) {
        qDebug() << "Session initialization failed";
        loginSession.sessionFail("Session initialization failed");
        return loginSession;
    }
    // 开始 SSH 会话
    if (libssh2_session_handshake(sessionId, socketId) != 0) {
        qDebug() << "Session startup failed";
        libssh2_session_free(sessionId);
        sessionId = nullptr;
        loginSession.sessionFail("Session startup failed");
        return loginSession;
    }
    // 设置会话超时时间（单位：毫秒）
    libssh2_session_set_timeout(sessionId,2000); // 设置为 2 秒，必须在libssh2_userauth_password方法前执行，否则认证失败会一直卡死
    // 验证用户身份
    if (authenticate(sessionId, username, password) == false) {
        qDebug() << "Authentication failed";
        libssh2_session_free(sessionId);
        sessionId = nullptr;
        loginSession.sessionFail("Authentication failed");
        return loginSession;
    }
    //请求一个shell渠道
    LIBSSH2_CHANNEL *channel = libssh2_channel_open_session(sessionId);
    if (!channel) {
        char *errmsg;
        int errlen;
        int err = libssh2_session_last_error(sessionId, &errmsg, &errlen, 0);
        qDebug() << "Channel open failed: " << QString::fromUtf8(errmsg, errlen);
        loginSession.channelFail("Channel open failed: " + QString::fromUtf8(errmsg, errlen));
        return loginSession;
    }
//    // 在创建channel后设置终端类型
//    libssh2_channel_setenv(channel, "TERM", "dumb");
    /** 创建虚拟终端（libssh2_channel_request_pty里的dumb模式进行连接，
     *          xterm（支持ANSI/颜色/鼠标等，ANSI会有特殊字符处理，在网页里可以使用xterm.js兼容，但exe里需要自己支持如esc、颜色处理，比较麻烦）
                vt100（基础终端）
                vt220（增强型终端）
                screen（终端复用器）
                dumb（无特殊功能，禁用ANSI）
                linux（Linux控制台默认类型）
    ）**/
    if (libssh2_channel_request_pty(channel, "dumb") != 0) {
        qDebug() << "Xterm Connection execution failed";
        libssh2_channel_free(channel);
        loginSession.channelFail("Xterm Connection execution failed");
        return loginSession;
    }
    // 启动 shell
    if (libssh2_channel_shell(channel) != 0) {
        qDebug() << "Command execution failed";
        libssh2_channel_free(channel);
        loginSession.channelFail("Command execution failed");
        return loginSession;
    }
//    libssh2_channel_set_blocking(channel,1);//0-非阻塞，1-阻塞
    //创建连接成功
    loginSession.channelSuccess("连接成功", sessionId, channel);
    return loginSession;
}
void QSshSocket::closeChannel(LIBSSH2_CHANNEL *channel) {
    if (channel) {
        libssh2_channel_close(channel);
        libssh2_channel_free(channel);
    }
}
void QSshSocket::closeSession(LIBSSH2_SESSION *session) {
    if (session) {
        libssh2_session_disconnect(session, "Normal Shutdown, Thank you for playing");
        libssh2_session_free(session);
        session = nullptr;
    }
}

bool QSshSocket::authenticate(LIBSSH2_SESSION *session, const QString &username, const QString &password) {
    if (libssh2_userauth_password(session, username.toUtf8().constData(), password.toUtf8().constData()) != 0) {
        return false;
    }
    return true;
}


Result<ExecuteCommandResult> QSshSocket::executeCommand(const QString &command, LIBSSH2_SESSION *session) {
    QMutexLocker locker(&mutex); //在这里自动锁定mutex（在这个区域内对共享数据的操作是线程安全的）
    Result <ExecuteCommandResult> result;
    ExecuteCommandResult executeCommandResult;
    executeCommandResult.commandType = 0; //判断是否不需要密码的非阻塞0
    if (!session) {
        qDebug() << "Session not created";
        return Result<ExecuteCommandResult>::fail("Session not created", executeCommandResult);
    }
    //请求一个shell渠道
    LIBSSH2_CHANNEL *channel = libssh2_channel_open_session(session);
    if (!channel) {
        char *errmsg;
        int errlen;
        int err = libssh2_session_last_error(session, &errmsg, &errlen, 0);
        qDebug() << "Channel open failed: " << QString::fromUtf8(errmsg, errlen);
        return Result<ExecuteCommandResult>::fail("Channel open failed: " + QString::fromUtf8(errmsg, errlen), executeCommandResult);
    }
    //创建虚拟终端（libssh2_channel_request_pty里的xterm使用交互模式进行连接）
    if (libssh2_channel_request_pty(channel, "xterm") != 0) {
        qDebug() << "Xterm Connection execution failed";
        libssh2_channel_free(channel);
        return Result<ExecuteCommandResult>::fail("Xterm Connection execution failed", executeCommandResult);
    }
    //执行单条命令
    if (libssh2_channel_shell(channel) != 0) {
        qDebug() << "Command execution failed";
        libssh2_channel_free(channel);
        return Result<ExecuteCommandResult>::fail("Command execution failed", executeCommandResult);
    }
    // 读取命令输出
    QString output;
    char buffer[1024];
    ssize_t byteCount;
//    executeCommandResult.channel = channel;
    while ((byteCount = libssh2_channel_read(channel, buffer, sizeof(buffer))) > 0) {
        if (strstr(buffer, "密码：") != NULL) { //判断包含密码：文本认为其需要输入密码，规定
            executeCommandResult.commandType = 1;
            break;
        } else {
            executeCommandResult.commandType = 0;
        }
        if (byteCount < 0) {
            result.code = 500;
        } else {
            result.code = 200;
        }
        result.msg = QString::fromUtf8(buffer, byteCount);
        qDebug() << "read命令结果：" << result.msg;
        return result;
    }
    // 关闭通道
    libssh2_channel_close(channel);
    libssh2_channel_free(channel);
    return Result<ExecuteCommandResult>::success(output, executeCommandResult);
}

/**
 * 发送阻塞的结束命令
 * @brief sendEndEof
 * @param channel
 * @return 返回值<0则说明报错，>=0则发送
 */
int QSshSocket::sendBlockEndSignal(LIBSSH2_CHANNEL *channel){
    int rc = libssh2_channel_write(channel, "\x03", 1);
    return rc;
}
Result<ExecuteCommandResult> QSshSocket::executeShellCommand(const QString &command, LIBSSH2_SESSION *session, LIBSSH2_CHANNEL *channel) {
    QMutexLocker locker(&mutex); //在这里自动锁定mutex（在这个区域内对共享数据的操作是线程安全的）
        Result <ExecuteCommandResult> result;
        ExecuteCommandResult executeCommandResult;
        // 检查当前通道是否存活
        if (libssh2_channel_eof(channel)) {
            qDebug() << "Channel is closed. Reconnecting...";
            // 关闭并释放旧通道
            libssh2_channel_close(channel);
            libssh2_channel_free(channel);

            qDebug()<<"channel处于结束状态";
            channel = libssh2_channel_open_session(session);
            if (!channel) {
                return Result<ExecuteCommandResult>::fail("Failed to open a new session channel", executeCommandResult);
            }
            //创建虚拟终端（libssh2_channel_request_pty里的dumb模式进行连接）
            if (libssh2_channel_request_pty(channel, "dumb") != 0) {
                libssh2_channel_free(channel);
                return Result<ExecuteCommandResult>::fail("linux Connection execution failed", executeCommandResult);
            }
        }


        //必须设置非阻塞
        libssh2_channel_set_blocking(channel,0);

        // 发送命令
        result = writeChannel(channel, command);

        // 获取阻塞命令的错误信息
        int err;
        char *errmsg;
        err = libssh2_session_last_error(session, &errmsg, nullptr, 0);
        if(err!=0 || result.code!=200){
            qDebug() << "libssh2_channel_read 发生错误，错误码（错误码可以在libssh2.h里搜索）: " << err << " 错误信息: " << QString::fromUtf8(errmsg);
    //        QThread::msleep(1000);
    //        readChannel(channel);
    //        channel = reconnectChannel(session,channel);
        }
        return result;
}

QString QSshSocket::getSessionLastErrot(LIBSSH2_SESSION *session){
    // 获取阻塞命令的错误信息
    int err;
    char *errmsg;
    err = libssh2_session_last_error(session, &errmsg, nullptr, 0);
    if(err!=0 ){
        return QString("session报错："+ QString::number(err) + "，  错误信息: " + QString::fromUtf8(errmsg));
    }
}

void QSshSocket::sendHeartBeatPackage(LIBSSH2_SESSION *session, LIBSSH2_CHANNEL *channel) {
    Result <ExecuteCommandResult> result;
    ExecuteCommandResult executeCommandResult;
    if (libssh2_channel_eof(channel)) {
        qDebug()<<"channel处于结束状态";
        channel = libssh2_channel_open_session(session);
        if (!channel) {
            return ;
        }
        //创建虚拟终端（libssh2_channel_request_pty里的dumb模式进行连接）
        if (libssh2_channel_request_pty(channel, "dumb") != 0) {
            libssh2_channel_free(channel);
            return ;
        }
    }
    //发送心跳命令包// 发送一个简单的命令作为心跳包
    int writeResult = libssh2_channel_exec(channel, "echo heartbeat");
    if(writeResult > 0){
        qDebug()<<"心跳包发送成功";
        return ;
    }else{
        qDebug()<<"心跳包发送失败";
        return ;
    }
}

/**
 * 管道写入数据（在shell模式下，等同于执行一条命令）
 * @brief QSshSocket::wirteChannel
 * @param data
 * @param channel
 * @return
 */
Result<ExecuteCommandResult> QSshSocket::writeChannel(LIBSSH2_CHANNEL *channel, const QString &command) {
    qDebug() << "wirte命令：" << command;
    Result<ExecuteCommandResult> result;
    ExecuteCommandResult executeCommandResult;
    int writeResult = libssh2_channel_write(channel, command.toUtf8().constData(), strlen(command.toUtf8().constData()));


//   result = readChannel(channel);
    if(writeResult > 0){
        qDebug()<<"write命令执行成功，读取返回内容";
        return result;
    }else{
        qDebug()<<"write命令执行失败,命令:"+command;
        return Result<ExecuteCommandResult>::fail(command + "命令执行失败：", executeCommandResult);
    }


}

/**
 * 管道读取数据（在shell模式下，等同于读取一条命令执行结果）
 * @brief QSshSocket::wirteChannel
 * @param data
 * @param channel
 * @return
 */
Result<ExecuteCommandResult> QSshSocket::readChannel(LIBSSH2_CHANNEL *channel) {
//    QMutexLocker locker(&mutex);
//    LIBSSH2_POLLFD pollfd;
//    pollfd.type = LIBSSH2_POLLFD_CHANNEL;
//    pollfd.fd.channel = channel;
//    pollfd.events = LIBSSH2_POLLFD_POLLIN; // 监听可读事件

//    int rc = libssh2_poll(&pollfd, 1, 1500); // 1.5秒超时
//    if (rc > 0 && (pollfd.revents & LIBSSH2_POLLFD_POLLIN)) {
//        qDebug()<<"poll有可读";
    // 读取命令输出
    char buffer[1024]; //接收管道数据的字节数组
    ssize_t byteCount; //记录管道数据的字节数
    Result<ExecuteCommandResult> result;
    ExecuteCommandResult executeCommandResult;
    int i = 1;
    QString output;
    while ((byteCount = libssh2_channel_read(channel, buffer, sizeof(buffer))) > 0) {
        // 可以在这里处理读取到的登录信息，但不保存到最终结果中
        QString partOutput = QString::fromUtf8(buffer, byteCount);
        output.append(partOutput);
//        if(output!=partOutput){
           qDebug() << "命令1第"+QString::number(i)+"次执行后读取：" << partOutput;
            qDebug()<<"发送了readTimeCommandResultUpdate信号";
           emit readTimeCommandResultUpdate(partOutput);
//        }
        i++;
    }
    executeCommandResult.commandType =  getCommndType(output);
    executeCommandResult.content = output;
    if(executeCommandResult.commandType == 0){
        executeCommandResult.content = output;
        result.code = 200;
        result.msg = "读取成功";
        result.data = executeCommandResult;
        return result;
    }
    if(executeCommandResult.commandType == 1){
        executeCommandResult.content = output;
        result.code = 200;
        result.msg = "读取成功，阻塞命令";
        result.data = executeCommandResult;

        return result;
    }
//    }
}



//获取执行命令的命令类型：阻塞、密码
int  QSshSocket::getCommndType(QString output){
//    qDebug()<<"getCommandType判断命令："<<output;
    int commandType = -1;//0-非阻塞，1-阻塞（2-密码）
    if(output.trimmed().endsWith("]$")||output.trimmed().endsWith("]#")){
        //命令执行结果成功，处于非阻塞状态（成功|失败，失败则返回的是错误信息）
        commandType = 0;
//        qDebug()<<"非阻塞命令";
        return commandType;
    }else{
        commandType = 1;
//        qDebug()<<"阻塞命令";
        //命令执行结果失败，处于阻塞状态
        if(output.trimmed().endsWith("密码：")){
            commandType = 2;
            qDebug()<<"密码命令";
            return commandType;
        }else{

        }
    }
    return commandType;
}

//Result<int> QSshSocket::executeSuCommand(const QString &command,const QString &password, LIBSSH2_SESSION *session) {
//    QMutexLocker locker(&mutex); //在这里自动锁定mutex（在这个区域内对共享数据的操作是线程安全的）
//    Result<int> result ;


//    //请求一个shell渠道
//    LIBSSH2_CHANNEL *channel = libssh2_channel_open_session(session);
//    if (!channel) {
//        char *errmsg;
//        int errlen;
//        int err = libssh2_session_last_error(session, &errmsg, &errlen, 0);
//        qDebug() << "Channel open failed: " << QString::fromUtf8(errmsg, errlen);
//        return Result<int>::fail("Channel open failed: "+QString::fromUtf8(errmsg, errlen),-1);
//    }
//    //创建虚拟终端（libssh2_channel_request_pty里的xterm使用交互模式进行连接）
//    if (libssh2_channel_request_pty(channel, "xterm") != 0)
//    {
//        qDebug() << "Xterm Connection execution failed";
//        libssh2_channel_free(channel);
//        return Result<int>::fail("Xterm Connection execution failed",-1);
//    }
//    //执行单条命令
//    if (libssh2_channel_exec(channel, command.toUtf8().constData()) != 0) {
//        qDebug() << "Command execution failed";
//        libssh2_channel_free(channel);
//        return Result<int>::fail("Command execution failed",-1);
//    }
//    // 读取命令输出
//    QString output;
//    char buffer[1024];
//    ssize_t byteCount;
//    while((byteCount = libssh2_channel_read(channel, buffer, sizeof(buffer))) > 0) {
//        if(strstr(buffer, "密码：") != NULL) {//判断包含password文本认为其需要输入密码
//            qDebug()<<"libssh命令需要密码认证";
//            // 发送密码进行身份验证
//            int writeResult = libssh2_channel_write(channel, "shenqinlinshiwodie.123.\n", strlen("shenqinlinshiwodie.123.\n"));

//            char buffer2[1024];
//            byteCount = libssh2_channel_read(channel, buffer2, sizeof(buffer2));
//            QString output = QString::fromUtf8(buffer2, byteCount);
//            if(writeResult>0){
//                qDebug()<<"write命令执行成功，结果："<<output;
//                return Result<int>::success(output,-1);
//            }else{
//                qDebug()<<"write命令执行失败，结果："<<output;
//                return Result<int>::fail(output,-1);
//            }
//        }else{//非密码认证相关命令
//            QString output = QString::fromUtf8(buffer, byteCount);
//            qDebug()<<"libssh命令不需要密码认证";
//            qDebug()<<"read命令结果："<<output;
//        }
//    }
//    if(byteCount < 0){
//        result.code = 500;
//    }else{
//        result.code = 200;
//    }
//    result.msg=output;

//    // 关闭通道
//    libssh2_channel_close(channel);
//    libssh2_channel_free(channel);
//    return Result<int>::success(output,-1);
//}


/**
 * 执行su开头命令(创建一个新的Session方式)
 * @brief QSshSocket::executeSuCommand
 * @param command
 * @param session
 * @return
 */
LoginSession QSshSocket::executeSuCommand(const QString &ipAddr, const int port, const QString &username, const QString &password) {
    // 重新连接并使用新用户认证
    LoginSession loginSession = createConnection(ipAddr, port);
    // 修正动态分配对象的方式，使用智能指针或者直接创建对象（这里使用直接创建对象）
    LoginUser loginUser(QUuid::createUuid().toString(), username, password, ipAddr, port, QDateTime::currentDateTime());
    loginSession.loginUser = loginUser;
    loginSession.loginSessionId = QUuid::createUuid().toString();
    loginSession.createTime = QDateTime::currentDateTime();
    if (loginSession.code == 200) {
        qDebug() << "创建连接成功";
        loginSession = this->createSession(username, password, loginSession);
        if (loginSession.code == 200) {
            qDebug() << "认证成功";
            return loginSession;
        } else {
            qDebug() << "认证失败";
            // 修正动态分配对象的方式，使用智能指针或者直接创建对象（这里使用直接创建对象）
            loginSession.code = 500;
            loginSession.msg = "Authentication Failed,Reason：" + loginSession.msg + "\n";
            return loginSession;
        }
    } else {
        qDebug() << "创建连接失败：" << loginSession.msg;
        loginSession.code = 500;
        loginSession.msg = "Connect Failed,Reason：" + loginSession.msg + "\n";
        return loginSession;
    }
}



/**
 * 开启对应的sftp
 * @brief createSftp
 * @param loginSession
 * @return
 */
SftpSession* QSshSocket::createSftp(LoginSession loginSession){
    SftpSession* sftpSession = new SftpSession();
    if (!m_winsockInitialized) {
        qDebug() << "Winsock is not initialized";
        return sftpSession;
    }
    // 创建 socket
    int socketId = socket(AF_INET, SOCK_STREAM, 0);
    if (socketId == INVALID_SOCKET) {
        qDebug() << "Socket creation failed";
        return sftpSession;
    }
    // 设置服务器地址和端口
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(loginSession.loginUser.loginPort);
    int result = InetPton(AF_INET,loginSession.loginUser.loginIp.toUtf8().constData(), &sin.sin_addr);
    if (result != 1) {
        qDebug() << "IP address conversion failed";
        closesocket(socketId);
        socketId = -1;
        return sftpSession;
    }
    // 设置套接字为阻塞模式
    u_long mode = 1;
    if (ioctlsocket(socketId, FIONBIO, &mode) == SOCKET_ERROR) {
        qDebug() << "ioctlsocket failed";
        closesocket(socketId);
        return sftpSession;
    }
    // 尝试连接
    result = ::connect(socketId, (struct sockaddr *)&sin, sizeof(sin));
    if (result == SOCKET_ERROR) {
        if (WSAGetLastError() != WSAEWOULDBLOCK) {
            qDebug() << "Connect failed";
            closesocket(socketId);
            return sftpSession;
        }
    }
    // 使用 select 函数检查连接状态
    fd_set writefds;
    FD_ZERO(&writefds);
    FD_SET(socketId, &writefds);
    struct timeval timeout;
    timeout.tv_sec = 3;  // 设置超时时间为 3 秒
    timeout.tv_usec = 0;
    result = select(0, NULL, &writefds, NULL, &timeout);
    if (result == 0) {
        qDebug() << "Connect timed out";
        closesocket(socketId);
        return sftpSession;
    } else if (result == SOCKET_ERROR) {
        qDebug() << "Select failed";
        closesocket(socketId);
        return sftpSession;
    }
    // 检查套接字是否可写
    if (FD_ISSET(socketId, &writefds)) {
        int error;
        socklen_t len = sizeof(error);
        if (getsockopt(socketId, SOL_SOCKET, SO_ERROR, (char *)&error, &len) == SOCKET_ERROR) {
            qDebug() << "getsockopt failed";
            closesocket(socketId);
            return sftpSession;
        }
        if (error != 0) {
            qDebug() << "Connect failed";
            closesocket(socketId);
            return sftpSession;
        }
        qDebug() << "连接成功";
    }

    sftpSession->m_socket = socketId;
    // 初始化 libssh2 库
     if (libssh2_init(0) != 0) {
         qDebug()<< "Failed to initialize libssh2";
         return sftpSession;
     }

     // 创建 SSH 会话
     LIBSSH2_SESSION* session = libssh2_session_init();
     if (!session) {
         qDebug()<< "Failed to initialize SSH session";
         libssh2_exit();
         return sftpSession;
     }

     // 执行 SSH 握手
     if (libssh2_session_handshake(session, socketId) != 0) {
         qDebug()<< "SSH handshake failed";
         libssh2_session_free(session);
         libssh2_exit();
         return sftpSession;
     }

     // 设置会话超时时间（单位：毫秒）
       libssh2_session_set_timeout(session, 2000);

       QByteArray usernameBytes = loginSession.loginUser.loginUserName.trimmed().toUtf8();
       QByteArray passwordBytes = loginSession.loginUser.loginUserPassword.trimmed().toUtf8();
       const char* username = usernameBytes.constData();
       const char* password = passwordBytes.constData();

       // 非阻塞式认证
       int authResult;
       while (true) {
           authResult = libssh2_userauth_password(session, username, password);
           if (authResult != LIBSSH2_ERROR_EAGAIN) {
               break;
           }
           QThread::msleep(10); // 适当延时，避免 CPU 占用过高
       }

       if (authResult != 0) {
           qDebug() << "Authentication failed:" << getSessionLastErrot(session);
           libssh2_session_free(session);
           libssh2_exit();
           return sftpSession;
       }

       sftpSession->session = session;

       // 非阻塞式初始化 SFTP 会话
       LIBSSH2_SFTP* sftp = nullptr;
       while (true) {
           sftp = libssh2_sftp_init(session);
           if (sftp || libssh2_session_last_error(session, nullptr, nullptr, 0) != LIBSSH2_ERROR_EAGAIN) {
               break;
           }
           QThread::msleep(10); // 适当延时，避免 CPU 占用过高
       }

       if (!sftp) {
           qDebug() << "Failed to initialize SFTP session" << getSessionLastErrot(session);
           libssh2_session_free(session);
           libssh2_exit();
           return sftpSession;
       }

       sftpSession->sftp = sftp;
       return sftpSession;

}
/**
 * 判断远程路径是否为文件
 * @brief QSshSocket::isFile
 * @param sftp
 * @param path
 * @return
 */
bool QSshSocket::isFile(LIBSSH2_SFTP *sftp, const QString &path) {
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    int rc = libssh2_sftp_stat(sftp, path.toUtf8().constData(), &attrs);
    if (rc == 0) {
        return !(attrs.permissions & LIBSSH2_SFTP_S_IFDIR);
    }
    return false;
}

// 辅助函数：判断是否为目录
bool QSshSocket::isDir(LIBSSH2_SFTP *sftp, const QString &path) {
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    int rc = libssh2_sftp_stat(sftp, path.toUtf8().constData(), &attrs);
    if (rc == 0) {
        return attrs.permissions & LIBSSH2_SFTP_S_IFDIR;
    }
    return false;
}


// 递归删除目录及其子目录和文件
bool QSshSocket::recursiveRemoveDir(LIBSSH2_SFTP* sftp, const QString& path, bool deleteMyself) {
    LIBSSH2_SFTP_HANDLE* handle = libssh2_sftp_opendir(sftp, path.toUtf8().constData());
    if (!handle) {
        qDebug() << "无法打开目录: " << path;
        return false;
    }

    LIBSSH2_SFTP_ATTRIBUTES attrs;
    char filename[512];
    while (1) {
        int rc = libssh2_sftp_readdir(handle, filename, sizeof(filename), &attrs);
        if (rc <= 0) {
            break;
        }

        QString qFilename = QString::fromUtf8(filename);
        if (qFilename == "." || qFilename == "..") {
            continue;
        }

        QString subPath = path + "/" + qFilename;
        if (attrs.permissions & LIBSSH2_SFTP_S_IFDIR) {
            // 递归删除子目录
            if (!recursiveRemoveDir(sftp, subPath, deleteMyself)) {
                libssh2_sftp_closedir(handle);
                return false;
            }else{//删除成功
                //子目录里当前目录也需要删除
                if (libssh2_sftp_rmdir(sftp, subPath.toUtf8().constData()) != 0) {
                    qDebug() << "无法删除目录: " << subPath ;
                    return false;
                }
            }
        } else {
            // 删除文件
            if (libssh2_sftp_unlink(sftp, subPath.toUtf8().constData()) != 0) {
                qDebug() << "无法删除文件: " << subPath;
                libssh2_sftp_closedir(handle);
                return false;
            }
        }
    }

    libssh2_sftp_closedir(handle);

    if (deleteMyself) {
        if (libssh2_sftp_rmdir(sftp, path.toUtf8().constData()) != 0) {
            qDebug() << "无法删除目录: " << path ;
            return false;
        }
    }

    return true;
}

//下载远程文件到本地
bool QSshSocket::downloadFile(LIBSSH2_SFTP* sftp,const QString& remotePath,const QString& localPath){
    // 打开远程文件
    LIBSSH2_SFTP_HANDLE* sftp_handle = libssh2_sftp_open(sftp, remotePath.toUtf8().constData(), LIBSSH2_FXF_READ, 0);
    if (!sftp_handle) {
        qDebug() << "Failed to open remote file:" << remotePath;
        return false;
    }

    // 检查文件所在目录是否存在，不存在则递归创建
    QFileInfo fileInfo (localPath);
    QDir dir = fileInfo.dir();
    if (!dir.exists ()) {
        if (!dir.mkpath(dir.absolutePath ())) {
        qDebug () << "Failed to create directory:" << dir.path ();
        libssh2_sftp_close (sftp_handle);
        return false;
        }
    }
    // 打开本地文件（若不存在则创建）
    QFile localFile (localPath);
    if (!localFile.open (QIODevice::WriteOnly)) {
        qDebug () << "Failed to open or create local file:" << localPath << "Error:" << localFile.errorString ();
        libssh2_sftp_close (sftp_handle);
        return false;
    }

    // 下载文件内容
    char buffer[1024];
    ssize_t bytes_read;
    while ((bytes_read = libssh2_sftp_read(sftp_handle, buffer, sizeof(buffer))) > 0) {
        qint64 bytes_written = localFile.write(buffer, bytes_read);
        if (bytes_written != bytes_read) {
            qDebug() << "Failed to write to local file.";
            localFile.close();
            libssh2_sftp_close(sftp_handle);
            return false;
        }
    }

    // 关闭本地文件和远程文件
    localFile.close();
    libssh2_sftp_close(sftp_handle);

    return true;
}
