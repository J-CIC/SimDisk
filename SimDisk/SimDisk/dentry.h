#pragma once
#include "iNode.h"
#include "file.h"
#include "dir.h"
//内存中维护的目录项
class dentry
{
public:
	string fileName;
	iNode inode;
	file fileObj;//打开时对应的对象
	vector<unsigned int> block_list;//iNode对应的内容block_list
	vector<dentry> child_list;
	vector<dentry> sibling_list;
	bool is_dir();//判断是否是目录
	dentry * parent;
	void setSubDentry(vector<dentry> list);
	void showDentry();
	vector<dir> getDirList();//将dentry转为dir
	dentry();
	~dentry();
};
