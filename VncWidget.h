#ifndef VNCCLIENT_H
#define VNCCLIENT_H

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <rfb/rfbclient.h>

class VncWidget : public QWidget
{
    Q_OBJECT
public:
    VncWidget(QWidget *parent = nullptr);
    ~VncWidget();

private:
    static signed char mallocFrameBuffer(rfbClient* client);
    static void gotFrameBufferUpdate(rfbClient* client, int x, int y, int w, int h);
    rfbClient* client;
    QLabel *label;
    void updateImage(const rfbClient* client, int x, int y, int w, int h);
};

#endif // VNCCLIENT_H
