#include "stdafx.h"
#include "dentry.h"
#include "toolkit.h"
#include "iomanip"
#include <algorithm>

dentry::dentry()
{

}

dentry::dentry(string fileName, iNode inode)
{
	this->fileName = fileName;
	this->inode = inode;
	this->pathName = "";
}

dentry::~dentry()
{
}

//获取子目录
void dentry::setSubDentry(vector<dentry *> list)
{
	this->child_list.clear();
	dentry father = *this;
	this->child_list.push_back(this);
	this->child_list = list;
}

//获取路径名称
string dentry::getPathName(){
	if (this->fileName == "/"){
		return "/";
	}
	return this->pathName;
}

//判断是否为目录
bool dentry::is_dir(){
	unsigned short mask = 1;
	mask = mask << 14;
	return inode.i_mode&mask;
}

void dentry::showItself(const vector<string> &users,string coverName){
	int subFile = 1;
	string showName = this->fileName;
	if (coverName != ""){
		showName = coverName;
	}
	if (this->is_dir()){
		subFile = this->child_list.size();
	}
	cout << num2permission(inode.i_mode) << " ";
	cout << subFile << " ";
	cout << users[inode.i_uid] << " ";
	cout << users[inode.i_uid] << " ";
	cout << inode.i_size << "\t";
	cout << inode.i_time << "\t";
	cout << showName;
	cout << endl;
}

void dentry::showDentry(vector<string> users)
{
	sort(this->child_list.begin(), this->child_list.end(), dentryComp);
	showItself(users,".");//输出当前目录
	this->parent->showItself(users,"..");//输出父目录
	for (auto item : child_list)
	{
		item->showItself(users);
	}
}

//将dentry转为dir
vector<dir> dentry::getDirList(){
	vector<dir> ret_list;
	for (auto item : child_list){
		dir temp = dir(item->fileName, item->inode.ino);
		ret_list.push_back(temp);
	}
	return ret_list;
}

void dentry::setParent(dentry *p)
{
	this->parent = p;
	if (this->fileName == "/"){
		this->pathName = "";
	}
	else{
		this->pathName = p->pathName + "/" + this->fileName;
	}
}

void dentry::addChild(dentry *s){
	this->child_list.push_back(s);
	inode.i_size = child_list.size()*sizeof(dir);//更新占用大小
}

void dentry::removeChild(dentry *s){
	//遍历找到对应的Dentry项删除
	for (auto iter = child_list.begin(); iter != child_list.end();){
		if ((*iter)->inode.ino == s->inode.ino){
			iter = child_list.erase(iter);
			break;
		}
		++iter;
	}
	inode.i_size = child_list.size()*sizeof(dir);//更新占用大小
}