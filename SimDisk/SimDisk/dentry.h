#pragma once
#include "iNode.h"
#include "dir.h"
//内存中维护的目录项
class dentry
{
public:
	string fileName;
	string pathName;
	iNode inode;
	vector<unsigned int> block_list;//iNode对应的内容block_list
	vector<dentry*> child_list;
	dentry * parent;
	bool is_dir();//判断是否是目录
	void setParent(dentry *);//设置父亲节点
	void addChild(dentry *);//添加子节点
	void removeChild(dentry *s);//删除子节点
	void setSubDentry(vector<dentry *> list);//设置字目录
	void showDentry(vector<string> users);//显示目录
	void showItself(const vector<string> &users, string coverName="");//展示自己的dentry信息
	string getPathName();//获取路径名称
	vector<dir> getDirList();//将dentry转为dir
	dentry();
	dentry(string fileName,iNode inode);
	~dentry();
};
