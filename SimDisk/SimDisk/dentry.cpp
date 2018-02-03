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
	this->child_list = list;
}