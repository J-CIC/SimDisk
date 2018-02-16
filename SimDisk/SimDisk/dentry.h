#pragma once
#include "iNode.h"
#include "file.h"
#include "dir.h"
//�ڴ���ά����Ŀ¼��
class dentry
{
public:
	string fileName;
	string pathName;
	iNode inode;
	file fileObj;//��ʱ��Ӧ�Ķ���
	vector<unsigned int> block_list;//iNode��Ӧ������block_list
	vector<dentry*> child_list;
	vector<dentry*> sibling_list;
	dentry * parent;
	bool is_dir();//�ж��Ƿ���Ŀ¼
	void setParent(dentry &);
	void addChild(dentry *);
	void removeChild(dentry *s);
	void setSubDentry(vector<dentry *> list);//������Ŀ¼
	void showDentry(vector<string> users);//��ʾĿ¼
	void showItself(const vector<string> &users, string coverName="");//չʾ�Լ���dentry��Ϣ
	string getPathName();//��ȡ·������
	vector<dir> getDirList();//��dentryתΪdir
	dentry();
	dentry(string fileName,iNode inode);
	~dentry();
};
