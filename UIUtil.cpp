#include <QApplication>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>
#include <QInputDialog>
#include <QDesktopWidget>
#include <QDebug>
#include <QMutex>

// 定义 UIUtil 工具类
class UIUtil {
private:
    static QMutex mutex;
public:
    // 将光标设置到文本末尾并使 QTextEdit 获得焦点
    static void setQTextEditCursorEndBlink(QTextEdit *textEdit) {
        if (textEdit) {
            QTextCursor cursor = textEdit->textCursor();
            cursor.movePosition(QTextCursor::End);
            textEdit->setTextCursor(cursor);
            textEdit->setFocus();
        }
    }
    //构造一个对话框
    static QString makeInputDialog(QString title, QString prompt, QString text) {
        QMutexLocker locker(&mutex);
        bool ok;
        QInputDialog dialog;
        dialog.setWindowTitle(title);
        dialog.setLabelText(prompt);
        dialog.setTextValue(text);
        // 获取屏幕可用区域
        QDesktopWidget desktop;
        QRect screenGeometry = desktop.availableGeometry();

        // 获取对话框的建议大小
        QSize dialogSize = dialog.sizeHint() / 2;
        // 将 QSize 转换为 QPoint
        QPoint dialogSizeAsPoint(dialogSize.width(), dialogSize.height());
        // 确保对话框在屏幕内
        QPoint pos = screenGeometry.center() - dialogSizeAsPoint;
        dialog.move(pos);

        if (dialog.exec() == QDialog::Accepted) {
            QString newFileName = dialog.textValue();
//            if (!newFileName.isEmpty()) {
//                qDebug() << "新文件名: " << newFileName;
//            }
            return newFileName;
        }else{
            return "";
        }
    }
};
