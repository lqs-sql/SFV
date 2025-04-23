// vnc_client.cpp
#include "vnc_client.h"
#include <QTimer>
#include <QDebug>

VncClient::VncClient(QWidget *parent) : QWidget(parent), m_client(nullptr) {
    m_screenLabel = new QLabel(this);
    setFixedSize(800, 600);
}

void VncClient::connectToServer(const char* host, int port) {
    m_client = rfbGetClient(8, 3, 4); // 8bpp, 3 channels, 4 bytes
    m_client->serverHost = strdup(host);
    m_client->serverPort = port;
    m_client->programName = "QtVncClient";
    m_client->canHandleNewFBSize = TRUE;
    m_client->MallocFrameBuffer = resizeWindow;
    m_client->GotFrameBufferUpdate = onUpdate;

    // 强制类型转换：VncClient* -> rfbClientData*
    m_client->clientData = reinterpret_cast<rfbClientData*>(this);

    if (rfbInitClient(m_client, nullptr, nullptr)) {
        qDebug() << "Connected to VNC Server";
        QTimer *timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, &VncClient::updateFrame);
        timer->start(30); // 30ms刷新
    } else {
        qDebug() << "Connection failed!";
    }
}

void VncClient::updateFrame() {
    if (m_client && WaitForMessage(m_client, 50000) > 0)
        HandleRFBServerMessage(m_client);
}

// 静态回调：处理帧更新
void VncClient::onUpdate(rfbClient* client, int x, int y, int w, int h) {
    // 使用 reinterpret_cast 转换 void* 到 VncClient*
    VncClient* widget = reinterpret_cast<VncClient*>(client->clientData);
    QImage image((uchar*)client->frameBuffer, client->width, client->height, QImage::Format_RGB32);
    widget->m_screenLabel->setPixmap(QPixmap::fromImage(image).scaled(widget->size()));
}

// 静态回调：调整窗口大小
rfbBool VncClient::resizeWindow(rfbClient* client) {
    VncClient* widget = reinterpret_cast<VncClient*>(client->clientData);
    widget->setFixedSize(client->width, client->height);
    client->frameBuffer = (uint8_t*)malloc(client->width * client->height * 4);
    return TRUE;
}
