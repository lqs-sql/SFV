#ifndef VNC_SERVER_H
#define VNC_SERVER_H

// vnc_server.h
#include <QObject>
#include <rfb/rfb.h>

class VncServer : public QObject {
    Q_OBJECT
public:
    explicit VncServer(QObject *parent = nullptr);
    ~VncServer();
    bool startServer(int port = 5900);

private:
    rfbScreenInfoPtr m_server;
    static void clientHook(rfbClientPtr client);
};

#endif // VNC_SERVER_H
