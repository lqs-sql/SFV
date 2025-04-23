#include <QObject>
#include <QProcess>
#include <QString>
#include <QByteArray>

class QProcessUtil
//        : public QObject
{
//    Q_OBJECT
public:
//    explicit QProcessUtil(QObject *parent = nullptr) : QObject(parent) {}

    // 执行命令并返回结果
    static QString executeCommand(const QString &command)
    {
        QProcess process;
        // 启动命令
        process.start(command);
        // 等待命令启动
        if (!process.waitForStarted()) {
            return "FailSign：Failed to start the process.";
        }
        // 等待命令执行完成
        if (!process.waitForFinished()) {
            return "FailSign：Process did not finish properly.";
        }
        // 读取标准输出
        QByteArray output = process.readAllStandardOutput();
        // 读取标准错误输出
        QByteArray error = process.readAllStandardError();

        // 如果有错误信息，返回错误信息
        if (!error.isEmpty()) {
            return QString("FailSign： %1").arg(QString::fromUtf8(error));
        }

        // 返回标准输出内容
        return QString::fromUtf8(output);
    }
};
