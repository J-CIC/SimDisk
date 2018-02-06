#include "stdafx.h"
#include "dentry.h"


dentry::dentry()
{

}

dentry::~dentry()
{
}

//获取子目录
void dentry::setSubDentry(vector<dentry> list)
{
	this->child_list.clear();
	dentry father = *this;
	this->child_list.push_back(*this);
	this->child_list = list;
}

void dentry::showDentry()
{
	cout << "." << endl;
	cout << ".." << endl;
	for (auto item : child_list)
	{
		cout << item.fileName << endl;
	}
}