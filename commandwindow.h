#ifndef COMMANDWINDOW_H
#define COMMANDWINDOW_H

#include <QWidget>
#include <QString>

namespace Ui {
class CommandWindow;
}

class CommandWindow : public QWidget
{
    Q_OBJECT

public:
    explicit CommandWindow(QWidget *parent = nullptr);

    //设置窗口文本
    void setCmdWinTxt(QString txt);
    //追加窗口文本
    void appendCmdWinText(QString txt);
    ~CommandWindow();

private:
    Ui::CommandWindow *ui;
};

#endif // COMMANDWINDOW_H
