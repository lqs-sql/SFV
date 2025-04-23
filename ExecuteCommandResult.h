#ifndef EXECUTECOMMANDRESULT_H
#define EXECUTECOMMANDRESULT_H

#include <libssh2.h>
#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(dll, "ws2_32.dll")

class ExecuteCommandResult {
public:

    int commandType; //0-非阻塞，1-阻塞（2-密码）

    QString content; //命令回显内容（命令结果）

    ExecuteCommandResult(){}

    ExecuteCommandResult(const int commandType) : commandType(commandType) {}


    //析构函数必须按照规范，自己定义delete this会导致野指针问题：19:49:38: 程序异常结束。 19:49:38: The process was ended forcefully.
    ~ExecuteCommandResult(){}

};

#endif // EXECUTECOMMANDRESULT_H
