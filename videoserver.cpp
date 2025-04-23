//#include "videoserver.h"
//#include <QDebug>




//extern "C" {
//    #include <libavformat/avformat.h>
//    #include <libavcodec/avcodec.h>
//    #include <libswscale/swscale.h>
//    #include <libavutil/imgutils.h>  // 添加这个头文件包含
//}
//VideoServer::VideoServer(QObject *parent) : QTcpServer(parent) {
//    // 初始化网络，以便可以处理网络视频流
//    avformat_network_init();

//    // 打开视频文件，需要将路径替换为实际的视频文件路径
//    if (avformat_open_input(&formatContext, "path/to/video.mp4", nullptr, nullptr) != 0) {
//        return;
//    }

//    // 读取视频文件的流信息
//    if (avformat_find_stream_info(formatContext, nullptr) < 0) {
//        // 关闭视频文件
//        avformat_close_input(&formatContext);
//        return;
//    }

//    // 视频流的索引
//    int videoStreamIndex = -1;
//    // 视频流的编解码参数
//    AVCodecParameters *codecParameters = nullptr;
//    // 遍历所有的流，找到视频流
//    for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
//        if (formatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
//            videoStreamIndex = i;
//            codecParameters = formatContext->streams[i]->codecpar;
//            break;
//        }
//    }

//    // 如果没有找到视频流，关闭视频文件
//    if (videoStreamIndex == -1) {
//        avformat_close_input(&formatContext);
//        return;
//    }

//    // 根据编解码参数查找对应的编解码器
//    const AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);
//    if (!codec) {
//        avformat_close_input(&formatContext);
//        return;
//    }

//    // 分配编解码器上下文
//    codecContext = avcodec_alloc_context3(codec);
//    // 将编解码参数复制到编解码器上下文中
//    if (avcodec_parameters_to_context(codecContext, codecParameters) < 0) {
//        // 释放编解码器上下文
//        avcodec_free_context(&codecContext);
//        avformat_close_input(&formatContext);
//        return;
//    }

//    // 打开编解码器
//    if (avcodec_open2(codecContext, codec, nullptr) < 0) {
//        avcodec_free_context(&codecContext);
//        avformat_close_input(&formatContext);
//        return;
//    }

//    // 分配视频帧对象
//    frame = av_frame_alloc();
//    // 分配 RGB 格式的视频帧对象
//    frameRGB = av_frame_alloc();
//    if (!frame || !frameRGB) {
//        // 释放帧对象
//        av_frame_free(&frame);
//        av_frame_free(&frameRGB);
//        avcodec_free_context(&codecContext);
//        avformat_close_input(&formatContext);
//        return;
//    }

//    // 计算 RGB 格式视频帧所需的缓冲区大小
//    int numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, codecContext->width, codecContext->height, 1);
//    // 分配缓冲区
//    buffer = (uint8_t *)av_malloc(numBytes * sizeof(uint8_t));
//    // 将缓冲区填充到 RGB 帧对象中
//    av_image_fill_arrays(frameRGB->data, frameRGB->linesize, buffer, AV_PIX_FMT_RGB24, codecContext->width, codecContext->height, 1);

//    // 创建图像转换上下文，用于将视频帧转换为 RGB 格式
//    swsContext = sws_getContext(codecContext->width, codecContext->height,
//                                codecContext->pix_fmt, codecContext->width, codecContext->height,
//                                AV_PIX_FMT_RGB24, SWS_BILINEAR, nullptr, nullptr, nullptr);

//    // 创建定时器对象
//    timer = new QTimer(this);
//    // 连接定时器的 timeout 信号到 sendFrame 槽函数
//    connect(timer, &QTimer::timeout, this, &VideoServer::sendFrame);
//    // 启动定时器，每隔 33 毫秒触发一次，约 30 FPS
//    timer->start(33);
//}

//VideoServer::~VideoServer() {
//    // 释放图像转换上下文
//    sws_freeContext(swsContext);
//    // 释放缓冲区
//    av_free(buffer);
//    // 释放 RGB 帧对象
//    av_frame_free(&frameRGB);
//    // 释放帧对象
//    av_frame_free(&frame);
//    // 释放编解码器上下文
//    avcodec_free_context(&codecContext);
//    // 关闭视频文件
//    avformat_close_input(&formatContext);
//    // 反初始化网络
//    avformat_network_deinit();
//}

//void VideoServer::incomingConnection(qintptr socketDescriptor) {
//    // 创建一个新的 QTcpSocket 对象
//    QTcpSocket *socket = new QTcpSocket(this);
//    // 设置套接字描述符
//    socket->setSocketDescriptor(socketDescriptor);
//    // 将新的套接字添加到套接字列表中
//    sockets.append(socket);
//}

//void VideoServer::sendFrame() {
//    // 存储视频数据包
//    AVPacket packet;
//    // 循环读取视频文件中的数据包
//    while (av_read_frame(formatContext, &packet) >= 0) {
//        // 将数据包发送到编解码器进行解码
//        avcodec_send_packet(codecContext, &packet);
//        // 从编解码器中接收解码后的视频帧
//        while (avcodec_receive_frame(codecContext, frame) == 0) {
//            // 将视频帧转换为 RGB 格式
//            sws_scale(swsContext, (const uint8_t * const *)frame->data, frame->linesize, 0,
//                      codecContext->height, frameRGB->data, frameRGB->linesize);

//            // 创建 QImage 对象，方便在 Qt 中处理
//            QImage image(frameRGB->data[0], codecContext->width, codecContext->height, QImage::Format_RGB888);
//            // 用于存储图像数据的字节数组
//            QByteArray byteArray;
//            // 创建 QBuffer 对象，用于将图像数据写入字节数组
//            QBuffer buffer(&byteArray);
//            // 以写入模式打开缓冲区
//            buffer.open(QIODevice::WriteOnly);
//            // 将图像以 JPEG 格式保存到缓冲区
//            image.save(&buffer, "JPEG");

//            // 遍历所有连接的客户端套接字
//            for (QTcpSocket *socket : sockets) {
//                // 将图像数据发送给客户端
//                socket->write(byteArray);
//            }
//        }
//        // 释放数据包
//        av_packet_unref(&packet);
//    }
//}

//void ServerThread::run() {
//    VideoServer server;
//    // 开始监听指定的地址和端口
//    if (server.listen(QHostAddress::Any, 12345)) {
//        qDebug() << "Server started";
//    } else {
//        qDebug() << "Server could not start!";
//    }
//    // 进入线程的事件循环
//    exec();
//}
