#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTreeView>
#include <QFileSystemModel>
#include <QMenu>
#include "TreeNode.h"
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "LoginSession.h"
#include "QSshSocket.h"
#include "SshCmd.h"
#include "SshCmdResult.h"
#include "CustomizeCmdTextEdit.h"
#include "SftpSession.h"

#include "notepad.h"
#include "VNCServerThread.h"
#include "VNCClientThread.h"

#include <QMetaType>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE


// 声明该类型为元类型
Q_DECLARE_METATYPE(LoginSession)


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    // 递归获取QTreeView里的节点的路径
    QString getPath(QStandardItem *item);

    void tabCountChange();

    QTreeWidgetItem* convertTreeNodeToTreeWidgetItem(TreeNode* node, QTreeWidget* treeWidget);

    void showContextMenu(const QPoint &pos);
    void showRemoteContextMenu(const QPoint &pos);
    void createFile();
    void createDir();
    void deleteFileOrDir();
    void clearDir();
    void modifyFileName();
    void openFile();
    void copyFile();
    void uploadFile();
    void uploadDir();

    void copyRemoteFile(QString path);
    //删除远程文件
    void deleteRemoteFile(QString path);
    void modifyRemoteFileName(QString path);
    void openRemoteDirOrFile(QString path);
    void downloadRemoteDirOrFile(QString path);
    void createRemoteFile(QString path);
    void clearRemoteDir(QString path);
    void createRemoteDir(QString path);
    void deleteRemoteDir(QString path);
    void modifyRemoteDirName(QString path);
    void doubleClickOpenRemoteDirOrFile();


    //构造对应文件树（递归子方法）
    void traverseRemoteDirectory(LIBSSH2_SESSION *session,LIBSSH2_SFTP* sftp, const QString& path, QStandardItem* parentItem);

    //构造对应文件树
    void getPathQTreeView(LIBSSH2_SESSION *session,LIBSSH2_SFTP* sftp, const QString& path,QTreeView* treeView);

    //连接菜单的双击事件
    void connectTreeNodeDoubleClick(QTreeWidgetItem *item, int column);


    //发送一个标识符pwd命令
    void sendPwdCommand(LoginSession loginSession);


    //在index索引处添加一个新标签
    CustomizeCmdTextEdit* addTab(int index,QString tabName);
    //获取index索引处标签操作对象
    CustomizeCmdTextEdit* getTab(int index);


    //终止读线程
    void deleteReadThread(int index);
    //重新创建读线程（直接创建赋值）
    void createReadThread();
    //命令行窗口ctrl+c事件处理，重置当前读线程（先销毁后创建）
    void resetReadThread();

    //终止写线程
    void deleteWriteThread();
    //重新创建写线程（直接创建赋值）
    void createWriteThread();
    //重置写线程（先销毁后创建）
    void resetWriteThread();

    //重置光标判断（如果是当前窗口则设置光标到最后，否则则不）
    bool setTextEditCursor(int index);

    //移动index之后索引的元素向前一个索引值
    void mapIndexSubtractOne(int index);

    // 重写事件过滤器函数，用于处理特定事件
    bool eventFilter(QObject *target, QEvent *event) override;
public slots:
    //文件树里文件/目录更名信号
    void itemChanged(QStandardItem *item);

    //子线程进行ssh登录后进行的插槽方法
    void loginUpdateUI(LoginSession loginSession,
                       bool loginStatus);
    // 自定义槽函数，用于关闭指定索引的标签页
    void closeTab(int index);
    void clickTab(int index);
    void tabSwitch(int index);//标签切换
    //命令行窗口回车事件处理
    void customizeCmdTextEditEnterPressHandle();
    //给当前索引处的标签对应QTextEdit追加内容
    void appendContent(int index,QString content);
    //重连channel
    void resetChannel();

    //当前路径更新，更新对应的文件树
    void updateFileTreeView(QString currentPath);

    void updateSftpSession(SftpSession* sftpSession);

    //接收到notepad的保存信号，将内容上传远程服务器
    void uploadFileByLocalPath(QString localPath);
    //更新进度条信号
    void updateDownloadProgressBar(int progress,QString localPath,bool isOpen);
    void updateUploadProgressBar(int progress,QString localPath,bool isUpdateRemote);

//    //绘制屏幕图像
//    void renderScreen(const QImage& image);
private:
    //存放每个ssh连接与其对应信息，key是连接的用户信息，value是一个Map（key是执行的cmd命令对象，value是执行cmd命令后的返回结果对象）
    QMap<int,LoginSession> sshConnectMap;
    Ui::MainWindow *ui;
    QTreeView *fileTreeView = nullptr ; //远程文件树
    QFileSystemModel *fileSystemModel = nullptr ; //远程文件树对应的文件模型

    QTreeView *localFileTreeView = nullptr ; //本地文件树
    QFileSystemModel *localFileSystemModel = nullptr ; //本地文件树对应的文件模型


    QString signalStr; //监控文件路径变化的标志符号字符串

    SftpSession* currentSftp;//当前文件树对应的sftp值
    QString currentPath; //当前所在远程目录的路径

    QScopedPointer<Notepad> notepad;//

//    VNCServerThread* vncServerThread;//测试的vnc服务端对象

//    VNCClientThread* vncClientThread;//当前连接的vnc客户端对象
};
#endif // MAINWINDOW_H
