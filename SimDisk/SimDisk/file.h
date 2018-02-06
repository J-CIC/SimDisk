#pragma once
#include "iNode.h"
#include "dir.h"

//打开一个文件就生成一个file对象
//其中存放文件描述符和其中的偏移之类的信息
class file
{
public:
	iNode inode;//对应的iNode节点
	unsigned long pos;//对应的偏移位置
	vector<unsigned int>blocks_list;
	void setBlockList();//设置block列表
	file();
	file(iNode inode, vector<unsigned int>blocks_list);
	~file();
};
