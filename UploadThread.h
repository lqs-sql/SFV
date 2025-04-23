#ifndef UPLOADTHREAD_H
#define UPLOADTHREAD_H


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

class UploadThread : public QThread {
    Q_OBJECT
public:
    //在 C++ 里，如果要为参数设置默认值，必须从右向左连续设置。也就是说，一旦某个参数有了默认值，它右边的所有参数都必须有默认值。
    UploadThread( LIBSSH2_SFTP* sftp,QString& localPath,QString& remoteDirPath, bool stopFlag = false,QObject *parent = nullptr)
        : QThread(parent), sftp(sftp),localPath(localPath),remoteDirPath(remoteDirPath),stopFlag(stopFlag){}

    /**
     * 判断是否是文件
     * @brief isFile
     * @param fileName
     * @return
     */
    bool isFile(const QString &fileName) {
        QFileInfo info(fileName);
        return info.isFile();
    }

    /**
     * 判断是否是目录
     * @brief isDir
     * @param fileName
     * @return
     */
    bool isDir(const QString &fileName) {
        QFileInfo info(fileName);
        return info.isDir();
    }


    // 上传文件
       void uploadFile(const QString& localPath, const QString& remoteDirPath, LIBSSH2_SFTP* sftp) {
           qDebug() << "上传文件：" << localPath;
           // 打开本地文件
           QFile localFile(localPath);
           if (!localFile.open(QIODevice::ReadOnly)) {
               qDebug() << "Failed to open local file for reading：" << localPath;
               return;
           }

           // 获取文件总大小
           qint64 totalSize = localFile.size();
           qint64 uploadedSize = 0;

           // 构建远程临时文件路径
//           QString remotePath = remoteDirPath + "/" + StringUtil::extractFileNameOrDirNameByAbsolutePath(localPath) + ".tmp";
             QString remotePath = remoteDirPath + "/" + StringUtil::extractFileNameOrDirNameByAbsolutePath(localPath);
           // 打开远程文件进行写入
           LIBSSH2_SFTP_HANDLE* handle = libssh2_sftp_open(sftp, remotePath.toUtf8().constData(),
                                                           LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC, 0644);
           if (!handle) {
               qDebug() << "libssh2_sftp_open failed：" << remotePath;
               localFile.close();
               return;
           }

           // 缓冲区
           char buffer[1024];
           while (qint64 bytesRead = localFile.read(buffer, sizeof(buffer))) { // 直接使用返回值作为条件
               if (bytesRead < 0) { // 读取错误（如文件被删除）
                   qDebug() << "File read error";
                   break;
               }
               // 写入远程文件（使用正确的 bytesRead）
               libssh2_sftp_write(handle, buffer, static_cast<size_t>(bytesRead));
               uploadedSize += bytesRead;
               int progress = static_cast<int>((uploadedSize * 100) / totalSize);
               emit updateUploadProgressBar(progress, localPath,false);
           }

           // 关闭远程文件句柄
           libssh2_sftp_close(handle);

//           // 重命名远程文件(LIBSSH2_SFTP_RENAME_OVERWRITE是覆盖模式，会强制覆盖目标文件)
//           QString finalRemotePath = remoteDirPath + "/" + StringUtil::extractFileNameOrDirNameByAbsolutePath(localPath);
//           qDebug()<< "源路径："<<remotePath << "，目标路径："<<finalRemotePath;
//           int renameResult =libssh2_sftp_rename_ex(sftp, remotePath.toUtf8().constData() ,remotePath.toUtf8().length(), finalRemotePath.toUtf8().constData() ,finalRemotePath.toUtf8().length(),LIBSSH2_SFTP_RENAME_OVERWRITE);
//           if (renameResult != 0) {
//               qDebug() << "libssh2_sftp_rename_ex failed with error code: " << renameResult;
//               // 重命名失败，尝试覆盖文件内容
//               LIBSSH2_SFTP_HANDLE* tmpHandle = libssh2_sftp_open(sftp, remotePath.toUtf8().constData(), LIBSSH2_FXF_READ, 0);
//               if (!tmpHandle) {
//                   qDebug() << "Failed to open temporary file for reading: " << remotePath;
//               } else {
//                   LIBSSH2_SFTP_HANDLE* finalHandle = libssh2_sftp_open(sftp, finalRemotePath.toUtf8().constData(), LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC, 0644);
//                   if (!finalHandle) {
//                       qDebug() << "Failed to open final file for writing: " << finalRemotePath;
//                   } else {
//                       while (qint64 bytesRead = libssh2_sftp_read(tmpHandle, buffer, sizeof(buffer))) {
//                           if (bytesRead < 0) {
//                               qDebug() << "Error reading from temporary file";
//                               break;
//                           }
//                           libssh2_sftp_write(finalHandle, buffer, static_cast<size_t>(bytesRead));
//                       }
//                       libssh2_sftp_close(finalHandle);
//                   }
//                   libssh2_sftp_close(tmpHandle);
//               }
//               // 无论覆盖是否成功，都删除临时文件（避免残留）
//               int unlinkResult = libssh2_sftp_unlink(sftp, remotePath.toUtf8().constData());
//               if (unlinkResult != 0) {
//                   qDebug() << "Failed to delete temporary file: " << remotePath << ", error code: " << unlinkResult;
//               } else {
//                   qDebug() << "Temporary file deleted: " << remotePath;
//               }
//           }
           // 关闭本地文件
           localFile.close();
           emit updateUploadProgressBar(100, localPath,true);
       }

       // 递归上传目录
       void uploadDirectory(const QString& localDirPath, const QString& remoteDirPath, LIBSSH2_SFTP* sftp) {
           QDir localDir(localDirPath);
           if (!localDir.exists()) {
               qDebug() << "Local directory does not exist: " << localDirPath;
               return;
           }

           QString remotePath = remoteDirPath + "/" + localDir.dirName();
           // 检查远程目录是否存在
           LIBSSH2_SFTP_ATTRIBUTES attr;
           int statResult = libssh2_sftp_stat(sftp, remotePath.toUtf8().constData(), &attr);
           if (statResult == 0) {
//               // 目录存在，尝试删除
//               deleteRemoteDir(remotePath);
           }else{
           //目录不存在，在远程创建目录
               int rc = libssh2_sftp_mkdir(sftp, remotePath.toUtf8().constData(), 0755);
               if (rc != 0 && rc != LIBSSH2_FX_FILE_ALREADY_EXISTS) {//LIBSSH2_ERROR_SFTP_EEXIST是目录已存在的报错，允许目录已存在
                   qDebug() << "Failed to create remote directory: " << remotePath <<"，报错码："<<rc;
               }
           }

           // 计算目录总大小
           qint64 totalSize = calculateDirectorySize(localDirPath);
           qint64 uploadedSize = 0;

           // 遍历本地目录下的所有文件和子目录
           QFileInfoList entries = localDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
           for (const QFileInfo& entry : entries) {
               if (entry.isFile()) {
                   // 上传文件
                   QString localFilePath = entry.absoluteFilePath();
                   QString remoteFilePath = remotePath + "/" + entry.fileName();
                   QFile localFile(localFilePath);
                   if (!localFile.open(QIODevice::ReadOnly)) {
                       qDebug() << "Failed to open local file for reading: " << localFilePath;
                       continue;
                   }

                   // 打开远程文件进行写入
                   LIBSSH2_SFTP_HANDLE* handle = libssh2_sftp_open(sftp, remoteFilePath.toUtf8().constData(),
                                                                   LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC, 0644);
                   if (!handle) {
                       qDebug() << "libssh2_sftp_open failed: " << remoteFilePath;
                       localFile.close();
                       continue;
                   }

                   // 缓冲区
                   char buffer[3072];
                   while (qint64 bytesRead = localFile.read(buffer, sizeof(buffer))) { // 直接使用返回值作为条件
                       if (bytesRead < 0) { // 读取错误（如文件被删除）
                           qDebug() << "File read error";
                           break;
                       }
                       // 写入远程文件（使用正确的 bytesRead）
                       libssh2_sftp_write(handle, buffer, static_cast<size_t>(bytesRead));
                       uploadedSize += bytesRead;
                       int progress = static_cast<int>((uploadedSize * 100) / totalSize);


                      qDebug()<<"File文件，单次下载："<<bytesRead<<"，已经上传："<<uploadedSize<<"，总上传："<<totalSize<<"，计算比例："<<progress<<"，当前上传的文件："<<localFile;

                       emit updateUploadProgressBar(progress, localPath,false);
                   }


                   // 关闭远程文件句柄
                   libssh2_sftp_close(handle);

                   // 关闭本地文件
                   localFile.close();
               } else if (entry.isDir()) {
                   // 递归上传子目录（由于一开始计算了总比例，所以上传目录不需要发送目录进度更新）
                   uploadDirectory(entry.absoluteFilePath(), remotePath, sftp);
               }
           }
       }
       // 辅助函数：计算目录总大小
       qint64 calculateDirectorySize(const QString& dirPath) {
           QDir dir(dirPath);
           qint64 size = 0;
           QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
           for (const QFileInfo& entry : entries) {
               if (entry.isFile()) {
                   size += entry.size();
               } else if (entry.isDir()) {
                   size += calculateDirectorySize(entry.absoluteFilePath());
               }
           }
           return size;
       }
       // 辅助函数：删除目录
       void deleteRemoteDir(QString path){
           //libssh2_sftp_rmdir删除的目录必须为空
       //    int rc = libssh2_sftp_rmdir(this->currentSftp->sftp, path.toUtf8().constData());
           QSshSocket sshSocket;
           bool result = sshSocket.recursiveRemoveDir(sftp, path.toUtf8().constData(),true);
           if (result) {
               qDebug() << "删除远程目录成功: " << path;
           } else {
               QSshSocket sshSocket;
               qDebug() << "删除远程目录失败: " << path ;
           }
       }
signals:
    //发送更新进度条信号
    void updateUploadProgressBar(int progress,QString localPath,bool isUpdateRemote);

protected:
    //子线程的具体运行内容
    void run() override {
        if(isFile(localPath)){
            qDebug()<<"上传文件："<<localPath;
            uploadFile(localPath,remoteDirPath,sftp);
        }else if(isDir(localPath)){
            qDebug()<<"上传目录："<<localPath;
            uploadDirectory(localPath,remoteDirPath,sftp);
        }

        //下载执行完毕，发送信号
        emit updateUploadProgressBar(100,localPath,true);

    }
public:
    bool stopFlag; //while循环停止标志
    LIBSSH2_SFTP* sftp;
    QString localPath;
    QString remoteDirPath;
    int progress;
};





#endif // UPLOADTHREAD_H
