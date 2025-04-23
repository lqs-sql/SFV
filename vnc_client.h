#ifndef VNC_CLIENT_H
#define VNC_CLIENT_H

// vnc_client.h
#include <QWidget>
#include <QLabel>
#include <rfb/rfbclient.h>

class VncClient : public QWidget {
    Q_OBJECT
public:
    explicit VncClient(QWidget *parent = nullptr);
    void connectToServer(const char* host = "127.0.0.1", int port = 5900);

private slots:
    void updateFrame();

private:
    QLabel *m_screenLabel;
    rfbClient* m_client;
    QImage m_image;

    static void onUpdate(rfbClient* client, int x, int y, int w, int h);
    static rfbBool resizeWindow(rfbClient* client);
};

#endif // VNC_CLIENT_H
