#include "vnc_server.h"
#include <QImage>
#include <QDebug>

VncServer::VncServer(QObject *parent) : QObject(parent), m_server(nullptr) {}

VncServer::~VncServer() {
    if (m_server) rfbScreenCleanup(m_server);
}

// vnc_server.cpp (修正部分)
bool VncServer::startServer(int port) {
    // 初始化屏幕参数
    m_server = rfbGetScreen(nullptr, nullptr, 800, 600, 8, 3, 4);
    m_server->desktopName = "QtVncServer";
    m_server->frameBuffer = (char*)malloc(800 * 600 * 4); // 32bpp
    m_server->port = port;
    m_server->alwaysShared = TRUE;

    // 设置像素格式 (ARGB32 兼容 Qt)
    // 不再使用 rfbFormat，直接设置 rfbScreenInfo 的像素字段
    m_server->serverFormat.bitsPerPixel = 32;
    m_server->serverFormat.depth = 24;
    m_server->serverFormat.trueColour = TRUE;
    m_server->serverFormat.redShift   = 16;
    m_server->serverFormat.greenShift = 8;
    m_server->serverFormat.blueShift  = 0;
    m_server->serverFormat.redMax     = 255;
    m_server->serverFormat.greenMax   = 255;
    m_server->serverFormat.blueMax    = 255;

    // 启动服务线程
    rfbInitServer(m_server);
    rfbRunEventLoop(m_server, -1, TRUE); // 非阻塞模式

    qDebug() << "VNC Server started on port" << port;
    return true;
}
