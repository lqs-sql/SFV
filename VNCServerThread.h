// VNCServerThread.h
#ifndef VNCSERVERTHREAD_H
#define VNCSERVERTHREAD_H

//#include <QThread>
//#include <rfb/rfb.h>

//class VNCServerThread : public QThread {
//    Q_OBJECT
//public:
//    explicit VNCServerThread(int port, QObject* parent = nullptr)
//        : QThread(parent), serverPort(port), server(nullptr), isRunning(false) {}

//    ~VNCServerThread() {
//        stopServer();
//    }

//public slots:
//    void stopServer() {
//        if (isRunning && server) {
//            rfbShutdownServer(server, true);
//            rfbScreenCleanup(server);
//            isRunning = false;
//        }
//    }

//protected:
//    void run() override {
//        // 初始化服务器
//        server = rfbGetScreen(nullptr, nullptr, 800, 600, 8, 3, 4);
//        if (!server) return;

//        // 配置像素格式（BGR 32位）
//        server->serverFormat.bitsPerPixel = 32;
//        server->serverFormat.depth = 24;
//        server->serverFormat.bigEndian = false;
//        server->serverFormat.trueColour = true;
//        server->serverFormat.redMax = 255;
//        server->serverFormat.greenMax = 255;
//        server->serverFormat.blueMax = 255;
//        server->serverFormat.redShift = 16;   // 红色在最高字节（BGR布局）
//        server->serverFormat.greenShift = 8;
//        server->serverFormat.blueShift = 0;

//        // 分配帧缓冲区
//        server->frameBuffer = new char[800 * 600 * 4];

//        // 填充渐变数据（BGR格式）
//        for (int y = 0; y < 600; ++y) {
//            uint32_t* fb = reinterpret_cast<uint32_t*>(server->frameBuffer + y * 800 * 4);
//            for (int x = 0; x < 800; ++x) {
//                uint8_t b = static_cast<uint8_t>(x * 255 / 800);
//                uint8_t r = static_cast<uint8_t>((800 - x) * 255 / 800);
//                *fb++ = (0xFF << 24) | (r << 16) | (0 << 8) | b;  // ARGB格式存储（实际BGR）
//            }
//        }

//        server->port = serverPort;
//        rfbInitServer(server);
//        isRunning = true;

//        // 事件循环
//        while (isRunning) {
//            rfbProcessEvents(server, 100000);
//        }
//    }

//private:
//    int serverPort;
//    rfbScreenInfoPtr server;
//    bool isRunning;
//};


#include <QThread>
#include <rfb/rfb.h>

// VNC 服务器线程类
class VncServerThread : public QThread
{
    Q_OBJECT
protected:
    void run() override
    {
        rfbScreenInfoPtr server = rfbGetScreen(nullptr, nullptr, 640, 480, 8, 3, 4);
        server->frameBuffer = (char*)malloc(640 * 480 * 4);

        rfbInitServer(server);
        rfbRunEventLoop(server, -1, FALSE);
    }
};

#endif // VNCSERVERTHREAD_H
