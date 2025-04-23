#ifndef VNCCLIENTTHREAD_H
#define VNCCLIENTTHREAD_H

#include <QDebug>
#include <QThread>
#include <QWidget>
#include <QPainter>
#include <QImage>
#include <rfb/rfbclient.h>
#include <QMutex>

class VNCClientThread : public QThread {
    Q_OBJECT
public:
    // 构造函数，初始化服务器 IP、端口、父级组件等信息，初始时不创建 displayWidget
    explicit VNCClientThread(const QString& ip, int port, QWidget* parentWidget = nullptr)
        : QThread(parentWidget), serverIP(ip), serverPort(port),
          displayWidget(nullptr), parentWidget(parentWidget),
          client(nullptr), isConnected(false), clientData(nullptr) {}

    // 析构函数，断开与服务器的连接，释放 displayWidget 和 clientData
    ~VNCClientThread() {
        disconnectFromServer();
        delete displayWidget;
        // 释放 rfbClientData 结构体
        if (clientData) {
            delete clientData;
            clientData = nullptr;
        }
    }

    // 获取显示窗口（用于添加到 QTabWidget）
    QWidget* getDisplayWidget() {
        return displayWidget;
    }

    // 设置显示窗口
    void setDisplayWidget(QWidget* widget) {
        displayWidget = widget;
    }

    // 在主线程中设置显示窗口，若之前已创建则先释放，再创建新的显示窗口
    void setupDisplayWidgetInMainThread(QWidget* parent) {
        if (displayWidget) {
            delete displayWidget;
        }
        displayWidget = new QWidget(parent);
        displayWidget->resize(800, 600);
    }

public slots:
    // 连接到服务器
    void connectToServer() {
        // 使用互斥锁保护 isConnected 标志，避免多线程访问冲突
        QMutexLocker locker(&connectionMutex);
        if (isConnected) return;

        // 初始化客户端
        client = rfbGetClient(8, 3, 4);
        if (!client) {
            qDebug() << "Failed to initialize VNC client";
            return;
        }

        // 配置像素格式（严格根据服务端日志设置，仅使用 rfbPixelFormat 存在的成员）
        client->format.bitsPerPixel = 32;    // 服务端为 32 位像素
        client->format.depth = 32;            // 深度与 bitsPerPixel 一致
        client->format.bigEndian = false;     // 服务端为小端序（least significant byte first）
        client->format.trueColour = true;      // 使用真彩色
        client->format.redMax = 0xff;
        client->format.greenMax = 0xff;
        client->format.blueMax = 0xff;
        client->format.redShift = 0;           // 服务端红通道偏移 0（最低 8 位：BGR 顺序）
        client->format.greenShift = 8;         // 绿色通道偏移 8（中间 8 位）
        client->format.blueShift = 16;         // 蓝色通道偏移 16（最高 8 位）

        // 创建 rfbClientData 结构体并存储 this 指针
        clientData = new rfbClientData;
        clientData->tag = nullptr;
        clientData->data = this;  // 将当前对象指针存入 data 成员
        clientData->next = nullptr;
        client->clientData = clientData;  // 设置 clientData

        // 设置帧缓冲区更新回调
        client->GotFrameBufferUpdate = handleFrameBufferUpdate;

        // 连接服务器
        int argc = 0;
        char** argv = nullptr;
        if (!rfbInitClient(client, &argc, argv)) {
            qDebug() << "Failed to connect to server";
            rfbClientCleanup(client);
            // 释放 rfbClientData 结构体
            if (clientData) {
                delete clientData;
                clientData = nullptr;
            }
            client = nullptr;
            return;
        }

        isConnected = true;
        qDebug() << "Connected to" << serverIP << ":" << serverPort;

        qDebug() << "Client pixel format: bitsPerPixel=" << client->format.bitsPerPixel
                 << ", depth=" << client->format.depth
                 << ", bigEndian=" << client->format.bigEndian
                 << ", trueColour=" << client->format.trueColour
                 << ", redMax=" << (int)client->format.redMax
                 << ", greenMax=" << (int)client->format.greenMax
                 << ", blueMax=" << (int)client->format.blueMax
                 << ", redShift=" << client->format.redShift
                 << ", greenShift=" << client->format.greenShift
                 << ", blueShift=" << client->format.blueShift;
        runEventLoop();
    }

    // 断开与服务器的连接
    void disconnectFromServer() {
        // 使用互斥锁保护 isConnected 标志，避免多线程访问冲突
        QMutexLocker locker(&connectionMutex);
        if (!isConnected || !client) return;
        rfbClientCleanup(client);
        client = nullptr;
        isConnected = false;
        qDebug() << "Disconnected from server";
    }

signals:
    // 发送屏幕更新信号（主线程渲染）
    void screenUpdated(QImage image);

private:
    // 帧缓冲区更新回调（通过 clientData 获取上下文）
    static void handleFrameBufferUpdate(rfbClient* cl, int x, int y, int w, int h) {
        if (!cl->clientData) return;
        VNCClientThread* self = static_cast<VNCClientThread*>(cl->clientData->data);  // 从 data 成员取值
        if (self && self->client) {
            // 检查 frameBuffer 是否为有效指针
                    if (!self->client->frameBuffer) {
                        qDebug() << "Error: client->frameBuffer is a null pointer";
                        return;
                    }
            // 将帧缓冲区数据转换为临时 QImage（注意像素格式和字节顺序）
            // 服务端返回的是 BGR 格式，转换为 QImage 的 RGB 格式
            QImage tempImage(self->client->frameBuffer,
                             self->client->width,
                             self->client->height,
                             QImage::Format_RGB32);  // 32 位 RGB（不带 alpha）
            // 复制一份数据，确保 QImage 拥有自己的数据副本
            QImage image = tempImage.copy();
            image = image.rgbSwapped();          // BGR 转 RGB 的关键步骤
            // 检查转换后的 QImage 是否有效
                    if (image.isNull()) {
                        qDebug() << "Error: Converted QImage is null";
                        return;
                    }
                    qDebug() << "Received image size: " << image.width() << "x" << image.height();
            // 发送信号到主线程渲染
            QMetaObject::invokeMethod(self, "screenUpdated", Qt::QueuedConnection, Q_ARG(QImage, image));
        }
    }

    // 运行事件循环，处理服务器消息
    void runEventLoop() {
        while (true) {
            {
                            QMutexLocker locker(&connectionMutex);
                            if (!isConnected) break;
                        }
                        int result = WaitForMessage(client, 1000000);
                        if (result <= 0) {
                            qDebug() << "WaitForMessage failed with result: " << result;
                            disconnectFromServer();
                            break;
                        }
                        int handleResult = HandleRFBServerMessage(client);
                        if (handleResult != 0) {
                            qDebug() << "HandleRFBServerMessage failed with result: " << handleResult;
                            disconnectFromServer();
                            break;
                        }
       }
    }

    // 线程运行函数，调用 connectToServer 连接服务器
    void run() override {
        connectToServer();
    }

private:
    QString serverIP;
    int serverPort;
    QWidget* displayWidget;    // 显示窗口
    QWidget* parentWidget;     // 父级 UI 组件
    rfbClient* client;
    bool isConnected;
    rfbClientData* clientData; // 新增成员，用于存储 rfbClientData 结构体指针
    QMutex connectionMutex;    // 用于保护 isConnected 标志的互斥锁
};

#endif // VNCCLIENTTHREAD_H
