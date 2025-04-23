#include "commandwindow.h"
#include "ui_commandwindow.h"

CommandWindow::CommandWindow(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CommandWindow)
{
    ui->setupUi(this);

    this->setAutoFillBackground(true);
    ui->textEdit->setAutoFillBackground(true);
    ui->textEdit->setLineWrapMode(QTextEdit::NoWrap);
}
//设置窗口文本
void CommandWindow::setCmdWinTxt(QString txt){
    ui->textEdit->clear();
    appendCmdWinText(txt);
}
//追加窗口文本
void CommandWindow::appendCmdWinText(QString txt){
    ui->textEdit->append(txt);

}
CommandWindow::~CommandWindow()
{
    delete ui;
}
