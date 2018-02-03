#pragma once
#include "iNode.h"
//内存中维护的目录项
class dentry
{
public:
	string fileName;
	iNode ino;
	vector<dentry> child_list;
	vector<dentry> sibling_list;
	dentry * parent;
	dentry();
	~dentry();
};

