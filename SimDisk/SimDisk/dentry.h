#pragma once
#include "iNode.h"
//�ڴ���ά����Ŀ¼��
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

