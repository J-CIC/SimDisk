#pragma once
#include "iNode.h"
#include "dir.h"
//�ڴ���ά����Ŀ¼��
class dentry
{
public:
	string fileName;
	string pathName;
	iNode inode;
	vector<unsigned int> block_list;//iNode��Ӧ������block_list
	vector<dentry*> child_list;
	dentry * parent;
	bool is_dir();//�ж��Ƿ���Ŀ¼
	void setParent(dentry *);//���ø��׽ڵ�
	void addChild(dentry *);//����ӽڵ�
	void removeChild(dentry *s);//ɾ���ӽڵ�
	void setSubDentry(vector<dentry *> list);//������Ŀ¼
	void showDentry(vector<string> users);//��ʾĿ¼
	void showItself(const vector<string> &users, string coverName="");//չʾ�Լ���dentry��Ϣ
	string getPathName();//��ȡ·������
	vector<dir> getDirList();//��dentryתΪdir
	dentry();
	dentry(string fileName,iNode inode);
	~dentry();
};
