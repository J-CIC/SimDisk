#pragma once
#include "stdafx.h"
#include "superBlock.h"
#include "iNode.h"

class FileSystem
{
public:
	static const string fileName;//磁盘所在的文件名
	superBlock s_block;//超级块
	iNode root;//根节点
	FileSystem();
	~FileSystem();
private:
	fstream fileDisk;//磁盘的文件流
	int alloc_inode(unsigned long size,iNode &node,bool is_dentry = false);//申请iNode节点,size为字节
	int destroy_inode(int id);//销毁iNode节点
	int read_indoe();//读取iNode节点信息
	int write_inode();//更新iNode信息
};
