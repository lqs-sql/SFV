#include "StringUtil.h"
#include <QStringList>
#include <QDebug>
#include <QRegularExpression>

/**
 * 从标志串类型的partOutput提取出对应的当前所在路径
 * @brief extractPathByPartOutput
 * @param input
 * @return
 */
 QString StringUtil::extractPathByPartOutput(const QString& input,const QString& signalStr) {
     int index = input.lastIndexOf(signalStr);
    if (index != -1) {
        // 定位到目标字符串之后的位置
        index += signalStr.length();
        // 跳过可能存在的空格
        while (index < input.length() && input[index].isSpace()) {
            ++index;
        }
        // 查找下一个空格的位置
        int nextSpaceIndex = input.indexOf(' ', index);
        if (nextSpaceIndex != -1) {
            return input.mid(index, nextSpaceIndex - index);
        } else {
            // 如果没有找到下一个空格，就取到字符串末尾
            return input.mid(index);
        }
    }
    return "";
}
 /**
  * 出现俩次用户字符串情况下，删除第一个用户字符串
 * @brief StringUtil::deleteFirstRedundantUserStr
 * @param input
 * @param path
 * @return
 */
QString StringUtil::deleteRedundantUserStr(QString mainStr,QString path){
    int pathIndex = mainStr.lastIndexOf(path);
//    QStringList substrings = mainStr.split(QRegularExpression("\\s+"), QString::SkipEmptyParts);
    int startIndex = -1;
    int endIndex = -1;
    for(int i = 0;i<mainStr.length()-1;i++){
        if(mainStr[i] == '['){
            startIndex = i;
        }
        if(mainStr[i] == ']' && (mainStr[i+1] == '$' || mainStr[i] == '#')){
            endIndex = i;
            break;
        }
    }
    // 提取 startIndex 之前的部分
    QString before = mainStr.left(startIndex-2);
    // 提取 endIndex 之后的部分
    QString after = mainStr.mid(endIndex + 2);
    // 重新组合字符串
    mainStr = before + after;
    return mainStr;
 }

/**
 * 实现类的静态成员函数：QString转char*
 * @brief toCharPtr
 * @param str
 * @return
 */
const char* StringUtil::stringToCharPtr(const QString&  str) {
    return str.toUtf8().constData();
}
/**
 * 替代主串的最后一个匹配的旧子串替代为新子串
 * @brief StringUtil::replaceEndSubStr
 * @param str：主串
 * @param oldStr：旧子串
 * @param newStr：新子串
 * @return
 */
const QString StringUtil::replaceEndSubStr(QString & str,const QString oldStr,const QString newStr){
    // 查找最后一次出现的位置
    int lastIndex = str.lastIndexOf(oldStr);
    if (lastIndex != -1) {
        // 从最后一次出现的位置开始替换
        return str.replace(lastIndex, oldStr.length(), newStr);
    }
}
/**
 * 从主串中提取子串之后的部分
 * @brief extractSubstringAfter
 * @param mainString
 * @param subString
 * @return
 */
const QString StringUtil::extractSubstringAfter(const QString &mainString, const QString &subString) {
    // 查找子串在主串中的起始位置
    int index = mainString.indexOf(subString);
    if (index != -1) {
        // 计算子串之后的起始位置
        int start = index + subString.length();
        // 提取子串之后的部分
        return mainString.mid(start);
    }else if(subString==""){
        return mainString;
    }
    // 如果子串不存在于主串中，返回空字符串
    return "";
}
/**
 * 提取一个绝对路径下的当前路径（不包括当前目录名/文件名）
 * @brief StringUtil::extractPathByAbsolutePath
 * @param absolutePath
 * @return
 */
const QString StringUtil::extractPathByAbsolutePath(const QString absolutePath){
    int lastIndex = absolutePath.lastIndexOf("/");
    //从0索引截取lastIndex+1长度的字符串（即包括lastIndex索引字符）
    return absolutePath.mid(0,lastIndex+1);
}
/**
 * 提取一个绝对路径下的当前最后一级的文件名/目录名
 * @brief StringUtil::extractPathByAbsolutePath
 * @param absolutePath
 * @return
 */
const QString StringUtil::extractFileNameOrDirNameByAbsolutePath(const QString absolutePath){
    int lastIndex = absolutePath.lastIndexOf("/");
    //从0索引截取lastIndex+1长度的字符串（即包括lastIndex索引字符）
    return absolutePath.mid(lastIndex+1);
}
/**
 * 提取用户名（首先找到最后一个换行符 \n 的位置，然后从该位置之后的子字符串中查找最后一个空格的位置）
 * @brief extractSubstringAfter
 * @param mainString
 * @param subString
 * @return
 */
const QString StringUtil::extractUserName(const QString &str){
     int pos = str.lastIndexOf('\n');
     QString username = "";
     if(pos !=-1){
         QString reverseUsername="";
         for(int i=pos-1;i>=0;i--){
             if(str[i]==" "){
                 break;
             }else{
                reverseUsername.append(str[i]);
             }
         }
         if(StringUtil::isNotBlank(reverseUsername)){
             for(int i=reverseUsername.length()-1;i>=0;i--){
                 username.append(reverseUsername[i]);
             }
         }
     }
    return username;
}
/**
 * 获取字符串里最后的命令执行内容
 * @brief StringUtil::getLastCommandStr
 * @param allStr：字符串主串
 * @return
 */
const QString StringUtil::getLastCommandStr(const QString allStr){
    QStringList splitStr = allStr.split("]$");
    return splitStr.last().trimmed();
}

/**
 * 判断字符串非空且非""
 *      isNull()：用于判断字符串对象是否未被初始化。
        isEmpty()：用于判断字符串的长度是否为 0，null 字符串和空字符串都会返回 true。
 * @brief StringUtil::isNotBlank
 * @param str
 * @return
 */
const bool StringUtil::isNotBlank(const QString & str){
   if(str.isEmpty()){
       return false;
   }else{
       return true;
   }
}
/**
 * 判断字符串为空或者为""
 *      isNull()：用于判断字符串对象是否未被初始化。
        isEmpty()：用于判断字符串的长度是否为 0，null 字符串和空字符串都会返回 true。
 * @brief StringUtil::isBlank
 * @param str
 * @return
 */
const bool StringUtil::isBlank(const QString & str){
   if(str.isEmpty()){
       return true;
   }else{
       return false;
   }
}
/**
 * 将字节数转换为合适的单位表示
 * @brief byteCountToSize
 * @param bytes
 * @return
 */
QString StringUtil::byteCountToSize(quint64 bytes) {
    if (bytes < 1024) {
        return QString("%1 B").arg(bytes);
    } else if (bytes < 1024 * 1024) {
        return QString("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    } else if (bytes < 1024 * 1024 * 1024) {
        return QString("%1 MB").arg(bytes / (1024.0 * 1024), 0, 'f', 1);
    } else {
        return QString("%1 GB").arg(bytes / (1024.0 * 1024 * 1024), 0, 'f', 1);
    }
}
