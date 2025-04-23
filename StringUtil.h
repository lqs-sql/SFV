#ifndef STRINGUTIL_H
#define STRINGUTIL_H

#include <QString>
#include <QByteArray>

//静态字符串工具类
class StringUtil {
public:

    /**
     * 从标志串类型的partOutput提取出对应的当前所在路径
     * @brief extractPathByPartOutput
     * @param input
     * @return
     */
    static QString extractPathByPartOutput(const QString& input,const QString& signalStr);
    /**
     * 出现俩次用户字符串情况下，删除第一个用户字符串
    * @brief StringUtil::deleteFirstRedundantUserStr
    * @param input
    * @param path
    * @return
    */
    static QString deleteRedundantUserStr(QString mainStr,QString path);
    /**
     * QString转char*
     * @brief toCharPtr
     * @param str
     * @return
     */
    static const char* stringToCharPtr(const QString & str);
    /**
     * 替代主串的最后一个匹配的旧子串替代为新子串
     * @brief StringUtil::replaceEndSubStr
     * @param str：主串
     * @param oldStr：旧子串
     * @param newStr：新子串
     * @return
     */
    static const QString replaceEndSubStr(QString & str,const QString oldStr,const QString newStr);

    /**
     * 从主串中提取子串之后的部分
     * @brief extractSubstringAfter
     * @param mainString
     * @param subString
     * @return
     */
    static const QString extractSubstringAfter(const QString &mainString, const QString &subString);
    /**
     * 提取一个绝对路径下的当前路径（不包括当前目录名/文件名）
     * @brief StringUtil::extractPathByAbsolutePath
     * @param absolutePath
     * @return
     */
    static const QString extractPathByAbsolutePath(const QString absolutePath);
    /**
     * 提取一个绝对路径下的当前最后一级的文件名/目录名
     * @brief StringUtil::extractPathByAbsolutePath
     * @param absolutePath
     * @return
     */
    static const QString extractFileNameOrDirNameByAbsolutePath(const QString absolutePath);
    /**
     * 提取用户名（首先找到最后一个换行符 \n 的位置，然后从该位置之后的子字符串中查找最后一个空格的位置）
     * @brief extractSubstringAfter
     * @param mainString
     * @param subString
     * @return
     */
    static const QString extractUserName(const QString &str);
    /**
     * 获取字符串里最后的命令执行内容
     * @brief StringUtil::getLastCommandStr
     * @param allStr：字符串主串
     * @return
     */
    static const QString getLastCommandStr(const QString allStr);
    /**
     * 判断字符串非空且非""
     *      isNull()：用于判断字符串对象是否未被初始化。
            isEmpty()：用于判断字符串的长度是否为 0，null 字符串和空字符串都会返回 true。
     * @brief StringUtil::isNotBlank
     * @param str
     * @return
     */
    static const bool isNotBlank(const QString & str);
    /**
     * 判断字符串为空或者为""
     *      isNull()：用于判断字符串对象是否未被初始化。
            isEmpty()：用于判断字符串的长度是否为 0，null 字符串和空字符串都会返回 true。
     * @brief StringUtil::isBlank
     * @param str
     * @return
     */
    static const bool isBlank(const QString & str);

    /**
     * 将字节数转换为合适的单位表示
     * @brief byteCountToSize
     * @param bytes
     * @return
     */
    static QString byteCountToSize(quint64 bytes);
};

#endif // STRINGUTIL_H
