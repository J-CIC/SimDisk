#pragma once
#include "iNode.h"
#include "file.h"
#include "dir.h"
//�ڴ���ά����Ŀ¼��
class dentry
{
public:
	string fileName;
	iNode inode;
	file fileObj;//��ʱ��Ӧ�Ķ���
	vector<unsigned int> block_list;//iNode��Ӧ������block_list
	vector<dentry> child_list;
	vector<dentry> sibling_list;
	bool is_dir();//�ж��Ƿ���Ŀ¼
	dentry * parent;
	void setSubDentry(vector<dentry> list);
	void showDentry();
	vector<dir> getDirList();//��dentryתΪdir
	dentry();
	~dentry();
};
