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
	this->blockSize = 1024;//���С,�ֽ�
	this->inode_num = 10240;//i�ڵ���Ŀ
	this->inode_remain = 10240;//i�ڵ�ʣ����Ŀ
	this->block_num = 1024 * 100;//���̿���
	this->block_remain = 1024 * 100;//���̿�ʣ����
	this->inodemap_pos = ceil((blockSize + inode_num*sizeof(iNode)) / blockSize) * blockSize;//����iNodeλͼƫ��λ��
	this->bitmap_pos = ceil((inodemap_pos + inode_num/8)/blockSize)*blockSize;//���п�λͼƫ��λ��
	this->first_data_block = ceil(ceil(bitmap_pos + block_num / 8) / blockSize) * blockSize;//��һ�����ݿ��λ��
	this->blockSize_bit = 10;//���Сռ��λ��
	this->maxBytes = 1024*1024*10;//�ļ�����С
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