#include "stdafx.h"
#include "superBlock.h"


superBlock::superBlock()
{
	
}


superBlock::~superBlock()
{
}

int superBlock::init()
{
	this->blockSize = 1024;//块大小,字节
	this->inode_num = 10240;//i节点数目
	this->inode_remain = 10240;//i节点剩余数目
	this->block_num = 1024 * 100;//磁盘块数
	this->block_remain = 1024 * 100; //磁盘块剩余数
	this->inode_table = this->blockSize*1;
	this->inodemap_pos = ceil((this->inode_table + inode_num*sizeof(iNode)) / blockSize) * blockSize;//空闲iNode位图偏移位置
	this->bitmap_pos = ceil((inodemap_pos + inode_num/8)/blockSize)*blockSize;//空闲块位图偏移位置
	this->first_data_block = ceil(ceil(bitmap_pos + block_num / 8) / blockSize) * blockSize;//第一个数据块的位置
	this->first_data_block_no = ceil(first_data_block/blockSize)+1;//第一个数据块的号码
	this->blockSize_bit = 10;//块大小占用位数
	this->maxBytes = 1024*1024*10;//文件最大大小
	this->block_remain -= first_data_block_no;//去掉保留块
	return 1;
}

void superBlock::printInfo()
{
	cout << "i节点数目: " << inode_num << endl;
	cout << "i节点剩余数目: " << inode_remain << endl;
	cout << "磁盘块数: " << block_num << endl;
	cout << "磁盘块剩余数目: " << block_remain << endl;
	cout << "iNode表的存放偏移位置: " << inode_table << endl;
	cout << "iNode节点位图空闲位图偏移位置: " << inodemap_pos << endl;
	cout << "空闲块位图偏移位置: " << bitmap_pos << endl;
	cout << "块大小: " << blockSize << endl;
	cout << "块大小占用位数: " << blockSize_bit << endl;
	cout << "文件最大大小: " << maxBytes << endl;
	cout << "第一个数据块偏移位置: " << first_data_block << endl;
	cout << "第一个数据块号: " << first_data_block_no << endl;
}