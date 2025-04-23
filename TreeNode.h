#ifndef TREENODE_H
#define TREENODE_H


#include <QString>
#include <QIcon>
#include <QList>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QDebug>

class TreeNode {
public:
    QString path;  // 节点对应的文件或目录路径
    QString name;  // 节点对应的文件或目录名称
    bool isDirectory;  // 标记节点是否为目录
    QIcon icon;  // 节点对应的图标
    QList<TreeNode *> children; // 子节点列表
    TreeNode *parent; //指向对应父节点

    TreeNode(const QString &path, bool isDir, const QString &name){
        QFileIconProvider iconProvider;
        QFileInfo fileInfo(path);
        icon = iconProvider.icon(fileInfo);
        this->name = name;
        this->path = path;
        this->isDirectory = isDirectory;
    }

    ~TreeNode() {
        // 释放子节点内存
        for (TreeNode *child : children) {
            delete child;
        }
    }

};

#endif // TREENODE_H
