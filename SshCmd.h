#ifndef SSHCMD_H
#define SSHCMD_H


#include <QString>
#include <QDateTime>


class SshCmd {
public:
    QString sshCmdId; //cmd命令的主键id
    QString sshCmdContent;  //执行的cmd命令内容
    QString sshCmdExecuteUser; // cmd命令的执行人
    QDateTime sshCmdExecuteTime; //cmd命令的执行时间

    SshCmd(const QString & sshCmdId, const QString &sshCmdContent, const QString & sshCmdExecuteUser, const QDateTime &sshCmdExecuteTime){
        this->sshCmdId = sshCmdId;
        this->sshCmdContent = sshCmdContent;
        this->sshCmdExecuteUser = sshCmdExecuteUser;
        this->sshCmdExecuteTime = sshCmdExecuteTime;
    }

    //析构函数必须按照规范，自己定义delete this会导致野指针问题：19:49:38: 程序异常结束。 19:49:38: The process was ended forcefully.
    ~SshCmd(){}

};


#endif // SSHCMD_H
