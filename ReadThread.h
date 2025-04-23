#ifndef READTHREAD_H
#define READTHREAD_H

#include "QSshSocket.h"
#include <QDebug>
#include "LoginUser.h"
#include "LoginSession.h"
#include "StringUtil.h"
#include <QThread>
#include <QUuid>
#include <cstring>

class ReadThread : public QThread {
    Q_OBJECT
public:
    //在 C++ 里，如果要为参数设置默认值，必须从右向左连续设置。也就是说，一旦某个参数有了默认值，它右边的所有参数都必须有默认值。
    ReadThread(LoginSession loginSession,QString signalStr, bool stopFlag = false,QObject *parent = nullptr)
        : QThread(parent), loginSession(loginSession),signalStr(signalStr),stopFlag(stopFlag){}

signals:
    //发送更新文件树当前所在路径的信号
    void updateFileTreeView(QString currentPath);
    //发送追加内容信号：index给哪个窗口发送、content发送的追加内容
    void appendContent(int index,QString content);

protected:
    //子线程的具体运行内容
    void run() override {
        // 获取子线程ID
        qint64 subThreadId = reinterpret_cast<qint64>(QThread::currentThreadId());


        while (!stopFlag) {

            Result<ExecuteCommandResult> result;
            // 在主线程中打印子线程ID
//            qDebug() << "循环读线程线程ID: " << subThreadId;
            // 读取命令输出
            char buffer[1024]; //接收管道数据的字节数组
            ssize_t byteCount; //记录管道数据的字节数
            int i = 1;
            while ((byteCount = libssh2_channel_read(loginSession.channel, buffer, sizeof(buffer))) > 0) {
                // 可以在这里处理读取到的登录信息，但不保存到最终结果中
                QString partOutput = QString::fromUtf8(buffer, byteCount);
//                qDebug()<<"读取内容，子线程id："<<subThreadId;
                qDebug() << "命令1第"+QString::number(i)+"次执行后读取：" << partOutput;
                if(partOutput.contains(signalStr)){//包括标志串，则进行文件路径变化监控
                    if(partOutput.trimmed().endsWith("]$") || partOutput.trimmed().contains("]#")){
                        QString currentPath = StringUtil::extractPathByPartOutput(partOutput,signalStr);//筛选出当前所在路径
                    qDebug()<<"当前所在路径**********："<<currentPath;
                    partOutput = partOutput.replace(signalStr,"");
                    partOutput = partOutput.replace(currentPath,"");
                    //echo "  $(pwd) ";
                    partOutput = partOutput.replace("echo","");
                    partOutput = partOutput.replace("\"","");
                    partOutput = partOutput.replace("$(pwd)","");
                    partOutput = partOutput.replace(";","");
                    partOutput = StringUtil::deleteRedundantUserStr(partOutput,currentPath);
                    qDebug()<<"删除后的;"<<partOutput;
                    emit updateFileTreeView(currentPath);
                    if(StringUtil::isNotBlank(partOutput)){
                        emit appendContent(loginSession.index,partOutput);
                    }
                    }
                }else{
                    emit appendContent(loginSession.index,partOutput);
                    i++;
                }
            }
            // 可以在这里添加适当的延迟，避免过于频繁地读取
            this->msleep(100); // 暂停 100 毫秒
//            qDebug()<<"读取结果："<<byteCount;
        }
//        qDebug() << "测试内容 ";
    }
public:
    bool stopFlag; //while循环停止标志
    LoginSession loginSession; //操作的loginSession
    QString signalStr; //监控文件路径变化的标志符号字符串
};


#endif // READTHREAD_H
