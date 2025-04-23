#include <QDebug>
#include <QString>
#include <QObject>
#include <QFile>
#include <QDir>
#include <QQueue>

#include <windows.h>
#include <aclapi.h>

#include <QDir>
#include <QDebug>
#include <QFileInfoList>

#include <QUrl>
#include <QDesktopServices>
#include <QIcon>
#include <QFileIconProvider>
#include <QQueue>

#include <TreeNode.h>


////定义树节点类
//class TreeNode {
//public:
//    QString path;  // 节点对应的文件或目录路径
//    bool isDirectory;  // 标记节点是否为目录
//    QIcon icon;  // 节点对应的图标
//    QList<TreeNode *> children; // 子节点列表

//    TreeNode(const QString &path, bool isDir) : path(path), isDirectory(isDir) {
//        QFileIconProvider iconProvider;
//        QFileInfo fileInfo(path);
//        icon = iconProvider.icon(fileInfo);
//    }

//    ~TreeNode() {
//        // 释放子节点内存
//        for (TreeNode * child : children) {
//            delete child;
//        }
//    }
//};

//静态文件工具类
class FileUtil {

public:

    /**
     * 使用深度优先搜索获取指定目录下所有exe文件的完整路径
     * @brief depthFirstSearchExe
     * @param dirPath 起始目录路径
     * @param exeList 用于存储找到的exe文件路径的列表
     */
    static void findExeByDeep(const QString &dirPath, QList<QString> &exeList) {
        QDir dir(dirPath);
        if (!dir.exists()) {
            return;
        }
        // 获取目录下所有条目
        QFileInfoList entryList = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
        for (const QFileInfo &info : entryList) {
            if (info.isDir()) {
                // 若是目录，则递归进入
                findExeByDeep(info.absoluteFilePath(), exeList);
            } else if (info.isFile() && info.suffix() == "exe") {
                // 若是exe文件，加入列表
                exeList.append(info.absoluteFilePath().replace("/", "\\"));
            }
        }
    }
    /**
     * 使用广度优先搜索获取指定目录下所有exe文件的完整路径
     * @brief breadthFirstSearchExe
     * @param dirPath 起始目录路径
     * @param exeList 用于存储找到的exe文件路径的列表
     */
    static void findExeByWide(const QString &dirPath, QList<QString> &exeList) {
        QQueue<QString> queue;
        queue.enqueue(dirPath);
        while (!queue.isEmpty()) {
            QString currentDir = queue.dequeue();
            QDir dir(currentDir);
            if (!dir.exists()) {
                continue;
            }
            QFileInfoList entryList = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
            for (const QFileInfo &info : entryList) {
                if (info.isDir()) {
                    queue.enqueue(info.absoluteFilePath());
                } else if (info.isFile() && info.suffix() == "exe") {
                    exeList.append(info.absoluteFilePath().replace("/", "\\"));
                }
            }
        }
    }
    /**
     * 使用windows自带打开程序打开文件（txt默认使用记事本，与系统保持一致）
     * @brief openFile
     * @param fileName
     * @return
     */
    static bool openFile(const QString &fileName) {
        // 使用 QDesktopServices::openUrl 打开文件
        QUrl fileUrl = QUrl::fromLocalFile(fileName);
        if (QDesktopServices::openUrl(fileUrl)) {
//            qDebug() << "文件打开成功：" << fileName;
            return true;
        } else {
//            qDebug() << "无法打开文件：" << fileName;
            return false;
        }
    }
    /**
     * 判断文件是否存在，不存在则创建
     * @brief createFileIfNotExist
     * @param fileName 文件名，包含路径
     * @return true 如果文件创建成功或者原本就存在，false 如果创建失败
     */
    static bool createFile(const QString &fileName) {
        QFile file(fileName);
        if (file.exists()) {
            return true;
        }
        // 获取文件所在目录路径
        QString dirPath = QFileInfo(fileName).dir().path();
        QDir dir(dirPath);
        // 如果目录不存在，先创建目录
        if (!dir.exists()) {
            if (!dir.mkpath(dirPath)) {
                qDebug() << "无法创建目录: " << dirPath;
                return false;
            }
        }
        // 创建文件
        if (file.open(QIODevice::ReadWrite | QIODevice::NewOnly | QIODevice::Text)) {
            file.close();
            return true;
        } else {
            qDebug() << "无法创建文件，错误：" << file.errorString();
            return false;
        }
    }
    /**
     * 判断目录是否存在，不存在则创建
     * @brief createDir
     * @param fileName 目录
     * @return true 如果文件创建成功或者原本就存在，false 如果创建失败
     */
    static bool createDir(const QString &path) {
        QDir dir(path);
        // 如果目录不存在，先创建目录
        if (!dir.exists()) {
            if (!dir.mkpath(path)) {
                qDebug() << "无法创建目录: " << path;
                return false;
            }
        }
        return true;
    }
    /**
     * 判断是否是文件
     * @brief isFile
     * @param fileName
     * @return
     */
    static bool isFile(const QString &fileName) {
        QFileInfo info(fileName);
        return info.isFile();
    }

    /**
     * 判断是否是目录
     * @brief isDir
     * @param fileName
     * @return
     */
    static bool isDir(const QString &fileName) {
        QFileInfo info(fileName);
        return info.isDir();
    }

    /**
     * 判断目录/文件是否存在
     * @brief isFileOrDirExist
     * @param path
     * @return
     */
    static bool isExist(const QString &path) {
        if (isFile(path)) {
            qDebug() << "目录类型";
            QFile file(path);
            if (file.exists()) {
                qDebug() << "目录存在";
                return true;
            } else {
                qDebug() << "目录不存在";
                return false;
            }
        }
        if (isDir(path)) {
            qDebug() << "文件类型";
            QDir dir(path);
            if (dir.exists()) {
                qDebug() << "文件存在";
                return true;
            } else {
                qDebug() << "文件不存在";
                return false;
            }
        }
    }

    /**
     * 判断文件是否可读
     * @brief isDir
     * @param fileName
     * @return
     */
    static bool isReadable(const QString &fileName) {
        QFile file(fileName);
        return file.isReadable();
    }
    /**
     * 判断文件是否可写
     * @brief isDir
     * @param fileName
     * @return
     */
    static bool isWritable(const QString &fileName) {
        QFile file(fileName);
        return file.isWritable();
    }
    /**
     * 读取文件内容并返回
     * @brief MainWindow::readFile
     * @param fileName
     */
    static QString readFile(QString fileName) {
        QFile file(fileName);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
            file.seek(0);
            QTextStream in(&file);
            QString content = in.readAll();
            file.close();
            qDebug() << "文件工具类------读取文件名:" << fileName << "   ，读取到文件内容：" << content;
            return content;
        } else {
            qDebug() << "无法打开文件，错误：" << file.errorString();
            return QString();
        }
    }
    /**
     * 删除目录/文件
     * @brief deleteFile
     * @param filePath
     * @return
     */
    static bool deleteFileOrDir(QString path) {
        QFileInfo fileInfo(path);
        if (fileInfo.exists()) {
            if (fileInfo.isFile()) {
                QFile file(path);
                if (file.remove()) {
                    qDebug() << "文件 " << path << " 已成功删除";
                    return true;
                } else {
                    file.close();
                    qDebug() << "删除文件 " << path << " 失败，错误: " << file.errorString();
                    return false;
                }
            } else if (fileInfo.isDir()) {
                QDir dir(path);
                if (dir.removeRecursively()) {
                    qDebug() << "目录 " << path << " 已成功删除";
                    return true;
                } else {
                    // 由于 QDir 没有 errorString 方法，可通过 QFileDevice 获取通用错误信息
                    QFileDevice::FileError error = QFileDevice::NoError;
                    // 这里只是模拟获取错误信息，实际可能无法精准对应 QDir 操作的错误
                    QFile dummyFile;
                    error = dummyFile.error();
                    qDebug() << "删除目录 " << path << " 失败，错误: " << dummyFile.errorString();
                    return false;
                }
            }
        } else {
            qDebug() << "路径 " << path << " 不存在";
            return false;
        }
        return false;
    }
    /**
     * 清空目录里的所有目录、文件
     * @brief clearDir
     * @param path
     * @return
     */
    static bool clearDir(QString path) {
        QFileInfo fileInfo(path);
        if (fileInfo.exists()) {
            if (fileInfo.isFile()) {
                qDebug() << "路径是文件路径，非目录路径";
                return false;
            } else if (fileInfo.isDir()) {
                QDir dir(path);
                // 遍历目录中的所有条目
                QFileInfoList entries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
                for (const QFileInfo &entry : entries) {
                    if (entry.isFile()) {
                        // 如果是文件，直接删除
                        if (!QFile::remove(entry.absoluteFilePath())) {
                            qDebug() << "删除文件 " << entry.absoluteFilePath() << " 失败，错误: " << QFile(entry.absoluteFilePath()).errorString();
                            return false;
                        }
                    } else if (entry.isDir()) {
                        // 如果是目录，递归删除
                        QDir subDir(entry.absoluteFilePath());
                        if (!subDir.removeRecursively()) {
                            qDebug() << "删除目录 " << entry.absoluteFilePath() << " 失败，错误: " << QFile(entry.absoluteFilePath()).errorString();
                            return false;
                        }
                    }
                }
                qDebug() << "目录 " << path << " 已成功清空";
                return true;
            }
        } else {
            qDebug() << "路径 " << path << " 不存在";
            return false;
        }
        return false;
    }

    /**
     * 修改目录名/文件名
     * @brief modifyFileName
     * @param originalPath
     * @param newName
     * @return
     */
    static bool modifyFileName(QString originalPath, QString newName) {
        QFileInfo fileInfo(originalPath);
        if (!fileInfo.exists()) {
            qDebug() << "路径 " << originalPath << " 不存在";
            return false;
        }
        QDir parentDir = fileInfo.dir();
        QString newPath = parentDir.absoluteFilePath(newName);
        if (fileInfo.isFile()) {
            QFile file(originalPath);
            if (file.rename(newPath)) {
                qDebug() << "文件 " << originalPath << " 已成功重命名为 " << newPath;
                return true;
            } else {
                qDebug() << "重命名文件 " << originalPath << " 失败，错误: " << file.errorString();
                return false;
            }
        } else if (fileInfo.isDir()) {
            QDir dir(originalPath);
            if (dir.rename(originalPath, newPath)) {
                qDebug() << "目录 " << originalPath << " 已成功重命名为 " << newPath;
                return true;
            } else {
                // 由于 QDir 没有 errorString 方法，通过 QFileDevice 获取通用错误信息
                QFile dummyFile;
                QFileDevice::FileError error = dummyFile.error();
                qDebug() << "重命名目录 " << originalPath << " 失败，错误: " << dummyFile.errorString();
                return false;
            }
        }
        return false;
    }
    /**
     * 将QString转换为LPWSTR（宽字符字符串），用于Windows API
     * @brief QStringToLPWSTR
     * @param qstr
     * @return
     */
    static LPWSTR QStringToLPWSTR(const QString &qstr) {
        QByteArray ba = qstr.toUtf8();
        LPWSTR lpwstr = new WCHAR[ba.length() + 1];
        MultiByteToWideChar(CP_UTF8, 0, ba.data(), ba.length(), lpwstr, ba.length() + 1);
        return lpwstr;
    }

    /**
     * 检查文件是否是文本文件类型，可编辑状态
     * @brief isFileEditable
     * @param filePath
     * @return
     */
    static bool isFileEditable(const QString& filePath) {
        QFileInfo fileInfo(filePath);
        QString suffix = fileInfo.suffix().toLower();

        // 定义可编辑文件后缀列表
        QStringList editableSuffixes = {
            "txt", "csv", "log", "ini", "conf", "xml", "json", "html", "css", "js"
        };

        return editableSuffixes.contains(suffix);
    }

};
