#include "stdafx.h"
#include "dentry.h"


dentry::dentry()
{

}


dentry::~dentry()
{
}

//��ȡ��Ŀ¼
void dentry::setSubDentry(vector<dentry> list)
{
	this->child_list = list;
}