#ifndef QSSHSOCKET_H
#define QSSHSOCKET_H

#define _WIN32_WINNT 0x0600

#include <QObject>
#include <QString>
#include <libssh2.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(dll, "ws2_32.dll")

#include "LoginSession.h"
#include "Result.h"
#include "ExecuteCommandResult.h"
#include "SftpSession.h"

#include <QMutex>
#include <QMutexLocker>
#include <QTreeView>
#include <QStandardItem>


class QSshSocket: public QObject
{
    Q_OBJECT
signals:
    // 自定义的实时更新命令的执行结果信号
    void readTimeCommandResultUpdate(QString commandResult);
public:
     QSshSocket();
    ~QSshSocket();

    /**
     * @brief 创建 SSH 连接
     * @param ip 服务器 IP 地址
     * @param port 服务器端口号
     * @return 连接的套接字描述符，若失败返回 -1
     */
    LoginSession createConnection(const QString &ip, int port);

    /**
     * 关闭Connection
     * @brief closeConnection
     * @param socketId
     */
    void closeConnection(int socketId);

    /**
     * @brief 关闭 SSH 连接
     */
    void closeSession(LIBSSH2_SESSION *session);

    /**
     * @brief 创建 SSH 会话
     * @param username 用户名
     * @param password 密码
     * @param socketId createConnection创建的套接字socketId
     * @return 会话对象指针，若失败返回 nullptr
     */
    LoginSession createSession(const QString &username, const QString &password, LoginSession& loginSession);

    /**
     * 开启会话渠道
     * @brief QSshSocket::createChannel
     * @param username
     * @param password
     * @param loginSession
     * @return
     */
    LoginSession createChannel(const QString &username, const QString &password, LoginSession &loginSession);

    /**
     * 开启对应的sftp
     * @brief createSftp
     * @param loginSession
     * @return
     */
    SftpSession* createSftp(LoginSession loginSession);

    /**
     * 关闭会话渠道
     * @brief closeChannel
     * @param channel
     */
    void closeChannel(LIBSSH2_CHANNEL *channel);
    /**
     * @brief 关闭 SSH 会话
     */
    void closeSession();

    /**
     * @brief 验证用户身份
     * @param session 会话对象指针
     * @return 验证成功返回 true，失败返回 false
     */
    bool authenticate(LIBSSH2_SESSION *session, const QString &username, const QString &password);


    /**
     * 执行远程命令
     * @brief QSshSocket::executeCommand
     * @param command
     * @param session
     * @return
     */
    Result<ExecuteCommandResult> executeCommand(const QString &command,LIBSSH2_SESSION *session);

    /**
     * 执行交互式远程命令
     * @brief
     * @param command
     * @param session
     * @return
     */
    Result<ExecuteCommandResult> executeShellCommand(const QString &command, LIBSSH2_SESSION *session,LIBSSH2_CHANNEL *channel);
    /**
     * 管道写入数据（在shell模式下，等同于执行一条命令）
     * @brief QSshSocket::wirteChannel
     * @param data
     * @param channel
     * @return
     */
    Result<ExecuteCommandResult> writeChannel( LIBSSH2_CHANNEL *channel ,const QString &command);
    /**
     * 管道读取数据（在shell模式下，等同于读取一条命令执行结果）
     * @brief QSshSocket::wirteChannel
     * @param data
     * @param channel
     * @return
     */
    Result<ExecuteCommandResult> readChannel(LIBSSH2_CHANNEL *channel);

    /**
     * 根据输出结果判断write写入执行的命令类型
     * @brief getCommndType
     * @param output
     * @return
     */
    int  getCommndType(QString output);
//    /**
//     * 执行su开头命令
//     * @brief QSshSocket::executeSuCommand
//     * @param command
//     * @param session
//     * @return
//     */
//    Result<int> executeSuCommand(const QString &command, const QString& password,LIBSSH2_SESSION *session);

    /**
     * 发送阻塞的结束命令
     * @brief sendEndEof
     * @param channel
     * @return 返回值<0则说明报错，>=0则发送
     */
    int sendBlockEndSignal(LIBSSH2_CHANNEL *channel);
    /**
     * 执行su开头命令(创建一个新的Session方式)
     * @brief QSshSocket::executeSuCommand
     * @param command
     * @param session
     * @return
     */
    LoginSession executeSuCommand(const QString &ipAddr,const int port,const QString &username, const QString& password);


    void sendHeartBeatPackage(LIBSSH2_SESSION *session, LIBSSH2_CHANNEL *channel);

    //获取session最后报错信息
    QString getSessionLastErrot(LIBSSH2_SESSION *session);


    /**
     * 判断远程路径是否为文件
     * @brief QSshSocket::isFile
     * @param sftp
     * @param path
     * @return
     */
    bool isFile(LIBSSH2_SFTP *sftp, const QString &path);
    /**
     * 判断远程路径是否为目录
     * @brief isDir
     * @param sftp
     * @param path
     * @return
     */
    bool isDir(LIBSSH2_SFTP *sftp, const QString &path);

    // 递归删除目录及其子目录和文件
    bool recursiveRemoveDir(LIBSSH2_SFTP* sftp, const QString& path, bool deleteMyself);


    //下载远程文件到本地
    bool downloadFile(LIBSSH2_SFTP* sftp,const QString& remotePath,const QString& localPath);
private:

    bool m_winsockInitialized;

   mutable QMutex mutex;//线程安全互斥锁
    mutable QMutex channelMutex;
};

#endif // QSSHSOCKET_H
