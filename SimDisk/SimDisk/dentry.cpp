#include "stdafx.h"
#include "dentry.h"
#include "toolkit.h"

dentry::dentry()
{

}

dentry::dentry(string fileName, iNode inode)
{
	this->fileName = fileName;
	this->inode = inode;
}

dentry::~dentry()
{
}

//��ȡ��Ŀ¼
void dentry::setSubDentry(vector<dentry *> list)
{
	this->child_list.clear();
	dentry father = *this;
	this->child_list.push_back(this);
	this->child_list = list;
}

//�ж��Ƿ�ΪĿ¼
bool dentry::is_dir(){
	unsigned short mask = 1;
	mask = mask << 14;
	return inode.i_mode&mask;
}

void dentry::showDentry()
{
	cout << "." << "\t";
	cout << ".." << "\t";
	for (auto item : child_list)
	{
		cout << item->fileName << "\t";
	}
	cout << endl;
}

//��dentryתΪdir
vector<dir> dentry::getDirList(){
	vector<dir> ret_list;
	for (auto item : child_list){
		dir temp = dir(item->fileName, item->inode.ino);
		ret_list.push_back(temp);
	}
	return ret_list;
}

void dentry::setParent(dentry &p)
{
	this->parent = &p;
}

void dentry::addChild(dentry *s){
	this->child_list.push_back(s);
	inode.i_size = child_list.size()*sizeof(dir);//����ռ�ô�С
}

void dentry::removeChild(dentry *s){
	//�����ҵ���Ӧ��Dentry��ɾ��
	for (auto iter = child_list.begin(); iter != child_list.end();){
		if ((*iter)->inode.ino == s->inode.ino){
			iter = child_list.erase(iter);
			break;
		}
		++iter;
	}
	inode.i_size = child_list.size()*sizeof(dir);//����ռ�ô�С
}