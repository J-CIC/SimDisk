#pragma once
#include "iNode.h"
#include "file.h"
#include "dir.h"
//内存中维护的目录项
class dentry
{
public:
	string fileName;
	string pathName;
	iNode inode;
	file fileObj;//打开时对应的对象
	vector<unsigned int> block_list;//iNode对应的内容block_list
	vector<dentry*> child_list;
	vector<dentry*> sibling_list;
	dentry * parent;
	bool is_dir();//判断是否是目录
	void setParent(dentry &);
	void addChild(dentry *);
	void removeChild(dentry *s);
	void setSubDentry(vector<dentry *> list);//设置字目录
	void showDentry(vector<string> users);//显示目录
	void showItself(const vector<string> &users, string coverName="");//展示自己的dentry信息
	string getPathName();//获取路径名称
	vector<dir> getDirList();//将dentry转为dir
	dentry();
	dentry(string fileName,iNode inode);
	~dentry();
};
