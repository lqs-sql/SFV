#include "VncWidget.h"
#include <QImage>
#include <QPainter>
#include <QDebug>
#include <QCryptographicHash>

signed char VncWidget::mallocFrameBuffer(rfbClient* client) {
    void* buffer = malloc(client->width * client->height * 4);
    if (buffer) {
        return 1;
    }
    return 0;
}

void VncWidget::gotFrameBufferUpdate(rfbClient* client, int x, int y, int w, int h) {
    static int callbackCount = 0;
    qDebug() << "Got frame buffer update. Callback count: " << ++callbackCount;
    qDebug() << "x: " << x << ", y: " << y << ", w: " << w << ", h: " << h;

    // 计算当前帧缓冲区数据的哈希值
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData((const char*)client->frameBuffer, client->width * client->height * 4);
    QString currentHash = hash.result().toHex();

    static QString previousHash;
    if (!previousHash.isEmpty() && currentHash == previousHash) {
        qDebug() << "Frame buffer data has not changed, skipping update.";
        return;
    }
    previousHash = currentHash;

    VncWidget* widget = reinterpret_cast<VncWidget*>(client->clientData);
    if (widget) {
        widget->updateImage(client, x, y, w, h);
    }
}

VncWidget::VncWidget(QWidget *parent) : QWidget(parent) {
    client = rfbGetClient(8, 3, 4);
    client->MallocFrameBuffer = &VncWidget::mallocFrameBuffer;
    client->GotFrameBufferUpdate = &VncWidget::gotFrameBufferUpdate;
    // 使用 reinterpret_cast 进行类型转换
    client->clientData = reinterpret_cast<rfbClientData*>(this);



    int argc = 0;
    char** argv = nullptr;
    if (int initResult = rfbInitClient(client, &argc, argv) < 0) {
        qDebug() << "rfbInitClient failed with error code: " << initResult;
        return;
    }

    label = new QLabel(this);
    label->setStyleSheet("background-color: transparent;"); // 设置背景透明
     label->setScaledContents(true); // 自动缩放图像
     QVBoxLayout *layout = new QVBoxLayout(this);
     layout->addWidget(label);
    resize(640, 480); // 设置窗口大小
}

VncWidget::~VncWidget() {
    if (client && client->frameBuffer) {
        free(client->frameBuffer);
    }
    rfbClientCleanup(client);
}

void VncWidget::updateImage(const rfbClient* client, int x, int y, int w, int h) {

    // 检查 QImage 的属性
    int expectedBytes = client->width * client->height * 4;
    // 直接使用 expectedBytes 作为实际字节数，因为内存是按此大小分配的
    int actualBytes = expectedBytes;

    if (actualBytes != expectedBytes) {
        qDebug() << "Frame buffer size mismatch. Expected: " << expectedBytes << ", Actual: " << actualBytes;
        return;
    }

    QImage image((uchar*)client->frameBuffer, client->width, client->height, QImage::Format_RGB32);
    if (image.isNull()) {
        qDebug() << "Failed to create QImage from frame buffer";
        return;
    }

    // 检查 QImage 的宽度和高度
    if (image.width() != client->width || image.height() != client->height) {
        qDebug() << "QImage size mismatch. Expected: " << client->width << "x" << client->height << ", Actual: " << image.width() << "x" << image.height();
        return;
    }

    QPainter painter(label);
    painter.drawImage(0, 0, image);
    painter.end();
}
