#include "ProgramSelectionDialog.h"
#include <QVBoxLayout>
#include <QProcess>
#include <QFileInfo>
#include <QDebug>
#include <QIcon>
#include <QStandardPaths>
#include <QDir>

ProgramSelectionDialog::ProgramSelectionDialog(const QString& filePath, QWidget *parent)
    : QDialog(parent), m_filePath(filePath)
{
    setupUI();
}

void ProgramSelectionDialog::setupUI()
{
    QVBoxLayout *layout = new QVBoxLayout(this);
    m_listWidget = new QListWidget(this);
    layout->addWidget(m_listWidget);

    populateProgramList();

    // 连接信号和槽
    connect(m_listWidget, &QListWidget::itemDoubleClicked, this, &ProgramSelectionDialog::onItemDoubleClicked);
}

void ProgramSelectionDialog::populateProgramList()
{
    QStringList systemPaths = QStandardPaths::standardLocations(QStandardPaths::ApplicationsLocation);
    for (const QString& path : systemPaths) {
        QDir dir(path);
        QStringList filters;
        filters << "*.exe"; // 仅考虑 Windows 可执行文件
        QFileInfoList fileInfoList = dir.entryInfoList(filters, QDir::Files);
        for (const QFileInfo& fileInfo : fileInfoList) {
            m_programList.append(fileInfo.absoluteFilePath());
            QListWidgetItem *item = new QListWidgetItem(QIcon::fromTheme("application-x-executable"), fileInfo.fileName());
            m_listWidget->addItem(item);
        }
    }
}

void ProgramSelectionDialog::onItemDoubleClicked(QListWidgetItem *item)
{
    // 获取当前项的索引
    int index = m_listWidget->row(item);
    QString programPath = m_programList[index];
    openFileWithProgram(programPath);
    accept();
}

void ProgramSelectionDialog::openFileWithProgram(const QString& programPath)
{
    QStringList arguments;
    arguments << m_filePath;
    if (QProcess::startDetached(programPath, arguments)) {
        qDebug() << "文件已使用 " << programPath << " 打开。";
    } else {
        qDebug() << "无法使用 " << programPath << " 打开文件。";
    }
}
