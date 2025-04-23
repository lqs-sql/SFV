#ifndef RESULT_H
#define RESULT_H

#include <QDebug>

template <typename T>
class Result {
public:
    int code; //连接状态码
    QString msg;//连接新
    T data; // 泛型数据
    Result(){}

    Result(const int code, const QString& msg, const T& data = T()) : code(code), msg(msg), data(data) {}

    static Result<T> set(const int code, const QString& msg, const T& data = T()) {
       return Result<T>(code, msg, data);
    }

    static Result<T> success(const QString& msg, const T& data = T()) {
       return set(200, msg, data);
    }

    static Result<T> fail(const QString& msg, const T& data = T()) {
       return set(500, msg, data);
    }

    void print(){
        qDebug()<<"code:"<<code<<"，msg："<<msg;
    }
    //析构函数必须按照规范，自己定义delete this会导致野指针问题：19:49:38: 程序异常结束。 19:49:38: The process was ended forcefully.
    ~Result(){}

};

#endif // RESULT_H
