#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QLabel>
#include <QTabBar>
#include <QDebug>
#include <QMouseEvent>
#include <QStandardPaths>
#include <FileUtil.cpp>
#include <TreeUtil.cpp>
#include <QInputDialog>
#include <StringUtil.h>
#include <QDesktopServices>
#include <QTreeWidgetItem>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>
#include <QFileDialog>

#include <QTextEdit>
#include "commandwindow.h"
#include "CustomizeCmdTextEdit.h"
#include "QSshSocket.h"
#include <QUuid>
#include "LoginSession.h"
#include <QThread>
#include "WorkThread.h"
#include <UIUtil.cpp>
#include "QSshSocket.h"
#include "Result.h"
#include "ExecuteCommandResult.h"
#include "ReadThread.h"
#include "MonitorPathChangeThread.h"
#include "SftpConnectThread.h"
#include "DownloadThread.h"
#include "UploadThread.h"

#include <Global.h>

#include "notepad.h"


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow) {
    ui->setupUi(this);
    //设置监控路径的标志符号字符串
    signalStr = "--**()--";//信号字符串，用来识别监控文件路径变化命令
    //设置spiltter里的控件比例（占五个单位）
    ui->splitter->setSizes(QList<int>() << width() / 16 * 3 << width() / 16 * 4 << width() / 16 * 3);
//    //设置QTabWidget的标签可被关闭
//    ui->SshTabWidget->setTabsClosable(true);
    // 连接 tabCloseRequested 信号到自定义槽函数
    connect(ui->SshTabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeTab);
    // 连接 tabBarClicked 信号到自定义槽函数
    connect(ui->SshTabWidget, &QTabWidget::tabBarClicked, this, &MainWindow::clickTab);
    // 连接 currentChanged 信号到自定义槽函数
    connect(ui->SshTabWidget, &QTabWidget::currentChanged, this, &MainWindow::tabSwitch);
    //开启所有标签页的可关闭按钮
    ui->SshTabWidget->setTabsClosable(true);
    //设置某个标签可关闭
    QTabBar *tabBar = ui->SshTabWidget->tabBar();
    tabBar->setTabButton(0, QTabBar::RightSide, NULL); // 移除索引为0位置的标签的关闭按钮
    tabBar->setTabButton(ui->SshTabWidget->count() - 1, QTabBar::RightSide, NULL); // 移除索引最后一个位置的标签的关闭按钮
    //开启索引标签页的可移动
    ui->SshTabWidget->setMovable(true);
    //使用原生标签移动tabMoved信号，触发自定义槽函数
    connect(ui->SshTabWidget->tabBar(), &QTabBar::tabMoved, this, [this](int to, int from) {
        qDebug() << "to：" << to << "，from：" << from;
        if (to == 0 || to == ui->SshTabWidget->count() - 1) {
            // 移动后的索引为 0，将标签移回原来的位置
            ui->SshTabWidget->tabBar()->moveTab(from, to);
        }
    });


    // 获取第一个标签页的 QWidget
    QWidget *firstTabWidget = ui->RemoteTabWidget->widget(0);
    // 创建文件树视图
    fileTreeView = new QTreeView(firstTabWidget);
    fileSystemModel = new QFileSystemModel(this);
    fileTreeView->setModel(fileSystemModel);
    //设置第一列宽度
    fileTreeView->setColumnWidth(0, 150);
   // 设置布局
    QVBoxLayout *remoteLayout = new QVBoxLayout(firstTabWidget);
    remoteLayout->addWidget(fileTreeView);
    firstTabWidget->setLayout(remoteLayout);
    // 设置右键菜单策略为允许自定义右键菜单
    fileTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(fileTreeView, &QTreeView::customContextMenuRequested, this, &MainWindow::showRemoteContextMenu);
    //文件树双击文件信号
    connect(fileTreeView, &QTreeView::doubleClicked, this, &MainWindow::doubleClickOpenRemoteDirOrFile);


    // 获取第二个标签页的 QWidget （本地文件树）
    QWidget *localFileTabWidget = ui->ConfigTabWidget->widget(1);
    // 创建文件树视图
    localFileTreeView = new QTreeView(localFileTabWidget);
    localFileSystemModel = new QFileSystemModel(this);
    qDebug() << "根路径（Linux下是/，windows下是C：）：" << QDir::rootPath();
    // 获取当前用户的桌面路径
    QString desktopPath = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation);
    qDebug() << "桌面路径：" << desktopPath;
    //设置树的根路径
    localFileSystemModel->setRootPath(desktopPath);
    localFileTreeView->setModel(localFileSystemModel);
    localFileTreeView->setRootIndex(localFileSystemModel->index(desktopPath));
    //设置第一列宽度
    localFileTreeView->setColumnWidth(0, 150);
    // 设置布局
    QVBoxLayout *layout = new QVBoxLayout(localFileTabWidget);
    layout->addWidget(localFileTreeView);
    localFileTabWidget->setLayout(layout);

    // 设置右键菜单策略为允许自定义右键菜单
    localFileTreeView->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(localFileTreeView, &QTreeView::customContextMenuRequested, this, &MainWindow::showContextMenu);
    //文件树双击文件信号
    connect(localFileTreeView, &QTreeView::doubleClicked, this, &MainWindow::openFile);




    //文件树安装对应的事件过滤器
    ui->remoteFileTab->installEventFilter(this);
    //添加事件过滤器
    ui->SshTabWidget->installEventFilter(this);
    ui->SshTabWidget->setFocus(); //获取焦点，只有当前焦点所在位置才能触发对应的事件过滤器
    // 为 QTabWidget 中已经存在的每个 QWidget 安装事件过滤器，让其点击后可以获取到焦点
    for (int i = 0; i < ui->SshTabWidget->count(); ++i) {
        QWidget *widget = ui->SshTabWidget->widget(i);
        if (widget) {
            widget->installEventFilter(this);
        }
    }
    // 为 QTabWidget 中已经存在的每个 QWidget 安装事件过滤器，让其点击后可以获取到焦点
    for (int i = 0; i < ui->RemoteTabWidget->count(); ++i) {
        QWidget *widget = ui->RemoteTabWidget->widget(i);
        if (widget) {
            widget->installEventFilter(this);
        }
    }
    //QTreeWidget
    // 设置显示的列数为1
    ui->connectTreeWidget->setColumnCount(1);
    // 清除表头(不然会有一个1值显示在上方)
    ui->connectTreeWidget->setHeaderHidden(true);
    // 设置QTreeWidget的属性
    ui->connectTreeWidget->setEditTriggers(QAbstractItemView::EditKeyPressed); // 可通过F2编辑
    ui->connectTreeWidget->setSelectionMode(QAbstractItemView::SingleSelection); // 可选择
    ui->connectTreeWidget->setDragEnabled(true); // 可拖动
    ui->connectTreeWidget->setAcceptDrops(true); // 可接受拖放
    ui->connectTreeWidget->setDefaultDropAction(Qt::MoveAction);
    ui->connectTreeWidget->setItemsExpandable(true);
    ui->connectTreeWidget->setRootIsDecorated(true);
    // 移除可能导致问题的这一行
    // ui->connectTreeWidget->setCheckState(0, Qt::Unchecked);
    ui->connectTreeWidget->setEnabled(true); // 可启用


    // 定义图标路径
    QString iconPath = ":/icon/";
    // 加载图标
    QIcon userIcon(iconPath + "folder.png"); // 一级菜单图标
    QIcon ipIcon(iconPath + "folder.png");   // 二级菜单图标
    // 创建一级菜单项目 user1
    QTreeWidgetItem *user1Item = new QTreeWidgetItem(ui->connectTreeWidget);
    user1Item->setText(0, "user1");
    user1Item->setIcon(0, userIcon);
    user1Item->setFlags(user1Item->flags() | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
    // 为 user1 添加二级菜单项目 192.168.168.128:22
    QTreeWidgetItem *ipItem1 = new QTreeWidgetItem(user1Item);
    ipItem1->setText(0, "192.168.168.128:22");
    ipItem1->setIcon(0, ipIcon);
    ipItem1->setFlags(ipItem1->flags() | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
    // 创建一级菜单项目 user2
    QTreeWidgetItem *user2Item = new QTreeWidgetItem(ui->connectTreeWidget);
    user2Item->setText(0, "user2");
    user2Item->setIcon(0, userIcon);
    user2Item->setFlags(user2Item->flags() | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
    // 为 user2 添加二级菜单项目 192.168.168.128:22
    QTreeWidgetItem *ipItem2 = new QTreeWidgetItem(user2Item);
    ipItem2->setText(0, "192.168.168.128:22");
    ipItem2->setIcon(0, ipIcon);
    ipItem2->setFlags(ipItem2->flags() | Qt::ItemIsEditable | Qt::ItemIsUserCheckable);
    // 设置样式表以去除水平分割线
    ui->connectTreeWidget->setStyleSheet("QTreeWidget{"
                                         "border: none;"
                                         "}");
    //设置QTreeWidget树形窗口里的节点的双击信号事件
    connect(ui->connectTreeWidget, &QTreeWidget::itemDoubleClicked, this, &MainWindow::connectTreeNodeDoubleClick);
    // 从指定路径读取图标和文件名进行显示
    // 构建文件树
    TreeNode *root = TreeUtil::buildDirTree("D:/1");
    TreeUtil::dfs(root);
    TreeUtil::bfs(root);
    qDebug() << "层数：" << TreeUtil::getTreeDepth(root);
    if (root) {
        //深度遍历树节点,转换为对应QTreeWidgetItem
        QTreeWidgetItem *rootTreeWidgetItem = convertTreeNodeToTreeWidgetItem(root, ui->connectTreeWidget);
        // 释放文件树内存
//        delete root;
    }
    // 显示 QTreeWidget
    ui->connectTreeWidget->show();


    //使用智能指针构建对象，成员变量超出作用域MainWindow之后会自动释放
    Notepad* notePad = new Notepad(this);
    notepad.reset(notePad);
    connect(notePad, &Notepad::uploadFile, this, &MainWindow::uploadFileByLocalPath,Qt::QueuedConnection);



    //隐藏进度条
    ui->downloadProgressBar->hide();
    ui->uploadProgressBar->hide();

//    vncServerThread = new VNCServerThread(5900);
//    // 连接线程结束信号到对象删除槽
//    QObject::connect(vncServerThread, &QThread::finished, vncServerThread, &QObject::deleteLater);
//    // 启动线程
//    vncServerThread->start();


//    // 创建客户端线程
//    QString ip = "127.0.0.1";
//    int port = 5900;
//    vncClientThread = new VNCClientThread(ip, port, this);

//    // 将显示窗口添加到 Tab 页
//    // 获取第二个标签页的 QWidget （本地文件树）
//    QWidget *vncWdiget = ui->RemoteTabWidget->widget(1);
//    QVBoxLayout* vncLayout = new QVBoxLayout(vncWdiget);
//    QWidget* display = vncClientThread->getDisplayWidget();

//        QPainter painter(display);

//        painter.end();
//        vncLayout->addWidget(display);
//    vncWdiget->setLayout(vncLayout);  // 设置布局;
//    // 连接屏幕更新信号到渲染函数
//    connect(vncClientThread, &VNCClientThread::screenUpdated,
//            this, &MainWindow::renderScreen);

//    // 启动线程
//    vncClientThread->start();

}
//void MainWindow::renderScreen(const QImage& image) {
//       // 在显示窗口上绘制图像
//       QPainter painter(vncClientThread->getDisplayWidget());
//       painter.drawImage(0, 0, image);
//   }
void MainWindow::itemChanged(QStandardItem *item){
    if (item->column() == 0) { // 假设文件名在第一列
      QString newName = getPath(item);
      QString oldName = StringUtil::extractPathByAbsolutePath(getPath(item)) + item->data(Qt::UserRole).toString();
      int rc = libssh2_sftp_rename(this->currentSftp->sftp, oldName.toUtf8().constData(), newName.toUtf8().constData());
      if (rc == 0) {
         qDebug() << "重命名远程文件成功: " << newName;
         //删除成功，发送路径更新命令
         LoginSession loginSession = sshConnectMap.value(ui->SshTabWidget->currentIndex());
         sendPwdCommand(loginSession);
      } else {
         qDebug() << "重命名远程文件失败，新名: " << newName << "，旧名：" << oldName;
      }

//      qDebug() << "文件名修改："<< newName;
//      qDebug() << "旧文件名："<< oldName;
    }
}
// 递归获取QTreeView里的节点的路径
QString MainWindow::getPath(QStandardItem *item) {
    QString path;
    QStandardItem *currentItem = item;
    while (currentItem) {
      if (!path.isEmpty()) {
          path = "/" + path;
      }
      path = currentItem->text() + path;
      currentItem = currentItem->parent();
    }
    QString parentPath = "/";
    if(StringUtil::isNotBlank(this->currentPath)){
        parentPath = this->currentPath;
    }
    return parentPath + "/" + path;
}

void MainWindow::deleteWriteThread(){
    qDebug()<<"写线程释放";
    CustomizeCmdTextEdit *currentTextEdit = this->getTab(ui->SshTabWidget->currentIndex());
    if (currentTextEdit->writeThread && currentTextEdit->writeThread->isRunning()) {
        // 请求线程退出
        currentTextEdit->writeThread->quit();
        // 等待线程执行完毕
        currentTextEdit->writeThread->wait();
    }
//    // 释放线程对象
//    delete currentTextEdit->writeThread;
//    // 将指针置为 nullptr
//    currentTextEdit->writeThread = nullptr;
//    // 3. 释放线程对象
//    delete currentTextEdit->writeWork;
//    currentTextEdit->writeWork = nullptr;
}
/**
 * 命令行窗口回车事件处理
 * @brief MainWindow::customizeCmdTextEditEnterPressHandle
 */
void MainWindow::customizeCmdTextEditEnterPressHandle() {

    // this传入给线程使用，例如调用 MainWindow 的方法
    CustomizeCmdTextEdit *currentCmdTextEdit = this->getTab(ui->SshTabWidget->currentIndex());
    qDebug() << "当前customizeCmdTextEditEnterPressHandle线程ID: " << reinterpret_cast<qint64>(QThread::currentThreadId());

    qDebug()<<"-------------------------"<<ui->SshTabWidget->currentIndex()<<"线程运行状态："<<currentCmdTextEdit->readThread->isRunning()<<"，stopFlag："<<currentCmdTextEdit->readThread->stopFlag;


    //        qDebug() << "命令行窗口回车！！！文本框内容：" << currentCmdTextEdit->toPlainText() << "筛选出来的命令：" << command << "筛选出来的lastText：" << currentCmdTextEdit->lastText;
        //执行cmd命令，并进行结果回显
        LoginSession loginSession = sshConnectMap[ui->SshTabWidget->currentIndex()];
        qDebug()<<"customizeCmdTextEditEnterPressHandle获取ip："<<loginSession.loginUser.loginIp;
//        //session和channel必须放在主线程，避免线程销毁后重复创建
//        LIBSSH2_SESSION *session = loginSession.session;
//        LIBSSH2_CHANNEL *channel = loginSession.channel;


        createWriteThread();
//    //有可能有耗时命令、实时读取刷新ui的命令，会阻塞主线程执行，需要新开线程
//    // 创建工作线程
//    QThread* writeThread =new QThread;
//    // 创建一个临时的 QObject 用于承载槽函数
//    QObject* worker =new QObject;
//    // 将 worker 对象移动到新线程中
//    worker->moveToThread(writeThread);
//    // 连接线程的 started 信号到一个 Lambda 表达式（槽函数）14412 14168
//    QObject::connect(writeThread, &QThread::started, writeWorker,  [this,currentCmdTextEdit,loginSession,command]() {


//        if (command.trimmed().startsWith("su") || command.trimmed().startsWith("sudo")) {  //切换用户命令的回车事件（需要输入密码）
//            currentCmdTextEdit->openEncrypt(); //开启密文模式
//        }


//        QSshSocket sshSocket;
//        Result<ExecuteCommandResult> result;
//        sshSocket.executeShellCommand(command,loginSession.session,loginSession.channel);

//        //    currentCmdTextEdit->insertPlainTextByEncryptStatus(result.data.content);


//        // 获取子线程ID
//        qint64 subThreadId = reinterpret_cast<qint64>(QThread::currentThreadId());
//        // 在主线程中打印子线程ID
//        qDebug() << "customizeCmdTextEditEnterPressHandle子线程ID: " << subThreadId;

////        //执行完命令后，结束当前线程
////        QThread::currentThread()->quit();
////        QThread::currentThread()->wait();

//    });
//    // 连接线程的 finished 信号到线程自身的 deleteLater 槽函数，确保线程对象在线程结束后被删除
//    QObject::connect(writeThread, &QThread::finished, this, &MainWindow::deleteWriteThread,Qt::QueuedConnection);
//    // 连接线程的 finished 信号到 worker 对象的 deleteLater 槽函数，确保 worker 对象在线程结束后被删除
//    QObject::connect(writeThread, &QThread::finished , worker, &QObject::deleteLater,Qt::QueuedConnection);

////    connect(this, &MainWindow::stopWork, thread, &QThread::quit);
////    connect(this, &MainWindow::stopWork, thread, &QThread::deleteLater);
////    connect(this, &MainWindow::stopWork, worker, &QThread::deleteLater);

//    // 启动线程
//    writeThread->start();






}
/**
 * 连接菜单的双击事件
 * @brief MainWindow::connectTreeNodeDoubleClick
 * @param item
 * @param column
 */
void MainWindow::connectTreeNodeDoubleClick(QTreeWidgetItem *item, int column) {
    // 判断是否为叶子节点
    if (item->childCount() == 0) {
        // 处理双击叶子节点的事件
        qDebug() << "Double clicked on leaf item:" << item->text(column);
        // 这里可以添加更多针对叶子节点的处理逻辑
        // 双击后在末尾处，新打开一个ssh连接窗口（linux支持同一个账号多人同时登录，同一个连接可打开多个窗口进行处理）
        //ssh工具类使用
        QStringList ipPortList = item->text(0).split(":");
        QString ipAddr = ipPortList.value(0); //本次连接的ip地址
        int port = ipPortList.value(1).toInt(); //本次连接的端口号
        qDebug() << "连接ip端口：" << ipAddr << port;
        QString userName = "shenqinlin";
        QString password = "woshisqlerzi";
        //创建当前窗口对象
        int index = ui->SshTabWidget->count() - 1 ; //添加窗口后索引值会改变，所以必须先暂时存起来
        CustomizeCmdTextEdit *newTextEdit = addTab(index, ipAddr + ":" + QString::number(port));
        newTextEdit->append("初始化:沈daye制作，基于libssh2的ssh终端管理工具\n");
//        newTextEdit->insertPlainText("["+userName + " 主机名 ~]$ ");
        //命令框的回车信号处理
        connect(newTextEdit, &CustomizeCmdTextEdit::enterPressed, this, &MainWindow::customizeCmdTextEditEnterPressHandle,Qt::QueuedConnection);
        //命令框的ctrl+c终止信号处理
        connect(newTextEdit, &CustomizeCmdTextEdit::ctrlAndCPressed, this, &MainWindow::resetChannel,Qt::QueuedConnection);
        // 获取主线程ID
        qint64 mainThreadId = reinterpret_cast<qint64>(QThread::currentThreadId());
        // 在主线程中打印线程ID
        qDebug() << "主线程ID: " << mainThreadId;


        // 创建工作线程
        WorkThread *workThread = new WorkThread(ipAddr, port, userName, password, index, this);
        // 连接信号和槽（假设你有相应的槽函数来处理 updateUI 信号）
        // 使用 Lambda 表达式连接信号和槽(Qt::QueuedConnection 来保证 UI 操作在主线程执行)
        connect(workThread, &WorkThread::updateUI, this, &MainWindow::loginUpdateUI, Qt::QueuedConnection);
        // 在线程结束后自动退出（删除对象）
        connect(workThread, &WorkThread::finished, workThread, &QObject::deleteLater,Qt::QueuedConnection);
        // 启动线程
        workThread->start();
    } else {
        qDebug() << "Double clicked on non - leaf item:" << item->text(column);
        // 这里可以添加针对非叶子节点的处理逻辑，若需要的话
    }
}
//发送一个标识符pwd命令
void MainWindow::sendPwdCommand(LoginSession loginSession){
    // 创建工作线程
    MonitorPathChangeThread *monitorPathChangeThread = new MonitorPathChangeThread(loginSession,signalStr,this);
    //启动文件路径变化监控线程
    monitorPathChangeThread->start();
}
//重置channel
void MainWindow::resetChannel(){
    qDebug()<<"重置Channel";
    LoginSession loginSession = sshConnectMap.value(ui->SshTabWidget->currentIndex());
    QSshSocket sshSocket;
    // 发送 Ctrl+C（0x03）到通道
    sshSocket.sendBlockEndSignal(loginSession.channel);
    qDebug()<<"重置Channel结束";
}
//终止读线程
void MainWindow::deleteReadThread(int index){
    CustomizeCmdTextEdit* currentTextEdit = getTab(index);
    //终止循环标志
    currentTextEdit->readThread->stopFlag = true;
    if (currentTextEdit->readThread && currentTextEdit->readThread->isRunning()) {
        // 请求线程退出
        currentTextEdit->readThread->quit();
        // 等待线程执行完毕
        currentTextEdit->readThread->wait();
    }
    // 释放线程对象
    delete currentTextEdit->readThread;
    // 将指针置为 nullptr
    currentTextEdit->readThread = nullptr;
}
//重置当前窗口的读线程（先销毁再创建）
void MainWindow::resetReadThread(){
    deleteReadThread(ui->SshTabWidget->currentIndex());
    //创建新的readThread线程并赋值
    createReadThread();
}
//创建当前窗口的读线程
void MainWindow::createReadThread(){

    qDebug()<<"重新创建线程信号！！！！！！！！";
    CustomizeCmdTextEdit* currentTextEdit = getTab(ui->SshTabWidget->currentIndex());
//    //有可能有耗时命令、实时读取刷新ui的命令，会阻塞主线程执行，需要新开线程
//    // 创建工作线程
//    QThread* readThread =new QThread;
//    // 创建一个临时的 QObject 用于承载槽函数
//    QObject* readWorker =new QObject;
//    // 将 worker 对象移动到新线程中
//    readWorker->moveToThread(readThread);
//    //取出当前窗口索引处的loginSession
//    LoginSession loginSession = sshConnectMap.value(ui->SshTabWidget->currentIndex());
//    // 连接线程的 started 信号到一个 Lambda 表达式（槽函数）14412 14168
//    QObject::connect(readThread, &QThread::started, readWorker,  [this,currentTextEdit,loginSession]() {


//        // 获取子线程ID
//        qint64 subThreadId = reinterpret_cast<qint64>(QThread::currentThreadId());
//        // 在主线程中打印子线程ID
//        qDebug() << "循环读线程线程ID: " << subThreadId;
//        QSshSocket sshSocket2;
//        //连接实时命令的更新信号(使用 Qt::QueuedConnection 连接信号和槽：保证信号和槽的调用在接收者所在的线程（通常是主线程）中执行，避免跨线程操作。)
//        connect(&sshSocket2, &QSshSocket::readTimeCommandResultUpdate, this, &MainWindow::appendContent,Qt::QueuedConnection);
//        while (1) {
////            qDebug()<<"读取内容，子线程id："<<subThreadId;
//            Result<ExecuteCommandResult> result;
//            sshSocket2.readChannel(loginSession.channel);
//            // 可以在这里添加适当的延迟，避免过于频繁地读取
//            QThread::msleep(100); // 暂停 100 毫秒
//        }


//    });

//    // 连接线程的 finished 信号到线程自身的 deleteLater 槽函数，确保线程对象在线程结束后被删除
//    QObject::connect(readThread, &QThread::finished, this, &MainWindow::deleteReadThread,Qt::QueuedConnection);
//    // 连接线程的 finished 信号到 writeWork 对象的 deleteLater 槽函数，确保 writeWork 对象在线程结束后被删除
//    QObject::connect(readThread, &QThread::finished , readWorker, &QObject::deleteLater,Qt::QueuedConnection);
//    // 启动线程
//    readThread->start();


    //取出当前窗口索引处的loginSession
    LoginSession loginSession = sshConnectMap.value(ui->SshTabWidget->currentIndex());
    // 创建工作线程
    ReadThread *readThread = new ReadThread(loginSession,signalStr,false,this);
    readThread->stopFlag = false;
    // 连接信号和槽（假设你有相应的槽函数来处理 updateUI 信号）
    // 使用 Lambda 表达式连接信号和槽(Qt::QueuedConnection 来保证 UI 操作在主线程执行)
    connect(readThread, &ReadThread::appendContent, this, &MainWindow::appendContent,Qt::QueuedConnection);
    connect(readThread, &ReadThread::updateFileTreeView, this, &MainWindow::updateFileTreeView,Qt::QueuedConnection);
    //读线程无限循环，必须使用手动调用deleteReadThread方法终止，使用finish信号会导致无限循环
//    // 连接线程的 finished 信号到线程自身的 deleteLater 槽函数，确保线程对象在线程结束后被删除
//    QObject::connect(readThread, &QThread::finished, this, &MainWindow::deleteReadThread,Qt::QueuedConnection);
    // 启动线程
    readThread->start();

    //线程赋值给ui
    currentTextEdit->readThread = readThread;
}
//重新创建写线程（直接创建赋值）
void MainWindow::createWriteThread(){
    CustomizeCmdTextEdit* currentTextEdit = getTab(ui->SshTabWidget->currentIndex());
    //取出当前窗口索引处的loginSession
    LoginSession loginSession = sshConnectMap.value(ui->SshTabWidget->currentIndex());
    //有可能有耗时命令、实时读取刷新ui的命令，会阻塞主线程执行，需要新开线程
    // 创建工作线程
    QThread* writeThread =new QThread;
    // 创建一个临时的 QObject 用于承载槽函数
    QObject* writeWork =new QObject;
    // 将 writeWork 对象移动到新线程中
    writeWork->moveToThread(writeThread);
    // 连接线程的 started 信号到一个 Lambda 表达式（槽函数）14412 14168
    QObject::connect(writeThread, &QThread::started, writeWork,  [this,currentTextEdit,loginSession]() {

        QString command = "";
        if(currentTextEdit->isEncryptMode){//密文模式
            //密文模式下执行命令为当前保存的明文
            command = currentTextEdit->encryptedSubStr;

//            qDebug()<<"密文密码："<<command;

//            qDebug()<<"测1："<<currentTextEdit->toPlainText()<<"测2："<<StringUtil::extractUserName(currentTextEdit->lastText.trimmed());
            currentTextEdit->tempUserName = StringUtil::extractUserName(currentTextEdit->lastText.trimmed());
             currentTextEdit->tempPassword = currentTextEdit->encryptedSubStr;


        }else{//明文模式
            //明文模式下才是提取的命令
            command = StringUtil::extractSubstringAfter(currentTextEdit->toPlainText(),currentTextEdit->lastText);
        }

        if (command.trimmed().startsWith("su") || command.trimmed().startsWith("sudo")) {  //切换用户命令的回车事件（需要输入密码）
            currentTextEdit->openEncrypt(); //开启密文模式
        }


        QSshSocket sshSocket;
        Result<ExecuteCommandResult> result;
        sshSocket.executeShellCommand(command,loginSession.session,loginSession.channel);

        //    currentTextEdit->insertPlainTextByEncryptStatus(result.data.content);

        if(command.trimmed().startsWith("cd ")){
            //登录认证成功，开启文件路径监控线程
            sendPwdCommand(loginSession);
//            updateFileTreeView(command.replace("cd","").trimmed());
        }

        // 获取子线程ID
        qint64 subThreadId = reinterpret_cast<qint64>(QThread::currentThreadId());
        // 在主线程中打印子线程ID
        qDebug() << "customizeCmdTextEditEnterPressHandle子线程ID: " << subThreadId;

    });


    // 启动线程
    writeThread->start();
    //线程赋值给ui
    currentTextEdit->writeThread = writeThread;
    currentTextEdit->writeWork = writeWork;
}


//重置写线程（先销毁后创建）
void MainWindow::resetWriteThread(){
    deleteWriteThread();
    createWriteThread();
}
//重置光标判断（如果是当前窗口则设置光标到最后，否则则不）
bool MainWindow::setTextEditCursor(int index){
    if(index == ui->SshTabWidget->currentIndex()){//是当前所在窗口，设置光标
        CustomizeCmdTextEdit* tabTextEdit = getTab(index);
        tabTextEdit->moveCursorToEndPos();
    }
}

//当前路径更新，更新对应的文件树
void MainWindow::updateFileTreeView(QString currentPath){
    qDebug()<<"当前路径更新，更新对应的文件树，X变化当前路径："<<currentPath;
    getPathQTreeView(this->currentSftp->session,this->currentSftp->sftp,currentPath,fileTreeView);
   fileTreeView->show();
   this->currentPath = currentPath;
}
void MainWindow::updateSftpSession(SftpSession* sftpSession){
    this->currentSftp = sftpSession;
    LoginSession loginSession = sshConnectMap.value(ui->SshTabWidget->currentIndex());
    //sftp连接成功，进行路径线程更新文件树
    sendPwdCommand(loginSession);
//    updateFileTreeView("/home/"+loginSession.loginUser.loginUserName+"/");
}
void MainWindow::uploadFileByLocalPath(QString localPath){
    UploadThread* uploadThread = new UploadThread(this->currentSftp->sftp, localPath,this->currentPath,
                                                                    false,this);
   connect(uploadThread,&UploadThread::updateUploadProgressBar,this,&MainWindow::updateUploadProgressBar,Qt::QueuedConnection);
   uploadThread->start();
//    qDebug()<<"上传文件："<<localPath;
//    // 打开本地文件
//    QFile localFile(localPath);
//    if (!localFile.open(QIODevice::ReadOnly)) {
//        qDebug()<< "Failed to open local file for reading：" << localPath;
//        return ;
//    }

//    // 构建远程临时文件路径
//    QString remotePath = this->currentPath + "/" + StringUtil::extractFileNameOrDirNameByAbsolutePath(localPath) + ".tmp";

//    // 打开远程文件进行写入
//    LIBSSH2_SFTP_HANDLE* handle = libssh2_sftp_open(this->currentSftp->sftp, remotePath.toUtf8().constData(),
//                                                    LIBSSH2_FXF_WRITE | LIBSSH2_FXF_CREAT | LIBSSH2_FXF_TRUNC, 0644);
//    if (!handle) {
//        qDebug()<< "libssh2_sftp_open failed：" << remotePath;
//        localFile.close();
//        return ;
//    }

//    // 缓冲区
//    char buffer[1024];
//    while (localFile.read(buffer, sizeof(buffer)) > 0) {
//        qint64 bytesRead = localFile.bytesAvailable();
//        if (bytesRead > 0) {
//            libssh2_sftp_write(handle, buffer, static_cast<size_t>(bytesRead));
//        }
//    }

//    // 关闭远程文件句柄
//    libssh2_sftp_close(handle);

//    // 重命名远程文件
//    QString finalRemotePath = remotePath + "/" + localPath;
//    libssh2_sftp_rename(this->currentSftp->sftp, remotePath.toUtf8().constData(), finalRemotePath.toUtf8().constData());

//    // 关闭本地文件
//    localFile.close();
}
void MainWindow::loginUpdateUI(LoginSession loginSession,
                               bool loginStatus) {
//    qDebug() << "接收到updateUI信号：" << loginStatus;
    CustomizeCmdTextEdit *currentTextEdit = getTab(loginSession.index);
    // 在这里编写原本 loginUpdateUI 方法中的逻辑
    if (loginStatus) {
        // 处理登录成功的情况，例如添加新连接
        // 这里可以添加你的具体代码
        //登录认证成功，存放已登录信息（QMap无法直接赋值，只能构造新LoginSession之后再进行赋值）
//        LoginSession newLoginSession = *new LoginSession(loginSession);
//        newLoginSession.session = loginSession.session;
//        newLoginSession.channel = loginSession.channel;
//        QMap<SshCmd, SshCmdResult> cmdMap;
//        newLoginSession.cmdMap = cmdMap;
        sshConnectMap.insert(loginSession.index, loginSession);
        QSshSocket sshSocket;
        Result<ExecuteCommandResult> result = sshSocket.readChannel(loginSession.channel);
        currentTextEdit->insertPlainTextByEncryptStatus(result.data.content);
        setTextEditCursor(ui->SshTabWidget->currentIndex());

//        UIUtil::setQTextEditCursorEndBlink(currentTextEdit);

//        //登录成功，获取文件显示树
//        sshSocket.getPathQTreeView(loginSession,"/home/shenqinlin/",fileTreeView);
//        fileTreeView->show();

//        loginSession = sshSocket.createSftp(loginSession);
//        sshSocket.getPathQTreeView(loginSession,"/home/shenqinlin/",fileTreeView);


//        this->currentSftp = sshSocket.createSftp(loginSession);


//        this->currentSftp = sshSocket.createSftp(sshConnectMap.value(ui->SshTabWidget->currentIndex()));
//        sshSocket.getPathQTreeView(this->currentSftp->session,this->currentSftp->sftp,"/home/shenqinlin/",fileTreeView);


        //登录认证成功，开启读线程
        createReadThread();

        //启动sftp连接线程
        SftpConnectThread* sftpConnectThread = new SftpConnectThread(loginSession,this);
        connect(sftpConnectThread, &SftpConnectThread::updateSftpSession, this, &MainWindow::updateSftpSession, Qt::QueuedConnection);
        //启动文件路径变化监控线程
        sftpConnectThread->start();





    } else {
        // 处理登录失败的情况，例如显示报错信息
        if (StringUtil::isNotBlank(loginSession.msg)) {
            currentTextEdit->insertPlainTextByEncryptStatus(loginSession.msg);
            setTextEditCursor(ui->SshTabWidget->currentIndex());
        }
//        UIUtil::setQTextEditCursorEndBlink(newTextEdit);
    }
    //记录命令执行后的文本
    currentTextEdit->lastText = currentTextEdit->toPlainText();
}
/**
 * 深度遍历：将 TreeNode 转换为 QTreeWidgetItem
 * @brief convertTreeNodeToTreeWidgetItem
 * @param node
 * @param treeWidget 传入 QTreeWidget 指针，用于创建根节点
 * @return
 */
QTreeWidgetItem *MainWindow::convertTreeNodeToTreeWidgetItem(TreeNode *node, QTreeWidget *treeWidget) {
    if (!node) {
        return nullptr;
    }
    QTreeWidgetItem *item;
    if (treeWidget) {
        item = new QTreeWidgetItem(treeWidget);
    } else {
        item = new QTreeWidgetItem();
    }
    item->setText(0, node->name);
    item->setIcon(0, node->icon);
    // 递归处理子节点
    for (TreeNode *child : node->children) {
        QTreeWidgetItem *childItem = convertTreeNodeToTreeWidgetItem(child, nullptr);
        item->addChild(childItem);
    }
    return item;
}
//树节点的右键菜单方法
void MainWindow::showContextMenu(const QPoint &pos) {
    qDebug() << "右键触发";
    QModelIndex index = localFileTreeView->indexAt(pos);
    if (index.isValid()) {
        QFileInfo fileInfo = localFileSystemModel->fileInfo(index);
        if (fileInfo.isFile()) {
            QMenu contextMenu(this);
            QAction *deleteAction = contextMenu.addAction("删除文件");
            deleteAction->setIcon(QIcon(":/icon/folder.png"));
            QAction *modifyFileNameAction = contextMenu.addAction("重命名");
            modifyFileNameAction->setIcon(QIcon(":/icon/folder.png"));
            QAction *queryAction = contextMenu.addAction("打开文件");
            queryAction->setIcon(QIcon(":/icon/folder.png"));
            QAction *copyAction = contextMenu.addAction("复制文件");
            copyAction->setIcon(QIcon(":/icon/folder.png"));
            QAction *uploadAction = contextMenu.addAction("上传文件");
            uploadAction->setIcon(QIcon(":/icon/folder.png"));
            connect(copyAction, &QAction::triggered, this, &MainWindow::createFile);
            connect(deleteAction, &QAction::triggered, this, &MainWindow::deleteFileOrDir);
            connect(modifyFileNameAction, &QAction::triggered, this, &MainWindow::modifyFileName);
            connect(queryAction, &QAction::triggered, this, &MainWindow::openFile);
            connect(uploadAction, &QAction::triggered, this, &MainWindow::uploadFile);

            contextMenu.exec(localFileTreeView->viewport()->mapToGlobal(pos));



        } else if (fileInfo.isDir()) {
            QMenu contextMenu(this);
            QAction *createFileAction = contextMenu.addAction("创建文件");
            // 使用资源路径来设置图标
            createFileAction->setIcon(QIcon(":/icon/folder.png"));
            QAction *clearDirAction = contextMenu.addAction("清空目录");
            clearDirAction->setIcon(QIcon(":/icon/folder.png"));
            QAction *createDirAction = contextMenu.addAction("创建目录");
            createDirAction->setIcon(QIcon(":/icon/folder.png"));
            QAction *deleteDirAction = contextMenu.addAction("删除目录");
            deleteDirAction->setIcon(QIcon(":/icon/folder.png"));
            QAction *uploadDirAction = contextMenu.addAction("上传目录");
            uploadDirAction->setIcon(QIcon(":/icon/folder.png"));
            connect(createFileAction, &QAction::triggered, this, &MainWindow::createFile);
            connect(createDirAction, &QAction::triggered, this, &MainWindow::createDir);
            connect(deleteDirAction, &QAction::triggered, this, &MainWindow::deleteFileOrDir);
            connect(clearDirAction, &QAction::triggered, this, &MainWindow::clearDir);
            connect(uploadDirAction, &QAction::triggered, this, &MainWindow::uploadDir);
            contextMenu.exec(localFileTreeView->viewport()->mapToGlobal(pos));
        }
    }
}
//树节点的右键菜单方法
void MainWindow::showRemoteContextMenu(const QPoint &pos) {
    qDebug() << "右键触发";
    QModelIndex index = fileTreeView->indexAt(pos);
    if (index.isValid()) {
        // 获取选中项的文件名
        QString fileName = fileTreeView->model()->data(index.siblingAtColumn(0)).toString();

        QString parentPath = "/";
        if(StringUtil::isNotBlank(currentPath)){
            parentPath = currentPath;
        }

        // 获取选中项所在的完整路径，考虑子目录/孙子目录层级
        QModelIndex currentIndex = index;
        QString subDirPath = ""; //拼接后的父目录名
        while (currentIndex.parent().isValid()) {
            QString subDirName = fileTreeView->model()->data(currentIndex.parent().siblingAtColumn(0)).toString();//不停获取选中项对应的父目录名字进行拼接
            subDirPath = subDirName + "/" + subDirPath;
            currentIndex = currentIndex.parent();
        }

        QString fullPath = parentPath + "/"+ subDirPath + fileName;
        qDebug()<<"选择项的文件名："<<fullPath;
        QSshSocket sshSocket;
        if (sshSocket.isFile(this->currentSftp->sftp,fullPath)) {
            qDebug()<<"选择项为文件：";
            QMenu contextMenu(this);
            QAction *deleteAction = contextMenu.addAction("删除文件");
            deleteAction->setIcon(QIcon(":/icon/folder.png"));
            QAction *modifyFileNameAction = contextMenu.addAction("重命名");
            modifyFileNameAction->setIcon(QIcon(":/icon/folder.png"));
            QAction *queryAction = contextMenu.addAction("打开文件");
            queryAction->setIcon(QIcon(":/icon/folder.png"));
            QAction *copyAction = contextMenu.addAction("复制文件");
            copyAction->setIcon(QIcon(":/icon/folder.png"));
            QAction *downloadAction = contextMenu.addAction("下载文件");
            downloadAction->setIcon(QIcon(":/icon/folder.png"));

            connect(copyAction, &QAction::triggered, [=]() {
                copyRemoteFile(fullPath);
            });
            connect(deleteAction, &QAction::triggered, [=]() {
                deleteRemoteFile(fullPath);
            });
            connect(modifyFileNameAction, &QAction::triggered, [=]() {
                modifyRemoteFileName(fullPath);
            });
            connect(queryAction, &QAction::triggered, [=]() {
                openRemoteDirOrFile(fullPath);
            });
            connect(downloadAction, &QAction::triggered, [=]() {
                downloadRemoteDirOrFile(fullPath);
            });

            contextMenu.exec(fileTreeView->viewport()->mapToGlobal(pos));
        } else if (sshSocket.isDir(this->currentSftp->sftp,fullPath)){
            qDebug()<<"选择项为目录：";
            QMenu contextMenu(this);
            QAction *createFileAction = contextMenu.addAction("创建文件");
            createFileAction->setIcon(QIcon(":/icon/folder.png"));
            QAction *clearDirAction = contextMenu.addAction("清空目录");
            clearDirAction->setIcon(QIcon(":/icon/folder.png"));
            QAction *createDirAction = contextMenu.addAction("创建目录");
            createDirAction->setIcon(QIcon(":/icon/folder.png"));
            QAction *deleteDirAction = contextMenu.addAction("删除目录");
            deleteDirAction->setIcon(QIcon(":/icon/folder.png"));
            QAction *modifyDirNameAction = contextMenu.addAction("重命名");
            modifyDirNameAction->setIcon(QIcon(":/icon/folder.png"));
            QAction *downloadAction = contextMenu.addAction("下载目录");
            downloadAction->setIcon(QIcon(":/icon/folder.png"));

            connect(createFileAction, &QAction::triggered, [=]() {
                createRemoteFile(fullPath);
            });
            connect(clearDirAction, &QAction::triggered, [=]() {
                clearRemoteDir(fullPath);
            });
            connect(createDirAction, &QAction::triggered, [=]() {
                createRemoteDir(fullPath);
            });
            connect(deleteDirAction, &QAction::triggered, [=]() {
                deleteRemoteDir(fullPath);
            });
            connect(modifyDirNameAction, &QAction::triggered, [=]() {
                modifyRemoteDirName(fullPath);
            });
            connect(downloadAction, &QAction::triggered, [=]() {
                downloadRemoteDirOrFile(fullPath);
            });
            contextMenu.exec(fileTreeView->viewport()->mapToGlobal(pos));
        }
    }
}
void MainWindow::copyRemoteFile(QString path){

}

void MainWindow::deleteRemoteFile(QString path){
    int rc = libssh2_sftp_unlink(this->currentSftp->sftp, path.toUtf8().constData());
    if (rc == 0) {
       qDebug() << "删除远程文件成功: " << path;
       //删除成功，发送路径更新命令
       LoginSession loginSession = sshConnectMap.value(ui->SshTabWidget->currentIndex());
       sendPwdCommand(loginSession);
    } else {
       qDebug() << "删除远程文件失败: " << path;
    }
}
void MainWindow::modifyRemoteFileName(QString path){
//        qDebug()<<"重命名功能，当前路径："<<path<<"，旧名："<<StringUtil::extractFileNameOrDirNameByAbsolutePath(path);
    // 提示用户输入新名称
    bool ok;
    QString newFileName = QInputDialog::getText(this, "重命名", "请输入新的名称:", QLineEdit::Normal, StringUtil::extractFileNameOrDirNameByAbsolutePath(path), &ok);
    //   QString newFileName = UIUtil::makeInputDialog( "重命名", "请输入新的名称:",toNonConstStr);
    if (ok && StringUtil::isNotBlank(newFileName)) {
//        qDebug()<<"测试1："<<QString(StringUtil::extractPathByAbsolutePath(path)+newFileName)<<"，测试2："<<path;
            int rc = libssh2_sftp_rename(this->currentSftp->sftp, path.toUtf8().constData(), QString(StringUtil::extractPathByAbsolutePath(path)+newFileName).toUtf8().constData());
            if (rc == 0) {
               qDebug() << "重命名远程文件成功: " << newFileName;
               //删除成功，发送路径更新命令
               LoginSession loginSession = sshConnectMap.value(ui->SshTabWidget->currentIndex());
               sendPwdCommand(loginSession);
            } else {
               qDebug() << "重命名远程文件失败: " << newFileName;
            }
    }

}
void MainWindow::openRemoteDirOrFile(QString path){
    qDebug()<<"路径："<<path;
//    QSshSocket sshSocket;
//    bool result = sshSocket.downloadFile(this->currentSftp->sftp,path,EXE_APPLICATION_PATH+"/download"+path);
    QString localPath = *new QString(EXE_APPLICATION_PATH+"/download"+path);
    QString remotePath = *new QString(path);
    DownloadThread* downloadThread = new DownloadThread(this->currentSftp->sftp, remotePath,
                                                                    localPath,false,this);
   connect(downloadThread,&DownloadThread::updateDownloadProgressBar,this,&MainWindow::updateDownloadProgressBar,Qt::QueuedConnection);
    downloadThread->start();
}

void MainWindow::downloadRemoteDirOrFile(QString path){
    // 打开文件对话框让用户选择本地保存路径
    QString defaultDir = EXE_APPLICATION_PATH + "/download";
    //自定义本地目录
    QString localPath = QFileDialog::getExistingDirectory(this, tr("选择保存目录"), defaultDir);

    if (!localPath.isEmpty()) {
      // 拼接最终的本地路径
      QString finalLocalPath = localPath + QDir::separator() + QFileInfo(path).fileName();
      qDebug()<<"本地路径："<<finalLocalPath;
      QString remotePath = path;
      DownloadThread* downloadThread = new DownloadThread(this->currentSftp->sftp, remotePath,
                                                          finalLocalPath, false, this);
      connect(downloadThread, &DownloadThread::updateDownloadProgressBar, this, &MainWindow::updateDownloadProgressBar, Qt::QueuedConnection);
      downloadThread->start();
    }
}

void MainWindow::createRemoteFile(QString path){
    // 提示用户输入新名称
    bool ok;
    QString newFileName = QInputDialog::getText(this, "重命名", "请输入新的名称:", QLineEdit::Normal, "new_file", &ok);
    if (ok && StringUtil::isNotBlank(newFileName)) {
        newFileName = path + "/" + newFileName;
        qDebug()<<"新增的文件名："<<newFileName;
        LIBSSH2_SFTP_HANDLE* handle = libssh2_sftp_open(this->currentSftp->sftp, newFileName.toUtf8().constData(),LIBSSH2_FXF_CREAT, 0644);
        if (handle) {
           qDebug() << "创建远程文件成功: " << newFileName;
           libssh2_sftp_close(handle);
           //删除成功，发送路径更新命令
           LoginSession loginSession = sshConnectMap.value(ui->SshTabWidget->currentIndex());
           sendPwdCommand(loginSession);
        } else {
           qDebug() << "创建远程文件失败: " << newFileName;
        }
    }
}
void MainWindow::clearRemoteDir(QString path){
    QSshSocket sshSocket;
    bool result = sshSocket.recursiveRemoveDir(this->currentSftp->sftp, path.toUtf8().constData(),false);
    if (result) {
        qDebug() << "删除远程目录成功: " << path;
        //删除成功，发送路径更新命令
        LoginSession loginSession = sshConnectMap.value(ui->SshTabWidget->currentIndex());
        sendPwdCommand(loginSession);
    } else {
        QSshSocket sshSocket;
        qDebug() << "删除远程目录失败: " << path <<"，error："<<sshSocket.getSessionLastErrot(this->currentSftp->session);
    }
}
void MainWindow::createRemoteDir(QString path){
    // 提示用户输入新名称
    bool ok;
    QString newDirName = QInputDialog::getText(this, "重命名", "请输入新的名称:", QLineEdit::Normal, "new_dir", &ok);
    if (ok && StringUtil::isNotBlank(newDirName)) {
        QString newDirPath = path + "/" + newDirName;

        qDebug()<<"新增的目录名："<<newDirPath;
        int rc = libssh2_sftp_mkdir(this->currentSftp->sftp, newDirPath.toUtf8().constData(), 0755);
        if (rc == 0) {
           qDebug() << "创建远程目录成功: " << newDirPath;
           //删除成功，发送路径更新命令
           LoginSession loginSession = sshConnectMap.value(ui->SshTabWidget->currentIndex());
           sendPwdCommand(loginSession);
        } else {
           qDebug() << "创建远程目录失败: " << newDirPath;
        }
    }
}
void MainWindow::deleteRemoteDir(QString path){
    //libssh2_sftp_rmdir删除的目录必须为空
//    int rc = libssh2_sftp_rmdir(this->currentSftp->sftp, path.toUtf8().constData());
    QSshSocket sshSocket;
    bool result = sshSocket.recursiveRemoveDir(this->currentSftp->sftp, path.toUtf8().constData(),true);
    if (result) {
        qDebug() << "删除远程目录成功: " << path;
        //删除成功，发送路径更新命令
        LoginSession loginSession = sshConnectMap.value(ui->SshTabWidget->currentIndex());
        sendPwdCommand(loginSession);
    } else {
        QSshSocket sshSocket;
        qDebug() << "删除远程目录失败: " << path <<"，error："<<sshSocket.getSessionLastErrot(this->currentSftp->session);
    }
}
void MainWindow::modifyRemoteDirName(QString path){
    // 提示用户输入新名称
    bool ok;
    QString newDirName = QInputDialog::getText(this, "重命名", "请输入新的名称:", QLineEdit::Normal, StringUtil::extractFileNameOrDirNameByAbsolutePath(path), &ok);
    if (ok && StringUtil::isNotBlank(newDirName)) {
            int rc = libssh2_sftp_rename(this->currentSftp->sftp, path.toUtf8().constData(), QString(StringUtil::extractPathByAbsolutePath(path)+newDirName).toUtf8().constData());
            if (rc == 0) {
               qDebug() << "重命名远程文件成功: " << newDirName;
               //删除成功，发送路径更新命令
               LoginSession loginSession = sshConnectMap.value(ui->SshTabWidget->currentIndex());
               sendPwdCommand(loginSession);
            } else {
               qDebug() << "重命名远程文件失败: " << newDirName;
            }
    }
}
void MainWindow::doubleClickOpenRemoteDirOrFile() {
    QModelIndex index = fileTreeView->currentIndex();
    if (index.isValid()) {
        // 获取选中项的文件名
        QString fileName = fileTreeView->model()->data(index.siblingAtColumn(0)).toString();

        QString parentPath = "/";
        if(StringUtil::isNotBlank(currentPath)){
            parentPath = currentPath;
        }

        // 获取选中项所在的完整路径，考虑子目录/孙子目录层级
        QModelIndex currentIndex = index;
        QString subDirPath = ""; //拼接后的父目录名
        while (currentIndex.parent().isValid()) {
            QString subDirName = fileTreeView->model()->data(currentIndex.parent().siblingAtColumn(0)).toString();//不停获取选中项对应的父目录名字进行拼接
            subDirPath = subDirName + "/" + subDirPath;
            currentIndex = currentIndex.parent();
        }

        QString fullPath = parentPath + "/"+ subDirPath + fileName;


//             qDebug()<<"双击文件路径："<<fullPath;
        openRemoteDirOrFile(fullPath);

    }
}
void MainWindow::copyFile() {
    QModelIndex index = localFileTreeView->currentIndex();
    if (index.isValid()) {
        QFileInfo fileInfo = localFileSystemModel->fileInfo(index);
        if (fileInfo.isDir()) {
            QString filePath = fileInfo.absoluteFilePath() + "/new_file.txt";
            QFile file(filePath);
            if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
                file.close();
                qDebug() << "文件创建成功：" << filePath;
            } else {
                qDebug() << "文件创建失败：" << filePath;
            }
        }
    }
}
void MainWindow::uploadFile(){
    QModelIndex index = localFileTreeView->currentIndex();
    if (index.isValid()) {
        QFileInfo fileInfo = localFileSystemModel->fileInfo(index);
        QString localPath = fileInfo.absoluteFilePath();
        uploadFileByLocalPath(localPath);
    }
}
void MainWindow::createFile() {
    QModelIndex index = localFileTreeView->currentIndex();
    if (index.isValid()) {
        QFileInfo fileInfo = localFileSystemModel->fileInfo(index);
        int i = 0;
        QString filePath = fileInfo.absoluteFilePath() + "/temp" + QString::number(i) + ".txt";
        while (true) {
            qDebug() << "path：" << filePath;
            if (!FileUtil::isExist(filePath)) { //如果不存在则使用当前文件名创建文件
                FileUtil::createFile(filePath);
                break;
            }
            i++;
            //构建要查找和替换的子串
            QString oldStr = QString::number(i - 1) + ".txt";
            QString newStr = QString::number(i) + ".txt";
            StringUtil::replaceEndSubStr(filePath, oldStr, newStr);
        }
    }
}
void MainWindow::uploadDir(){
    QModelIndex index = localFileTreeView->currentIndex();
    if (index.isValid()) {
        QFileInfo fileInfo = localFileSystemModel->fileInfo(index);
        QString localPath = fileInfo.absoluteFilePath();
        uploadFileByLocalPath(localPath);
    }
}
void MainWindow::createDir() {
    QModelIndex index = localFileTreeView->currentIndex();
    if (index.isValid()) {
        QFileInfo fileInfo = localFileSystemModel->fileInfo(index);
        int i = 0;
        QString filePath = fileInfo.absoluteFilePath() + "/temp" + QString::number(i) + "/";
        while (true) {
            qDebug() << "path：" << filePath;
            if (!FileUtil::isExist(filePath)) { //如果不存在则使用当前文件名创建文件
                FileUtil::createDir(filePath);
                break;
            }
            i++;
            //构建要查找和替换的子串
            QString oldStr = QString::number(i - 1) + "/";
            QString newStr = QString::number(i) + "/";
            StringUtil::replaceEndSubStr(filePath, oldStr, newStr);
        }
        FileUtil::createDir(filePath);
    }
}
void MainWindow::deleteFileOrDir() {
    QModelIndex index = localFileTreeView->currentIndex();
    if (index.isValid()) {
        QFileInfo fileInfo = localFileSystemModel->fileInfo(index);
        FileUtil::deleteFileOrDir(fileInfo.absoluteFilePath());
    }
}

void MainWindow::clearDir() {
    QModelIndex index = localFileTreeView->currentIndex();
    if (index.isValid()) {
        QFileInfo fileInfo = localFileSystemModel->fileInfo(index);
        FileUtil::clearDir(fileInfo.absoluteFilePath());
    }
}
void MainWindow::modifyFileName() {
    QModelIndex index = localFileTreeView->currentIndex();
    if (index.isValid()) {
        QFileInfo fileInfo = localFileSystemModel->fileInfo(index);
        QString originalPath = fileInfo.absoluteFilePath();
        // 提示用户输入新名称
        bool ok;
        QString newName = QInputDialog::getText(this, "重命名", "请输入新的名称:", QLineEdit::Normal, fileInfo.fileName(), &ok);
        if (ok && !newName.isEmpty()) {
            // 调用重命名方法
            if (FileUtil::modifyFileName(originalPath, newName)) {
                // 刷新文件树视图
                QString rootPath = fileSystemModel->rootPath();
                fileSystemModel->setRootPath("");
                fileSystemModel->setRootPath(rootPath);
            }
        }
    }
}
void MainWindow::openFile() {
    QModelIndex index = localFileTreeView->currentIndex();
    if (index.isValid()) {
        QFileInfo fileInfo = localFileSystemModel->fileInfo(index);
        if (fileInfo.isFile()) {
            QString filePath = fileInfo.absoluteFilePath();
            qDebug() << "尝试打开文件：" << filePath;
            bool result = FileUtil::openFile(filePath);
            if (!result) {
                qDebug() << "打开文件" + filePath + "报错：";
            }
        }
    }
}

void MainWindow::updateUploadProgressBar(int progress,QString localPath,bool isUpdateRemote){
    //设置进度条进度
    if (ui->uploadProgressBar->isHidden()) {
        // 如果进度条隐藏，首次接收到进度更新时显示它
        qDebug()<<"隐藏状态，执行开启";
        ui->uploadProgressBar->setVisible(true);
        ui->uploadProgressBar->show();
        QApplication::processEvents(); // 强制处理事件循环
    }
    ui->uploadProgressBar->setValue(progress);
    //到达一百后设置隐藏
    if(progress == 100){
        QThread::msleep(2000);
        ui->uploadProgressBar->setVisible(false);
        //上传完毕后刷新远程目录
        if(isUpdateRemote){
            LoginSession loginSession = sshConnectMap.value(ui->SshTabWidget->currentIndex());
            sendPwdCommand(loginSession);
        }
//        if(isOpen){ //打开状态为true时才打开目录/文件
//            if(FileUtil::isDir(localPath)){//目录类型，使用explorer打开
//                //            // 不可编辑文件，使用本地关联程序打开
//                //            // QProcess::startDetached("xdg-open", {filePath}); // 对于 Linux 系统
//                //            // 在 Windows 系统中，可以使用以下代码
////                qDebug()<<"目录打开路径："<< EXE_APPLICATION_PATH+"/download"+path;
////                QString fullPath =  EXE_APPLICATION_PATH+"/download"+path;
//                localPath.replace("/","\\");
//                qDebug()<<"目录打开路径："<< localPath;
//                QProcess::startDetached("explorer", {localPath});
//    //            QProcess::startDetached("explorer", {EXE_APPLICATION_PATH+"/download"+path});
//                //            // 在 macOS 系统中，可以使用以下代码
//                //            // QProcess::startDetached("open", {filePath});
//            }else if(FileUtil::isFile(localPath)){ //文件类型，使用win自带/自定义编辑器打开
//                if(FileUtil::isFileEditable(localPath)){//是可编辑文本类型，使用编辑器打开
//                    QString content = FileUtil::readFile(localPath);
//                    qDebug()<<"文件内容："<<content;
//                    notepad.data()->setCurrentFile(localPath,content);
//                    notepad.data()->show();
//                }else{//使用win自带默认程序打开
//                   // 检查路径是否存在
//                   if (!QFile::exists(localPath)) {
//                       qDebug() << "Path does not exist:" << localPath;
//                       return ;
//                   }
//                   // 创建 QUrl 对象
//                   QUrl url = QUrl::fromLocalFile(localPath);
//                   // 尝试打开 URL
//                   if (!QDesktopServices::openUrl(url)) {
//                       qDebug() << "Failed to open URL:" << url.toString();
//                   }
//                }
//            }
//        }
    }
}
void MainWindow::updateDownloadProgressBar(int progress,QString localPath,bool isOpen){
    //设置进度条进度
    if (ui->downloadProgressBar->isHidden()) {
        // 如果进度条隐藏，首次接收到进度更新时显示它
        qDebug()<<"隐藏状态，执行开启";
        ui->downloadProgressBar->setVisible(true);
        ui->downloadProgressBar->show();
        QApplication::processEvents(); // 强制处理事件循环
    }
//    qDebug()<<"当前进度："<<progress;
    ui->downloadProgressBar->setValue(progress);
    //到达一百后设置隐藏
    if(progress == 100){
        QThread::msleep(2000);
        ui->downloadProgressBar->setVisible(false);
        if(isOpen){ //打开状态为true时才打开目录/文件
            if(FileUtil::isDir(localPath)){//目录类型，使用explorer打开
                //            // 不可编辑文件，使用本地关联程序打开
                //            // QProcess::startDetached("xdg-open", {filePath}); // 对于 Linux 系统
                //            // 在 Windows 系统中，可以使用以下代码
//                qDebug()<<"目录打开路径："<< EXE_APPLICATION_PATH+"/download"+path;
//                QString fullPath =  EXE_APPLICATION_PATH+"/download"+path;
                localPath.replace("/","\\");
                qDebug()<<"目录打开路径："<< localPath;
                QProcess::startDetached("explorer", {localPath});
    //            QProcess::startDetached("explorer", {EXE_APPLICATION_PATH+"/download"+path});
                //            // 在 macOS 系统中，可以使用以下代码
                //            // QProcess::startDetached("open", {filePath});
            }else if(FileUtil::isFile(localPath)){ //文件类型，使用win自带/自定义编辑器打开
                if(FileUtil::isFileEditable(localPath)){//是可编辑文本类型，使用编辑器打开
                    QString content = FileUtil::readFile(localPath);
                    qDebug()<<"文件内容："<<content;
                    notepad.data()->setCurrentFile(localPath,content);
                    notepad.data()->show();
                }else{//使用win自带默认程序打开
                   // 检查路径是否存在
                   if (!QFile::exists(localPath)) {
                       qDebug() << "Path does not exist:" << localPath;
                       return ;
                   }
                   // 创建 QUrl 对象
                   QUrl url = QUrl::fromLocalFile(localPath);
                   // 尝试打开 URL
                   if (!QDesktopServices::openUrl(url)) {
                       qDebug() << "Failed to open URL:" << url.toString();
                   }
                }
            }
        }
    }
}

// 事件过滤器函数实现
bool MainWindow::eventFilter(QObject *target, QEvent *event) {
//    qDebug() << "触发了鼠标、快捷键事件过滤器:" << target;
    // 如果事件来自文件树
    if (target == ui->remoteFileTab && event->type() == QEvent::KeyPress) {
        // 将事件转换为键盘事件
        QKeyEvent *k = static_cast<QKeyEvent *>(event);
        // 如果按下F2键
        if (k->key() == Qt::Key_F2) {
            qDebug()<<"捕获到f2";
            // 调用对应的修改文件名方法
//            modifyFileName();
//            modifyRemoteFileName("test");
//            return true;// 事件已处理
        }
    }
    // 如果事件来自主体标签窗体（键盘事件）
    if (target == ui->SshTabWidget && event->type() == QEvent::KeyPress) {
        // 将事件转换为键盘事件
        QKeyEvent *k = static_cast<QKeyEvent *>(event);
        // 如果同时按下control键和w键
        if (k->modifiers() == Qt::ControlModifier && k->key() == Qt::Key_W) {
            //关闭当前标签
            if (ui->SshTabWidget->currentIndex() != 0) {
                closeTab(ui->SshTabWidget->currentIndex());
            }
            return true;//事件已被处理
        }
        // 如果同时按下control键和q键
        if (k->modifiers() == Qt::ControlModifier && k->key() == Qt::Key_Q) {
            //跳转上一个标签
            if (ui->SshTabWidget->currentIndex() - 1 != 0) {
                ui->SshTabWidget->setCurrentIndex(ui->SshTabWidget->currentIndex() - 1);
            }
            return true;//事件已被处理
        }
        // 如果同时按下control键和tab键
        if (k->modifiers() == Qt::ControlModifier && k->key() == Qt::Key_Tab) {
            //跳转下一个标签
            if (ui->SshTabWidget->currentIndex() + 1 != ui->SshTabWidget->count() - 1) {
                ui->SshTabWidget->setCurrentIndex(ui->SshTabWidget->currentIndex() + 1);
            }
            return true;//事件已被处理
        }

    }
    //如果是鼠标点击事件，让其获取到焦点
    if (event->type() == QEvent::MouseButtonPress) {
        QWidget *clickedWidget = qobject_cast<QWidget *>(target);
        if (clickedWidget) {
            // 让被点击的 QWidget 获取焦点
            clickedWidget->setFocus();
            qDebug() << "Widget got focus: " << clickedWidget->objectName();
            return true;
        }
    }
    // 如果不满足上述条件，调用父类的事件过滤器
    return QWidget::eventFilter(target, event);
}
// 处理标签页关闭事件
void MainWindow::closeTab(int index) {
    //手动删除线程
    deleteReadThread(index);
    // 移除指定索引的标签页
    ui->SshTabWidget->removeTab(index);
    if (ui->SshTabWidget->currentIndex() == ui->SshTabWidget->count() - 1) {
        //进行还原操作：让索引往前一个标签移动，最后一个标签用来做新增操作
        ui->SshTabWidget->setCurrentIndex(ui->SshTabWidget->currentIndex() - 1);
    }else{
        qDebug()<<"当前删除的索引为终止索引为："<<index;
        qDebug()<<"tab总数："<<ui->SshTabWidget->count();

        //保持删除操作：删除成功，在删除标签后面位置的标签对应的loginSession索引跟着移动
        mapIndexSubtractOne(index);
        //同时读线程ReadThread的loginSession也需要更新，否则发送的index错误
        for(int i= 0;i<ui->SshTabWidget->count();i++){
            CustomizeCmdTextEdit* currentTextEdit =  getTab(i);
            if(currentTextEdit!=nullptr){
                qDebug()<<"-870-当前索引值："<<currentTextEdit->readThread->loginSession.index;
                getTab(i)->readThread->loginSession.index = sshConnectMap.value(i).index;
            }
        }
    }
}
//移动index之后索引的元素向前一个索引值
void MainWindow::mapIndexSubtractOne(int index){
    QMap<int, LoginSession> tempMap;
    // 先将小于等于 index 的键值对复制到临时 map 中
    for (auto it = sshConnectMap.begin(); it != sshConnectMap.end(); ++it) {
        if (it.key() <= index) {
            tempMap.insert(it.key(), it.value());
        }
    }
    // 处理大于 index 的键值对，将键减 1 后插入临时 sshConnectMap 中
    for (auto it = sshConnectMap.begin(); it != sshConnectMap.end(); ++it) {
        if (it.key() > index) {
            it.value().index = it.value().index-1;
            tempMap.insert(it.key() - 1, it.value());
        }
    }
    // 用临时 map 替换原 map
    sshConnectMap = tempMap;
}
// 处理标签页点击事件
void MainWindow::clickTab(int index) {
    if (index == ui->SshTabWidget->count() - 1) {
        //在末尾处，添加一个新标签
        addTab(ui->SshTabWidget->count() - 1, "/home/test" + QString::number(ui->SshTabWidget->currentIndex()));
    }
    qDebug() << "----------点击主体标签";
    //重新获取焦点，让其触发对应事件过滤器
    ui->SshTabWidget->setFocus();
}
/**
 * 添加一个新标签
 * @brief MainWindow::addTab
 * @param index：添加的位置索引
 * @param tabName：标签名
 * @return ：返回值是添加后的CustomizeCmdTextEdit对象
 */
CustomizeCmdTextEdit *MainWindow::addTab(int index, QString tabName) {
    // 创建新的标签页
    QWidget *newTab = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(newTab);
//        QLabel *label = new QLabel("/home/test/" + QString::number(ui->SshTabWidget->currentIndex()), newTab);
//        CommandWindow* cmdWidget=new CommandWindow(newTab);
//        cmdWidget->setCmdWinTxt("初始化");
//        layout->addWidget(cmdWidget);
    // 创建 QTextEdit 并填入初始化值
    // 创建自定义 QTextEdit 并填入初始化值
    // 设置布局边距为 0，使Layout内部的QTextEdit 紧贴边缘
    layout->setContentsMargins(0, 0, 0, 0);
    CustomizeCmdTextEdit *textEdit = new CustomizeCmdTextEdit("", newTab);
    textEdit->setObjectName(tabName);
    textEdit->setAutoFillBackground(true);
    textEdit->setLineWrapMode(QTextEdit::NoWrap);
    // 去除边框
    textEdit->setStyleSheet("border: none;");
    // 设置 QTextEdit 的大小策略为可扩展
    textEdit->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
//          // 设置 QTextEdit 的固定大小为 500x400
//          textEdit->setFixedSize(500, 400);
    // 将 QTextEdit 添加到布局中，并设置拉伸因子为 1，让它在布局中占据尽可能多的空间。
    layout->addWidget(textEdit, 1);
//        layout->addWidget(label);
    //设置布局
    newTab->setLayout(layout);
    // 设置新标签页里的主窗口的样式
    newTab->setStyleSheet("background-color: black; color: white;");
    //新加的标签添加事件过滤器：获取焦点，获取焦点后才能使其触发事件过滤器
    newTab->installEventFilter(this);
    // 添加新的标签页
    int newTabIndex = ui->SshTabWidget->insertTab(index, newTab, tabName);
    //切换到当前标签页
    ui->SshTabWidget->setCurrentIndex(newTabIndex);
    //标签颜色重新渲染
    tabCountChange();
    return textEdit;
}
void MainWindow::appendContent(int index,QString content){
//    if(content.contains(signalStr)){//非标志字符串命令则发送信号进行输出

//    }else{
        CustomizeCmdTextEdit* currentTextEdit = getTab(index);
        currentTextEdit->insertPlainTextByEncryptStatus(content);
        setTextEditCursor(ui->SshTabWidget->currentIndex());
        //密文输入完之后需要清空原密文
        //执行密文命令之后，需要将原密文清空
        if(currentTextEdit->isEncryptMode){ //处于开启密文状态，由于appendContent是执行命令后的追加内容，所以顺序来讲，这是第一次的进入密文状态后执行的命令之后的回显结果，进行清空密文操作
            currentTextEdit->encryptedSubStr = QString("");
        }
        //如果处于密文开启状态，并且回显结果是]$ 或 ]#则认为是退出密文命令，则进行密文关闭操作
        if(currentTextEdit->isEncryptMode == true && (content.trimmed().endsWith("]$")
                                                      || content.trimmed().endsWith("]#"))){//处于密文模式下，并且上一条命令是su/sudo命令，并且已经执行密文输入操作之后的追加处于用户输入]$状态，则需要关闭密文模式
           currentTextEdit->closeEncrypt();
           if(!content.contains("鉴定故障")){
               //重新登录用户，需要更新对应的sftpSession，更新对应loginSession里的参数
               LoginSession loginSession = sshConnectMap.value(index);
               loginSession.loginUser.loginUserName = currentTextEdit->tempUserName;
               loginSession.loginUser.loginUserPassword = currentTextEdit->tempPassword;
               SftpConnectThread* sftpConnectThread = new SftpConnectThread(loginSession,this);
               connect(sftpConnectThread, &SftpConnectThread::updateSftpSession, this, &MainWindow::updateSftpSession, Qt::QueuedConnection);
               //启动文件路径变化监控线程
               sftpConnectThread->start();
           }
        }
//    }

}
/**
 * 获取索引处的标签（即TabWidget下的QWidget下的CustomizeCmdTextEdit）
 * @brief MainWindow::getTab
 * @param idex
 * @return
 */
CustomizeCmdTextEdit *MainWindow::getTab(int index) {
    // 获取指定索引处的 QWidget
    QWidget *tabWidgetPage = ui->SshTabWidget->widget(index);
    if (tabWidgetPage) {
        // 尝试将 QWidget 转换为 CustomizeCmdTextEdit 对象
        CustomizeCmdTextEdit *textEdit = tabWidgetPage->findChild<CustomizeCmdTextEdit *>();
//        CustomizeCmdTextEdit* textEdit = qobject_cast<CustomizeCmdTextEdit*>(tabWidgetPage->findChild<QTextEdit*>());
//        qDebug() << "当前线程ID: " << reinterpret_cast<qint64>(QThread::currentThreadId());
        if (!textEdit) {
            qDebug() << "Error: QTextEdit is nullptr!";
            return nullptr;
        }
        // 进一步检查对象是否有效
        if (textEdit->objectName().isEmpty()) {
            qDebug() << "Error: QTextEdit is invalid!";
            return nullptr;
        }
//        qDebug() << "控件所属线程：" << reinterpret_cast<qint64>(textEdit->writeThread->currentThreadId());
        return textEdit;
    }
    return nullptr;
}
//标签页数量变化（新增/减少）后要进行的标签颜色重新渲染操作
void MainWindow::tabCountChange() {
    if (ui->SshTabWidget->count() == 2) { //标签页数为2，说明回到初始化状态，回到白色
        ui->SshTabWidget->setStyleSheet("");
        ui->SshTabWidget->tabBar()->setStyleSheet("");
    } else { //有新增标签页情况，需要渲染为黑色
        ui->SshTabWidget->tabBar()->setStyleSheet("QTabBar::tab:selected { border: 1px solid #ccc; /* 标签边框 */ border-top-left-radius: 10px; /* 左上角圆角 */ border-top-right-radius: 10px; /* 右上角圆角 */background-color: black; color: white; "
                "}");
    }
}

//标签页进行切换后触发的事件：处理颜色重新渲染
void MainWindow::tabSwitch(int index) {
    qDebug() << "标签变化，切换的索引值:" << index << "，所在的索引值：" << ui->SshTabWidget->currentIndex();
    if (index == 0 || index == ui->SshTabWidget->count() - 1) { //切换的是初始化标签，或者结束标签
        ui->SshTabWidget->setStyleSheet("");
        ui->SshTabWidget->tabBar()->setStyleSheet("");
    } else { //切换的是非初始化标签
        ui->SshTabWidget->tabBar()->setStyleSheet("QTabBar::tab:selected { border: 1px solid #ccc; /* 标签边框 */ border-top-left-radius: 10px; /* 左上角圆角 */ border-top-right-radius: 10px; /* 右上角圆角 */background-color: black; color: white; }");
    }
}


/**
 * 递归遍历远程目录的函数
 * @brief QSshSocket::traverseRemoteDirectory
 * @param sftp: 指向 LIBSSH2_SFTP 结构体的指针，代表一个 SFTP 会话
 * @param path: 要遍历的远程目录的路径，使用 QString 类型表示
 * @param parentItem: 指向 QStandardItem 的指针，用于存储当前目录下的文件和子目录信息
 */
void MainWindow::traverseRemoteDirectory(LIBSSH2_SESSION *session,LIBSSH2_SFTP* sftp, const QString& path, QStandardItem* parentItem) {
    // 使用 libssh2_sftp_opendir 函数打开指定路径的远程目录
    // path.toUtf8().constData() 将 QString 类型的路径转换为 const char* 类型，以满足 libssh2 函数的参数要求
    // 该函数返回一个指向 LIBSSH2_SFTP_HANDLE 结构体的指针，代表打开的目录句柄
    LIBSSH2_SFTP_HANDLE* handle = libssh2_sftp_opendir(sftp, path.toUtf8().constData());
    // 检查目录是否成功打开
    if (!handle) {
        QSshSocket sshSocket;
        QString error = sshSocket.getSessionLastErrot(session);
        qDebug()<<"sftp get handle fail："<<error;
        //打印sftp报错代码
        unsigned long errcode = libssh2_sftp_last_error(sftp);
        qDebug()<<"sftp报错码："<<errcode;
        // 函数返回，终止本次遍历操作
        return;
    }

    // 定义一个 LIBSSH2_SFTP_ATTRIBUTES 结构体变量，用于存储文件或目录的属性信息
    LIBSSH2_SFTP_ATTRIBUTES attrs;
    // 定义一个字符数组，用于存储从远程目录读取的文件名
    char filename[512];
    QFileIconProvider iconProvider;

    // 进入一个无限循环，用于逐行读取目录中的文件和子目录信息
    while (1) {
        // 使用 libssh2_sftp_readdir 函数从打开的目录中读取下一个文件或子目录的信息
        // 该函数会将文件名存储到 filename 数组中，并将文件属性存储到 attrs 结构体中
        // 函数返回值 rc 表示读取的结果：
        // 大于 0 表示成功读取到一个文件或子目录
        // 等于 0 表示已经读取完目录中的所有条目
        // 小于 0 表示读取过程中出现错误
        int rc = libssh2_sftp_readdir(handle, filename, sizeof(filename), &attrs);
        // 检查读取结果
        if (rc <= 0) {
            // 如果 rc 小于等于 0，说明已经读取完目录或者出现错误，跳出循环
            break;
        }

        // 将从远程目录读取的文件名（char 数组）转换为 QString 类型，方便后续处理
        QString qFilename = QString::fromUtf8(filename);
        // 检查文件名是否为 "." 或 ".."
        // "." 表示当前目录，".." 表示父目录，通常不需要将它们添加到显示列表中
        if (qFilename == "." || qFilename == "..") {
            // 如果是 "." 或 ".."，跳过本次循环，继续读取下一个文件或子目录
            continue;
        }

        // 创建一个新的 QStandardItem 对象，用于存储当前读取到的文件名
        QStandardItem* nameItem = new QStandardItem(qFilename);
        //设置当前列不能双击编辑
        nameItem->setFlags(Qt::ItemIsEnabled);
        // 存储旧文件名到 Qt::UserRole（后面的itemChange方法要用）
        nameItem->setData(qFilename, Qt::UserRole);
        // 根据文件类型设置图标
        if (attrs.permissions & LIBSSH2_SFTP_S_IFDIR) {
            nameItem->setIcon(iconProvider.icon(QFileIconProvider::Folder));
        } else {
            nameItem->setIcon(iconProvider.icon(QFileIconProvider::File));
        }

        // 创建用于存储文件大小的 QStandardItem
        QStandardItem* sizeItem = new QStandardItem(StringUtil::byteCountToSize(attrs.filesize));
        //设置当前列不能双击编辑
        sizeItem->setFlags(Qt::ItemIsEnabled);
        // 创建用于存储文件上次更新时间的 QStandardItem
        QDateTime lastModified = QDateTime::fromSecsSinceEpoch(attrs.mtime);
        QStandardItem* lastModifiedItem = new QStandardItem(lastModified.toString("yyyy-MM-dd HH:mm:ss"));
        //设置当前列不能双击编辑
        lastModifiedItem->setFlags(Qt::ItemIsEnabled);
        // 创建用于存储文件权限的 QStandardItem
        QString permissions;
        if (attrs.permissions & LIBSSH2_SFTP_S_IRUSR) permissions += "r"; else permissions += "-";
        if (attrs.permissions & LIBSSH2_SFTP_S_IWUSR) permissions += "w"; else permissions += "-";
        if (attrs.permissions & LIBSSH2_SFTP_S_IXUSR) permissions += "x"; else permissions += "-";
        if (attrs.permissions & LIBSSH2_SFTP_S_IRGRP) permissions += "r"; else permissions += "-";
        if (attrs.permissions & LIBSSH2_SFTP_S_IWGRP) permissions += "w"; else permissions += "-";
        if (attrs.permissions & LIBSSH2_SFTP_S_IXGRP) permissions += "x"; else permissions += "-";
        if (attrs.permissions & LIBSSH2_SFTP_S_IROTH) permissions += "r"; else permissions += "-";
        if (attrs.permissions & LIBSSH2_SFTP_S_IWOTH) permissions += "w"; else permissions += "-";
        if (attrs.permissions & LIBSSH2_SFTP_S_IXOTH) permissions += "x"; else permissions += "-";
        QStandardItem* permissionItem = new QStandardItem(permissions);
        //设置当前列不能双击编辑
        permissionItem->setFlags(Qt::ItemIsEnabled);
        // 创建用于存储文件所有人的 QStandardItem
        QStandardItem* ownerItem = new QStandardItem(QString::number(attrs.uid));
        //设置当前列不能双击编辑
        ownerItem->setFlags(Qt::ItemIsEnabled);

        // 将这些 QStandardItem 添加到同一行
        QList<QStandardItem*> items;
        items << nameItem << sizeItem << lastModifiedItem << permissionItem << ownerItem;
        parentItem->appendRow(items);

        // 检查当前读取到的条目是否为目录
        // 通过检查 attrs.permissions 中的标志位 LIBSSH2_SFTP_S_IFDIR 来判断
        if (attrs.permissions & LIBSSH2_SFTP_S_IFDIR) {
            // 如果是目录，构建新的目录路径
            // 将当前目录路径 path 和子目录名 qFilename 拼接起来，中间用 "/" 分隔
            QString newPath = path + "/" + qFilename;
            // 递归调用 traverseRemoteDirectory 函数，继续遍历新的子目录
            // 传递新的目录路径和新创建的 QStandardItem 对象作为参数
            traverseRemoteDirectory(session,sftp, newPath, nameItem);
        }
    }

    // 当目录遍历完成后，使用 libssh2_sftp_closedir 函数关闭打开的目录句柄
    // 释放相关资源，避免资源泄漏
    libssh2_sftp_closedir(handle);
}

// QTreeView 获取目录回显
void MainWindow::getPathQTreeView(LIBSSH2_SESSION *session,LIBSSH2_SFTP* sftp, const QString& path, QTreeView* treeView) {
//    QMutexLocker locker(&mutex);

    // 创建 QStandardItemModel 并动态分配内存，指针类型可以在函数结束后仍然存在于内存，引用类型则局部显示，函数结束后被销毁
    QStandardItemModel* model = new QStandardItemModel(treeView);


    // 设置列标题
    model->setHorizontalHeaderLabels({"文件名", "文件大小", "文件上次更新时间", "文件权限", "文件所有人"});
    QStandardItem* rootItem = model->invisibleRootItem();

    // 遍历远程目录
    traverseRemoteDirectory(session ,sftp, path, rootItem);

    // 设置模型到 QTreeView
    treeView->setModel(model);

    // 自定义选中状态样式
    treeView->setStyleSheet(
        "QTreeView::item:selected {"
        "    background-color: #3399FF;"
        "    color: white;"
        "}"
    );
    //设置右键目录/文件时失去焦点后的依然有虚线框的选中状态
    treeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
    treeView->show();


    // 连接 itemChanged 信号到槽函数，文件名修改时触发
    connect(model, &QStandardItemModel::itemChanged, this, &MainWindow::itemChanged,Qt::QueuedConnection);
//    // 关闭 SFTP 会话
//    if (sftp) {
//       int shutdownResult = libssh2_sftp_shutdown(sftp);
//       if (shutdownResult != 0) {
//           qDebug() << "Failed to shutdown SFTP session, error code: " << shutdownResult;
//       }
//    }
}
MainWindow::~MainWindow() {
    delete ui;
}

