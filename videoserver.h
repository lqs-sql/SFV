//#ifndef VIDEOSERVER_H
//#define VIDEOSERVER_H

//#include <QObject>
//#include <QTcpServer>
//#include <QTcpSocket>
//#include <QImage>
//#include <QBuffer>
//#include <QTimer>
//#include <QThread>


//extern "C" {
//    #include <libavformat/avformat.h>
//    #include <libavcodec/avcodec.h>
//    #include <libswscale/swscale.h>
//    #include <libavutil/imgutils.h>  // 添加这个头文件包含
//}
//// 自定义的视频服务器类，继承自 QTcpServer
//class VideoServer : public QTcpServer {
//    Q_OBJECT
//public:
//    // 构造函数，接收一个可选的父对象指针
//    explicit VideoServer(QObject *parent = nullptr);
//    // 析构函数
//    ~VideoServer();

//protected:
//    // 重写 QTcpServer 的 incomingConnection 方法，处理新的客户端连接
//    void incomingConnection(qintptr socketDescriptor) override;

//private slots:
//    // 定时器触发的槽函数，用于发送视频帧
//    void sendFrame();

//private:
//    // 视频文件的格式上下文
//    AVFormatContext *formatContext = nullptr;
//    // 编解码器上下文
//    AVCodecContext *codecContext = nullptr;
//    // 视频帧对象
//    AVFrame *frame = nullptr;
//    // RGB 格式的视频帧对象
//    AVFrame *frameRGB = nullptr;
//    // 缓冲区
//    uint8_t *buffer = nullptr;
//    // 图像转换上下文
//    SwsContext *swsContext = nullptr;
//    // 存储所有连接的客户端套接字
//    QList<QTcpSocket *> sockets;
//    // 定时器对象
//    QTimer *timer;
//};

//// 服务器线程类，继承自 QThread
//class ServerThread : public QThread {
//    Q_OBJECT
//public:
//    // 重写 QThread 的 run 方法，线程启动后会执行此方法
//    void run() override;
//};


//#endif // VIDEOSERVER_H
