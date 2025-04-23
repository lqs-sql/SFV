#ifndef SSHCMDRESULT_H
#define SSHCMDRESULT_H



#include <QString>
#include <QDateTime>


class SshCmdResult {
public:
    QString sshCmdResultId; //返回内容id
    QString sshCmdResultContent;  //执行的cmd命令后的返回内容
    QString sshCmdResultExecuteUser; // cmd命令的执行人
    QDateTime sshCmdResultExecuteTime; //cmd命令的返回时间
    QString sshCmdId; //所属的cmd命令的主键id（即哪条cmd命令执行后的返回结果）

    SshCmdResult(const QString &sshCmdResultId,const QString &sshCmdResultContent, const QString & sshCmdResultExecuteUser, const QDateTime &sshCmdResultExecuteTime
                 , const QString &sshCmdId){
        this->sshCmdResultId = sshCmdResultId;
        this->sshCmdResultContent = sshCmdResultContent;
        this->sshCmdResultExecuteUser = sshCmdResultExecuteUser;
        this->sshCmdResultExecuteTime = sshCmdResultExecuteTime;
        this->sshCmdId = sshCmdId;
    }

    //析构函数必须按照规范，自己定义delete this会导致野指针问题：19:49:38: 程序异常结束。 19:49:38: The process was ended forcefully.
    ~SshCmdResult(){}

};

#endif // SSHCMDRESULT_H
