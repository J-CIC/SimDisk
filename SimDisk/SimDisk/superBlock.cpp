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
	this->block_remain = 1024 * 100;//磁盘块剩余数
	this->inodemap_pos = ceil((blockSize + inode_num*sizeof(iNode)) / blockSize) * blockSize;//空闲iNode位图偏移位置
	this->bitmap_pos = ceil((inodemap_pos + inode_num/8)/blockSize)*blockSize;//空闲块位图偏移位置
	this->first_data_block = ceil(ceil(bitmap_pos + block_num / 8) / blockSize) * blockSize;//第一个数据块的位置
	this->blockSize_bit = 10;//块大小占用位数
	this->maxBytes = 1024*1024*10;//文件最大大小
	return 1;
}

void superBlock::printInfo()
{
	cout << "blockSize: "<< this->blockSize << endl;
	cout << "inode_num: " << this->inode_num << endl;
	cout << "block_num: " << this->block_num << endl;
	cout << "bitmap_pos: " << this->bitmap_pos << endl;
	cout << "blockSize_bit: " << this->blockSize_bit << endl;
	cout << "maxBytes: " << this->maxBytes << endl;
}