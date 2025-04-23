#ifndef DOWNLOADTHREAD_H
#define DOWNLOADTHREAD_H

#include "QSshSocket.h"
#include "LoginUser.h"
#include "LoginSession.h"
#include "StringUtil.h"
#include <QThread>
#include <QUuid>
#include <cstring>
#include <QFileIconProvider>
#include <QDir>
#include <QDebug>
#include <QObject>
#include <QFileInfo>
#include <QFile>

class DownloadThread : public QThread {
    Q_OBJECT
public:
    //在 C++ 里，如果要为参数设置默认值，必须从右向左连续设置。也就是说，一旦某个参数有了默认值，它右边的所有参数都必须有默认值。
    DownloadThread( LIBSSH2_SFTP* sftp,QString& remotePath,QString& localPath, bool stopFlag = false,QObject *parent = nullptr)
        : QThread(parent), sftp(sftp),remotePath(remotePath),localPath(localPath),stopFlag(stopFlag){}
    /**
     * @brief download
     * @param sftp
     * @param remotePath
     * @param localPath
     * @param stopFlag
     */
    void download(LIBSSH2_SFTP* sftp, const QString& remotePath, const QString& localPath, bool& stopFlag,qint64 allDownloadSize,qint64 alreadyDownloadSize){
        // 获取子线程ID
        qint64 subThreadId = reinterpret_cast<qint64>(QThread::currentThreadId());



        QSshSocket sshSocket;
        if (sshSocket.isFile(sftp,remotePath)) {//选中文件
            // 检查是否需要断点续传
            qint64 resumeOffset = 0;
            QFile localFile(localPath);
            if (localFile.exists()) {
                resumeOffset = localFile.size(); //以本地文件大小作为断点续传的索引
            }

//            // 获取远程文件大小
//             LIBSSH2_SFTP_ATTRIBUTES attrs;
//             if (libssh2_sftp_stat(sftp, remotePath.toUtf8().constData(), &attrs) != 0) {
//                 qDebug() << "Failed to get remote file size:" << remotePath;
//                 return ;
//             }
//             qint64 totalSize = attrs.filesize;

            // 打开远程文件
            LIBSSH2_SFTP_HANDLE* sftp_handle = libssh2_sftp_open(sftp, remotePath.toUtf8().constData(), LIBSSH2_FXF_READ, LIBSSH2_SFTP_S_IRUSR|LIBSSH2_SFTP_S_IRGRP|LIBSSH2_SFTP_S_IROTH);
            if (!sftp_handle) {
                qDebug() << "Failed to open remote file:" << remotePath;
                return ;
            }

            // 定位到断点位置
            if (resumeOffset > 0) {
                //将远程文件指针移到resumeOffset处
                libssh2_sftp_seek64(sftp_handle, resumeOffset);
            }

            // 检查文件所在目录是否存在，不存在则递归创建
    //        qDebug()<<"本地文件路径："<<localPath;
            QFileInfo fileInfo(localPath);
            QDir dir = fileInfo.dir();
            if (!dir.exists()) {
                if (!dir.mkpath(dir.absolutePath())) {
                    qDebug() << "Failed to create directory:" << dir.path();
                    libssh2_sftp_close(sftp_handle);
                    return ;
                }
            }

            // 打开本地文件（以追加模式打开用于断点续传，本地文件指针会被移动到末尾处）
            if (!localFile.open(QIODevice::Append)) {
                qDebug() << "Failed to open or create local file:" << localPath << "Error:" << localFile.errorString();
                libssh2_sftp_close(sftp_handle);
                return ;
            }

            //本地已下载字节数
            qint64 downloadedSize = resumeOffset;

            qDebug()<<"当前已下载或者说已经存在本地文件的字节数："<<downloadedSize;
            qDebug()<<"远程文件字节数："<<allDownloadSize;
            if(downloadedSize == allDownloadSize){ //当前文件已下载完成，结束下载。
                return ;
            }

            // 下载文件内容
            char buffer[1024];
            ssize_t bytes_read;
            while ((bytes_read = libssh2_sftp_read(sftp_handle, buffer, sizeof(buffer))) > 0) {
                if (stopFlag) {
                    qDebug() << "Download stopped by user.";
                    break;
                }
                //将读取到内容写入本地文件
                qint64 bytes_written = localFile.write(buffer, bytes_read);
                /**
                 * bytes_read 是通过 libssh2_sftp_read 函数从远程文件读取的数据字节数，bytes_written是本地文件当前数据字节数
                 * */
                if (bytes_written != bytes_read) {
                    qDebug() << "Failed to write to local file.";
                    localFile.close();
                    libssh2_sftp_close(sftp_handle);
                    return ;
                }

                // 更新本地已下载字节数
                downloadedSize += bytes_read;
                // 计算当前进度
//                progress = static_cast<int>((downloadedSize * 100) / totalSize);
                alreadyDownloadSize = alreadyDownloadSize + bytes_read;
                progress = static_cast<int>((alreadyDownloadSize * 100) / allDownloadSize);
//               qDebug()<<"File文件，单次下载："<<bytes_read<<"，已经下载："<<alreadyDownloadSize<<"，总下载："<<allDownloadSize<<"，计算比例："<<progress<<"，当前下载的文件："<<remotePath;
                emit updateDownloadProgressBar(progress,localPath,false);

            }
            qDebug()<<"下载结束2";

            // 关闭本地文件和远程文件
            localFile.close();
            libssh2_sftp_close(sftp_handle);

    //        qDebug() << "测试内容 ";
        }else if(sshSocket.isDir(sftp,remotePath)){//选中目录
             qDebug()<<"双击目录路径："<<remotePath;
             // 是目录，递归下载目录下的所有文件和子目录
             QDir localDir(localPath);
             if (!localDir.exists()) {
                 if (!localDir.mkpath(localDir.absolutePath())) {
                     qDebug() << "Failed to create directory:" << localDir.path();
                     return;
                 }
             }

             // 计算远程目录下所有文件的总大小
             qint64 totalDirSize = 0;
             calculateTotalDirOrFileSize(sftp, remotePath, totalDirSize);

             // 已下载的目录总字节数
             qint64 totalDownloadedSize = 0;


             LIBSSH2_SFTP_HANDLE* sftp_handle = libssh2_sftp_opendir(sftp, remotePath.toUtf8().constData());
             if (!sftp_handle) {
                 qDebug() << "Failed to open remote directory:" << remotePath;
                 return;
             }

             LIBSSH2_SFTP_ATTRIBUTES item_attrs;
             char filename[512];
             char buffer[1024]; // 为长名称分配一个足够大的缓冲区
             while (libssh2_sftp_readdir_ex(sftp_handle, filename, sizeof(filename), buffer,sizeof(buffer), &item_attrs) > 0) {
                 if (stopFlag) {
                     qDebug() << "Download stopped by user.";
                     break;
                 }
                 if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
                     continue;
                 }
                 QString childRemotePath = remotePath + "/" + QString::fromUtf8(filename);
                 QString childLocalPath = localPath + "/" + QString::fromUtf8(filename);

                 //递归下载
                 download(sftp, childRemotePath, childLocalPath, stopFlag,allDownloadSize,alreadyDownloadSize);


                 // 获取子文件或子目录的大小
//                if (libssh2_sftp_stat(sftp, childRemotePath.toUtf8().constData(), &item_attrs) == 0) {
//                    totalDownloadedSize += item_attrs.filesize;
//                }
                 calculateTotalDirOrFileSize(sftp,childRemotePath,totalDownloadedSize);

                // 计算目录下载进度
//                int  dirProgress = static_cast<int>((totalDownloadedSize * 100) / totalDirSize);
                alreadyDownloadSize = alreadyDownloadSize + totalDownloadedSize;
                int  dirProgress = static_cast<int>((alreadyDownloadSize * 100) / allDownloadSize);
//                qDebug()<<"Dir目录，单次下载："<<totalDownloadedSize<<"，已经下载："<<alreadyDownloadSize<<"，总下载："<<allDownloadSize<<"，计算比例："<<dirProgress<<"，当前下载的目录："<<remotePath;
                //发送进度更新
                emit updateDownloadProgressBar(dirProgress, localPath,false);
             }
             libssh2_sftp_closedir(sftp_handle);
        }
    }
    // 递归计算远程目录（目录下所有文件的总大小）/远程文件的总大小
    void calculateTotalDirOrFileSize(LIBSSH2_SFTP* sftp, const QString& remotePath, qint64& totalSize) {
        LIBSSH2_SFTP_ATTRIBUTES attrs;
        if (libssh2_sftp_stat(sftp, remotePath.toUtf8().constData(), &attrs) != 0) {
            return;
        }

        if (LIBSSH2_SFTP_S_ISDIR(attrs.permissions)) {
            LIBSSH2_SFTP_HANDLE* sftp_handle = libssh2_sftp_opendir(sftp, remotePath.toUtf8().constData());
            if (!sftp_handle) {
                return;
            }

            LIBSSH2_SFTP_ATTRIBUTES item_attrs;
            char filename[512];
            char buffer[1024];
            while (libssh2_sftp_readdir_ex(sftp_handle, filename, sizeof(filename), buffer, sizeof(buffer), &item_attrs) > 0) {
                if (strcmp(filename, ".") == 0 || strcmp(filename, "..") == 0) {
                    continue;
                }
                QString childRemotePath = remotePath + "/" + QString::fromUtf8(filename);
                calculateTotalDirOrFileSize(sftp, childRemotePath, totalSize);
            }

            libssh2_sftp_closedir(sftp_handle);
        } else {
            totalSize += attrs.filesize;
        }
    }
    //递归计算本地目录大小
    qint64 calculateLocalDirectorySize(const QString &path) {
        QDir dir(path);
        qint64 size = 0;

        // 遍历目录中的所有文件和子目录
        QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QFileInfo &entry : entries) {
            if (entry.isFile()) {
                // 如果是文件，累加文件大小
                size += entry.size();
            } else if (entry.isDir()) {
                // 如果是目录，递归调用函数计算子目录的大小
                size += calculateLocalDirectorySize(entry.absoluteFilePath());
            }
        }

        return size;
    }
signals:
    //发送更新进度条信号
    void updateDownloadProgressBar(int progress,QString localPath,bool isOpen);

protected:
    //子线程的具体运行内容
    void run() override {

        QSshSocket sshSocket;
        qint64 allDownloadSize = -1;
        qint64 alreadyDownloadSize = 0;
        //先计算本次下载的文件/目录总大小，赋值给allDownloadSize
        calculateTotalDirOrFileSize(sftp,remotePath,allDownloadSize);
        //计算本地目录下已下载的子目录/子文件总字节数
        alreadyDownloadSize = calculateLocalDirectorySize(localPath);
        //递归下载
        download(this->sftp,remotePath,localPath,stopFlag,allDownloadSize,alreadyDownloadSize);

        //下载执行完毕，发送信号
        emit updateDownloadProgressBar(100,localPath,true);

    }
public:
    bool stopFlag; //while循环停止标志
    LIBSSH2_SFTP* sftp;
    QString remotePath;
    QString localPath;
    int progress;
};



#endif // DOWNLOADTHREAD_H
