#ifndef SFTPSESSION_H
#define SFTPSESSION_H

#include <QString>
#include <QDateTime>
#include "SshCmd.h"
#include "SshCmdResult.h"
#include "LoginUser.h"
#include <QMap>

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <libssh2.h>
#include <libssh2_sftp.h>
#pragma comment(lib, "ws2_32.lib")


class SftpSession {
public:
    int index; //所绑定对应的Tab所在索引
    int m_socket;//创建的ssh的socket对象（创建LIBSSH2_SESSION* session会话连接需要使用）
    LIBSSH2_SESSION* session;//连接的session对象（LIBSSH2_SESSION*可以跨线程传递，LIBSSH2_CHANNEL*不行，一个线程只能有一个channel）
    LIBSSH2_SFTP* sftp; //连接的sftp对象
};

#endif // SFTPSESSION_H
