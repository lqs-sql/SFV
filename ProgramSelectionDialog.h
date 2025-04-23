#ifndef PROGRAMSELECTIONDIALOG_H
#define PROGRAMSELECTIONDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QStringList>

class ProgramSelectionDialog : public QDialog
{
    Q_OBJECT
public:
    ProgramSelectionDialog(const QString& filePath, QWidget *parent = nullptr);

private slots:
    // 修改槽函数参数类型
    void onItemDoubleClicked(QListWidgetItem *item);

private:
    void setupUI();
    void populateProgramList();
    void openFileWithProgram(const QString& programPath);

    QListWidget *m_listWidget;
    QStringList m_programList;
    QString m_filePath;
};


#endif // PROGRAMSELECTIONDIALOG_H
