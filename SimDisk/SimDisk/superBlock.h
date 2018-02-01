#pragma once
//超级块数据结构
#include "iNode.h"
class superBlock
{
public:
	unsigned short inode_num;//i节点数目
	unsigned short inode_remain;//i节点剩余数目
	unsigned int block_num;//磁盘块数
	unsigned int block_remain;//磁盘块剩余数目
	unsigned int inode_table;//iNode表的存放偏移位置
	unsigned int inodemap_pos;//iNode节点位图空闲位图偏移位置
	unsigned int bitmap_pos;//空闲块位图偏移位置
	unsigned short blockSize;//块大小
	unsigned short blockSize_bit;//块大小占用位数
	unsigned long long maxBytes;//文件最大大小
	unsigned int first_data_block;//第一个数据块偏移位置
	unsigned int first_data_block_no;//第一个数据块号
	superBlock();
	~superBlock();
	void printInfo();
	int init();//初始化新的superBlock
};

