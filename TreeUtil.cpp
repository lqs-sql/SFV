
#include <TreeNode.h>

#include <QDir>
#include <QDebug>
#include <QQueue>

//树的工具类
class TreeUtil {

public:
    /**
     * 传入文件树根路径，返回一个构造的文件树的根节点(层次或者说广度遍历)
     * @brief buildDirTree
     * @param rootPath：传入的目录路径，必须是目录而不是文件
     */
    static TreeNode* buildDirTree(const QString &rootPath) {
        QDir rootDir(rootPath);
        if (!rootDir.exists()) {
            return nullptr;
        }

        QQueue<QDir> dirQueue;//构造一个非叶子节点（即带子节点的，目录类型）队列
        dirQueue.enqueue(rootDir);
        TreeNode* rootNode = nullptr;
        while (!dirQueue.isEmpty()) {
            //非叶子/目录队列不为空，取子节点进行层次遍历
            QDir currentDir = dirQueue.dequeue();
            qDebug() << "当前遍历的目录节点:" << currentDir.absolutePath();
            // 创建当前遍历的目录节点
            TreeNode* dirNode = new TreeNode(currentDir.absolutePath(), true,currentDir.dirName());
            //记录根节点
            if(dirNode->path == rootPath){
                rootNode = dirNode;
                rootNode->parent = nullptr;
            }
            //设置目录图标
            dirNode->icon = *new QIcon(":/icon/folder.png");
            // 创建当前遍历的目录节点旗下的子节点列表
            QList<TreeNode *> children;

            //递归查找所有子目录和子文件，加上 QDir::AllEntries参数实现查找所有子目录和子文件
    //        QFileInfoList entryList = currentDir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries);
            // 仅查找当前目录下的子目录和子文件，不递归查找到孙子以及下面级别
            QFileInfoList entryList = currentDir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
            for (const QFileInfo& info : entryList) {
                TreeNode* subNode = new TreeNode(info.absoluteFilePath(),info.isDir(),info.fileName());
                if (info.isDir()) {//子节点是非叶子节点/目录，子节点入队
                    QDir subDir(info.absoluteFilePath());
                    dirQueue.enqueue(subDir);

                    //设置目录图标
                    subNode->icon = *new QIcon(":/icon/folder.png");

                }else{//子节点是叶子节点/文件
                    //设置文件图标
                    subNode->icon = *new QIcon(":/icon/cut.png");
                }
                //将子节点加入对应当前遍历的目录节点下方
                children.append(subNode);
                //设置子节点的父节点指向
                subNode->parent = dirNode;
            }
            //层次遍历后的子节点复制
            dirNode->children = children;
            //把更新后的最新dirNode目录节点更新到根节点下面
            updateOffspringNode(rootNode,dirNode);
        }
        //退出循环后，返回根节点
        return rootNode;

    }
    /**
     * 更新根节点下的后代节点的children属性
     * @brief updateOffspringNode
     * @param rootNode
     * @param offspringNode
     * @return
     */
    static TreeNode* updateOffspringNode(TreeNode* rootNode, TreeNode* offspringNode) {
        if (!rootNode || !offspringNode) {
            return rootNode;
        }

        if (rootNode->path == offspringNode->path) {//根据path属性判断是否同一个旧的后代节点offspringNode
            // 更新子节点的children属性
            rootNode->children = offspringNode->children;
            // 更新新子节点的父节点指针
            for (TreeNode *child : rootNode->children) {
                child->parent = rootNode;
            }
            return rootNode;
        }

        for (int i = 0; i < rootNode->children.size(); ++i) {
            // 递归查找子节点
            TreeNode* result = updateOffspringNode(rootNode->children[i], offspringNode);
            if (result) {
                return rootNode;
            }
        }
        return rootNode;
    }

    /**
     *  深度优先遍历树节点的方法
     * @brief dfs
     * @param root：根节点
     */
    static void dfs(TreeNode* rootNode) {
        if (rootNode == nullptr) {
            return;
        }
        // 访问当前节点，这里简单打印节点路径
        qDebug() << "dfs方式，当前遍历节点："<<rootNode->path;
        // 递归遍历所有子节点
        for (TreeNode* child : rootNode->children) {
            dfs(child);
        }
    }
    /**
     *  广度优先遍历树节点的方法（层次遍历）
     * @brief bfs
     * @param rootNode
     */
    static void bfs(TreeNode* rootNode) {
        if (rootNode == nullptr) {
            return;
        }
        QQueue<TreeNode*> noLeafQueue;//构造一个非叶子节点（即带子节点的，目录类型）队列
        noLeafQueue.enqueue(rootNode);
        while (!noLeafQueue.isEmpty()) {//非叶子/目录队列不为空，取叶子节点进行层次遍历
            TreeNode* currentNode = noLeafQueue.dequeue();
            for (TreeNode* child:currentNode->children) {
                if (child->isDirectory) {//子节点是非叶子节点/目录，子节点入队（非叶子节点才需要入队进行层次遍历）
                    noLeafQueue.enqueue(child);
                }else{//子节点是叶子节点/文件，进行遍历操作
                    qDebug() << "bfs方式，当前遍历节点："<<currentNode->path;
                }
            }
        }

    }


    /**
     *  获取树的高度/深度
     * @brief getTreeDepth
     * @return
     */
    static int getTreeDepth(TreeNode* rootNode){
        if (!rootNode) {
            return 1;
        }
        int maxDepth = 0;
        for (TreeNode* child : rootNode->children) {
            int childDepth = getTreeDepth(child);
            if (childDepth > maxDepth) {
                maxDepth = childDepth;
            }
        }
        return maxDepth + 1;
    }

};
