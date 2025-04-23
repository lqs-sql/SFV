#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QUuid>

#include "LoginSession.h"

#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <libssh2.h>
#pragma comment(lib, "ws2_32.lib")

#include <QThread>
#include <QSocketNotifier>
#include <cstring>
#include <QMetaType>

#include <Global.h>
#include <rfb/rfb.h>
#include <rfb/rfbclient.h>

#include "VNCServerThread.h"
#include "VNCClientThread.h"
#include <QVBoxLayout>
#include <QLabel>
#include "vnc_server.h"
#include "vnc_client.h"
#include "VncWidget.h"

//获取执行命令的命令类型：阻塞、密码
int getCommndType(QString output){
    int commandType = -1;//0-非阻塞，1-阻塞（2-密码）
    if(output.trimmed().endsWith("]$")){
        //命令执行结果成功，处于非阻塞状态（成功|失败，失败则返回的是错误信息）
        commandType = 0;
        qDebug()<<"非阻塞命令";
    }else{
        commandType = 1;
        qDebug()<<"阻塞命令";
        //命令执行结果失败，处于阻塞状态
        if(output.trimmed().endsWith("密码：")){
            commandType = 2;
            qDebug()<<"密码命令";
        }
    }
    return commandType;
}







int main(int argc, char *argv[]) {
    QApplication a(argc, argv);

    //注册需要跨线程的参数类型
    qRegisterMetaType<LoginSession>("LoginSession");

    //获取应用exe全局路径
    QString EXE_APPLICATION_PATH = QCoreApplication::applicationDirPath();


    // 启动 VNC 服务器线程
        VncServerThread serverThread;
        serverThread.start();

        // 启动 VNC 客户端窗口
        VncWidget w;
        w.show();

//    // 启动 VNC 服务端（本地测试）
//    VNCServerThread* vncServer = new VNCServerThread(5900);
//    QObject::connect(vncServer, &QThread::finished, vncServer, &QObject::deleteLater);
//    vncServer->start();

//    // 创建客户端显示窗口
//    QWidget mainWindow;
//    mainWindow.resize(800, 600);  // 匹配服务端分辨率

//    // 启动 VNC 客户端
//    VNCClientThread* vncClient = new VNCClientThread("127.0.0.1", 5900);

//    // 在主线程中设置 displayWidget
//    vncClient->setupDisplayWidgetInMainThread(&mainWindow);
//    // 创建一个 QLabel 用于显示图像
//    QLabel* imageLabel = new QLabel(&mainWindow);
//    imageLabel->setAlignment(Qt::AlignCenter);
//    // 手动设置 QLabel 的初始大小
//    imageLabel->resize(800, 600);
//    QVBoxLayout* layout = new QVBoxLayout(&mainWindow);
//    layout->addWidget(imageLabel);
//    layout->addWidget(vncClient->getDisplayWidget());
//    mainWindow.setLayout(layout);

//    // 连接接收线程的信号到更新 QLabel 的函数，并添加调试输出
//    bool connectResult = QObject::connect(vncClient, &VNCClientThread::screenUpdated, [&](const QImage& image) {
//        qDebug() << "screenUpdated signal received";
//        QPixmap pixmap = QPixmap::fromImage(image);
//        // 检查 scaled 方法的参数，确保图像能正确显示
//        QPixmap scaledPixmap = pixmap.scaled(imageLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
//        imageLabel->setPixmap(scaledPixmap);
//    });
//    if (!connectResult) {
//        qDebug() << "Error: Failed to connect screenUpdated signal";
//    }

//    // 启动客户端线程
//    vncClient->start();

//    // 显示主窗口
//    mainWindow.show();
//    // 强制刷新主窗口
//       mainWindow.update();


//    // 使用LibVNC的服务端功能
//    rfbScreenInfoPtr screen = rfbGetScreen(&argc, argv, 800, 600, 8, 3, 4);
//    if (screen) {
//        rfbInitServer(screen);
//        rfbRunEventLoop(screen, -1, FALSE);
//    }



//    //ip+端口：192.168.168.128:22
//    QSshSocket sshSocket;
//    int socketId = sshSocket.createConnection(QString("192.168.168.128"),22).m_socket;

//    // 创建 libssh2 会话
//    LIBSSH2_SESSION *sessionId = libssh2_session_init();
//    // 开始 SSH 会话
//    libssh2_session_handshake(sessionId, socketId);
//libssh2_session_set_timeout(sessionId, 3000);
//QString username = "shenqinlin";
//QString password = "woshisqlerzi";
//libssh2_userauth_password(sessionId, username.toUtf8().constData(), password.toUtf8().constData());
//     LIBSSH2_CHANNEL *channel = libssh2_channel_open_session(sessionId);
//     libssh2_channel_request_pty(channel, "linux");

//     libssh2_channel_shell(channel);


//     // 刷新缓存，读取登录信息和其他初始数据
//     char buffer[1024]; //接收管道数据的字节数组
//     ssize_t byteCount; //记录管道数据的字节数
//     // 读取命令输出
//     QString output;
//     while ((byteCount = libssh2_channel_read(channel, buffer, sizeof(buffer))) > 0) {
//        // 可以在这里处理读取到的登录信息，但不保存到最终结果中
//        QString partOutput = QString::fromUtf8(buffer, byteCount);
//        qDebug() << "登录后读取：" << partOutput;
//        output.append(partOutput);
//     }
//     qDebug()<<"登录后读取完整树数据："<<output;

////    //设置超时时间
////     libssh2_session_set_timeout(sessionId,2000);
////     //1-阻塞模式、0-非阻塞模式，不在阻塞模式下则无法读取到数据，默认为阻塞模式
////     libssh2_channel_set_blocking(channel,1);

//       // 发送channel结束标志（会关闭整个channel）
////       libssh2_channel_send_eof(channel);
//       int writeResult = -1;




//     //交互式命令执行
//   //  QString command = "ping 192.168.168.1 \n";
//    // QString command = "ping www.baidu.com \n";
//       QString command = "ls -l \n";
//  //       QString command = "su root \n";
// //       QString command = "cd /cadhkjasd/ \n";

//     writeResult = libssh2_channel_write(channel, command.toUtf8().constData(), strlen(command.toUtf8().constData()));
////     QThread::msleep(1000); // 增加延迟
//      QByteArray data1;
//      // 重新初始化缓冲区
////      memset(buffer, 0, sizeof(buffer));

//     if(writeResult > 0){
//         int i = 1;
//         QString output;
//         while ((byteCount = libssh2_channel_read(channel, buffer, sizeof(buffer))) > 0) {
//             // 可以在这里处理读取到的登录信息，但不保存到最终结果中
//             QString partOutput = QString::fromUtf8(buffer, byteCount);
//             output.append(partOutput);
//             if(output!=partOutput){
//                qDebug() << "命令1第"+QString::number(i)+"次执行后读取：" << partOutput;
//             }
//             i++;
//         }
////         getCommndType(output);
//         qDebug()<<"命令1执行后读取完整数据："<<output;

//     }


//     command = "cd  / \n";
////      command = "shenqinlinshiwodie.123. \n";
//     writeResult = libssh2_channel_write(channel, command.toUtf8().constData(), strlen(command.toUtf8().constData()));
////          QThread::msleep(1000); // 增加延迟
//     QByteArray data2;
//     // 重新初始化缓冲区
////     memset(buffer, 0, sizeof(buffer));
//     if(writeResult > 0){
//         int i = 1;
//         QString output;
//         while ((byteCount = libssh2_channel_read(channel, buffer, sizeof(buffer))) > 0) {
//             // 可以在这里处理读取到的登录信息，但不保存到最终结果中
//             QString partOutput  = QString::fromUtf8(buffer, byteCount);
//             output.append(partOutput);
//             if(output!=partOutput){
//                qDebug() << "命令2第"+QString::number(i)+"次执行后读取：" << partOutput;
//             }
//             i++;
//         }
////         getCommndType(output);
//         qDebug()<<"命令2执行后读取完整数据："<<output;
//     }






//     command = "pwd \n";
//     writeResult = libssh2_channel_write(channel, command.toUtf8().constData(), strlen(command.toUtf8().constData()));
////     QThread::msleep(1000); // 增加延迟

//     QByteArray data3;
//     // 重新初始化缓冲区
////     memset(buffer, 0, sizeof(buffer));
//     if(writeResult > 0){
//         int i = 1;
//         QString output;
//         while ((byteCount = libssh2_channel_read(channel, buffer, sizeof(buffer))) > 0) {
//             // 可以在这里处理读取到的登录信息，但不保存到最终结果中
//             QString partOutput = QString::fromUtf8(buffer, byteCount);
//             output.append(partOutput);
//             if(output!=partOutput){
//                qDebug() << "命令3第"+QString::number(i)+"次执行后读取：" << partOutput;
//             }
//             i++;
//         }
////         getCommndType(output);
//         qDebug()<<"命令3执行后读取完整数据："<<output;
//     }



    MainWindow mainWindow;
    mainWindow.show();




//    //ssh工具类使用
//    QSshSocket sshSocket;
//    ConnectResult connectResult = sshSocket.createConnection("192.168.168.128", 22);
//    if (connectResult.code == 200) {
//        qDebug() << "创建连接成功";
//        QString userName = "shenqinlin";
//        QString password = "woshisqlerzi";


//        SessionResult SessionResult= sshSocket.createSession(userName, password);
//        if (SessionResult.code == 200) {
//            LIBSSH2_SESSION *session = SessionResult.session;
//            qDebug() << "认证成功";
//            QString result = sshSocket.executeCommand("ls -l",session);
//            qDebug() << "Command output:" << result;

//            //登录认证成功，存放已登录信息

//            sshSocket.closeSession();
//        }else{
//            qDebug() << "认证失败";
//        }
//        sshSocket.closeConnection();
//    }else{
//        qDebug() << "创建连接失败";
//    }


    //普通的ssh测试代码
//    // 初始化 Winsock
//     WSADATA wsaData;
//     if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
//         qDebug() << "Winsock initialization failed";
//         return 1;
//     }
//     // 初始化 libssh2
//     if (libssh2_init(0) != 0) {
//         qDebug() << "libssh2 initialization failed";
//         WSACleanup();
//         return 1;
//     }
//     // 创建套接字
//     SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
//     if (sock == INVALID_SOCKET) {
//         qDebug() << "Socket creation failed";
//         libssh2_exit();
//         WSACleanup();
//         return 1;
//     }
//     // 设置服务器地址和端口
//     sockaddr_in sin{};
//     sin.sin_family = AF_INET;
//     sin.sin_port = htons(22); // SSH 默认端口
//     const char* server_ip = "192.168.168.128"; //SSH连接的ip地址
//     if (inet_pton(AF_INET, server_ip, &sin.sin_addr) != 1) {
//         qDebug() << "IP address conversion failed";
//         closesocket(sock);
//         libssh2_exit();
//         WSACleanup();
//         return 1;
//     }
//     // 连接到服务器
//     if (connect(sock, reinterpret_cast<sockaddr*>(&sin), sizeof(sin)) == SOCKET_ERROR) {
//         qDebug() << "Connect failed";
//         closesocket(sock);
//         libssh2_exit();
//         WSACleanup();
//         return 1;
//     }
//     // 创建 SSH 会话
//     LIBSSH2_SESSION *session = libssh2_session_init();
//     if (!session) {
//         qDebug() << "Session initialization failed";
//         closesocket(sock);
//         libssh2_exit();
//         WSACleanup();
//         return 1;
//     }
//     // 进行 SSH 握手
//     if (libssh2_session_handshake(session, sock) != 0) {
//         qDebug() << "Session handshake failed";
//         libssh2_session_free(session);
//         closesocket(sock);
//         libssh2_exit();
//         WSACleanup();
//         return 1;
//     }
//     // 身份验证（这里使用密码验证示例，你需要替换为实际的用户名和密码）
//     const char* username = "shenqinlin";
//     const char* password = "woshisqlerzi";
//     if (libssh2_userauth_password(session, username, password) != 0) {
//         qDebug() << "Authentication failed";
//     } else {
//         qDebug() << "Authentication succeeded";
//     }
//     // 清理资源
//     libssh2_session_disconnect(session, "Normal Shutdown, Thank you for playing");
//     libssh2_session_free(session);
//     closesocket(sock);
//     libssh2_exit();
//     WSACleanup();

        return a.exec();
}
