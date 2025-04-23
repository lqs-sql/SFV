#ifndef CUSTOMIZECMDTEXTEDIT_H
#define CUSTOMIZECMDTEXTEDIT_H


#include <QMenu>
#include <QAction>
#include <QResizeEvent>
#include <QTextEdit>
#include <QDebug>

#include <QRegularExpression>
#include <QTextCharFormat>
#include <QTextCursor>
#include <QDebug>

#include <libssh2.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(dll, "ws2_32.dll")

#include "StringUtil.h"
#include "QSshSocket.h"
#include "ReadThread.h"



// 自定义 QTextEdit 类，重写右键菜单事件和大小改变事件
class CustomizeCmdTextEdit : public QTextEdit {
    Q_OBJECT
public:
    CustomizeCmdTextEdit(const QString &text, QWidget *parent = nullptr) : QTextEdit(text, parent) {
        // 连接 textChanged 信号到槽函数
        connect(this, &CustomizeCmdTextEdit::textChanged, this, &CustomizeCmdTextEdit::onTextChanged);
    }
    QString lastText;//上一次执行后的文本框显示内容,用来提取上一条命令使用
    bool isEncryptMode = false;//true-密文开启模式（子串替代成密文*）、false-密文关闭模式（正常输入））
    QString encryptedSubStr = ""; //被加密的内容子串，如密码输入时使用，将密码保存起来

    QThread *writeThread;
    QObject *writeWork;

    ReadThread* readThread;


    //临时存储变量
    QString tempUserName;
    QString tempPassword;


    void insertPlainTextByEncryptStatus(QString txt) {
        if(StringUtil::isNotBlank(txt.trimmed())){
            insertPlainText(txt);
            lastText = toPlainText();
            qDebug() << "\n命令行追加：" << txt;
        }
    }

    //开启密文模式
    void openEncrypt() {
        qDebug() << "开启密文模式!!";
        isEncryptMode = true;
//        lastText = toPlainText();
    }
    //关闭密文模式
    void closeEncrypt() {
        qDebug() << "关闭密文模式!!";
        isEncryptMode = false;
        encryptedSubStr = QString("");
    }
    //让光标移动到末尾位置
    void moveCursorToEndPos() {
        // 将光标移动到文本末尾
        QTextCursor cursor = textCursor();
        cursor.movePosition(QTextCursor::End);
        setTextCursor(cursor);
        // 插入文本后设置焦点（重要！设置后才能让光标闪烁）
        setFocus();
    }

protected:
    void contextMenuEvent(QContextMenuEvent *event) override {
        QMenu menu(this);
        // 添加菜单项
        QAction *copyAction = menu.addAction("复制");
        QAction *pasteAction = menu.addAction("粘贴");
        QAction *selectAllAction = menu.addAction("全选");
        // 连接菜单项的信号和槽
        connect(copyAction, &QAction::triggered, this, &QTextEdit::copy);
        connect(pasteAction, &QAction::triggered, this, &QTextEdit::paste);
        connect(selectAllAction, &QAction::triggered, this, &QTextEdit::selectAll);
        // 显示菜单
        menu.exec(event->globalPos());
    }

    void resizeEvent(QResizeEvent *event) override {
        QTextEdit::resizeEvent(event);
        // 根据当前宽度动态调整字体大小
        int width = event->size().width();
        int fontSize = qMax(10, width / 20); // 简单示例，可根据需求调整计算方式
        QFont font = this->font();
        font.setPointSize(fontSize / 3);
        this->setFont(font);
    }
    //鼠标光标可以自由移动，不一定要保持在最后位置，此段注释
//    void mousePressEvent(QMouseEvent *event) override {
//        qDebug()<<"mousePressEvent!";
//        // 阻止鼠标点击改变光标位置
//        QTextCursor cursor = textCursor();
//        cursor.movePosition(QTextCursor::End);
//        setTextCursor(cursor);
//        // 不调用父类的 mousePressEvent，避免默认行为
//    }
signals:
    // 自定义的回车信号
    void enterPressed();
    //自定义的ctrl+c信号
    void ctrlAndCPressed();

private slots:
    void onTextChanged() {
    }

protected:
    // 重写 keyPressEvent 方法
    //文本框每次按键触发
    void keyPressEvent(QKeyEvent *event) override {
//        qDebug() << "keyPressEvent！";
        QTextEdit::keyPressEvent(event);//原按键触发，比如按键a就是键入a字符


        //必须在原来功能 QTextEdit::keyPressEvent执行之后进行还原（避免多线程下的多次修改偶尔失效的问题，不能在 QTextEdit::keyPressEvent之前执行）
        if(event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace){
//            qDebug()<<"按下回车键，当前文件："<<toPlainText()<<"\n上次执行文本："<<lastText<<"\n判断结果："<<!toPlainText().startsWith(lastText);
            if(!toPlainText().trimmed().startsWith(lastText.trimmed())){
                setPlainText(lastText);
                moveCursorToEndPos();
            }
        }
        if(event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {//当按下回车键时，发出自定义信号
            emit enterPressed();
        }

        //必须在QTextEdit::keyPressEvent(event)之后执行，按键文本输入键入了之后
        if(isEncryptMode == true){//密文模式
            if (event->key() == Qt::Key_Backspace || event->key() == Qt::Key_Delete) {//回退/删除键，表示删除操作
                if (encryptedSubStr.length() > 0) {
                    encryptedSubStr.chop(1);//已记载的密文也需要删除最后一个字符
                }
            }else if(toPlainText().startsWith(lastText)){
                QString newSignStr = toPlainText();//获取当前所有文本内容
                encryptedSubStr.append(newSignStr.back()); //将待删除的最后一个字符，追加到密文串encryptedSubStr
                newSignStr.chop(1); //删除最后一个字符
                newSignStr.append("*"); //追加一个*显示
                setPlainText(newSignStr);
                //移动光标到最后
                moveCursorToEndPos();
            }
        }

        //如果同时按下control键和c键，重置当前窗口读线程
        if (event->modifiers() & Qt::ControlModifier && event->key() == Qt::Key_C) {
            emit ctrlAndCPressed();
        }

    }
};


#endif // CUSTOMIZECMDTEXTEDIT_H
