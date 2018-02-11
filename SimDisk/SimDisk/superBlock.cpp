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
	this->block_remain = 1024 * 100; //���̿�ʣ����
	this->inode_table = this->blockSize*1;
	this->inodemap_pos = ceil((this->inode_table + inode_num*sizeof(iNode)) / blockSize) * blockSize;//����iNodeλͼƫ��λ��
	this->bitmap_pos = ceil((inodemap_pos + inode_num/8)/blockSize)*blockSize;//���п�λͼƫ��λ��
	this->first_data_block = ceil(ceil(bitmap_pos + block_num / 8) / blockSize) * blockSize;//��һ�����ݿ��λ��
	this->first_data_block_no = ceil(first_data_block/blockSize)+1;//��һ�����ݿ�ĺ���
	this->blockSize_bit = 10;//���Сռ��λ��
	this->maxBytes = 1024*1024*10;//�ļ�����С
	this->block_remain -= first_data_block_no;//ȥ��������
	return 1;
}

void superBlock::printInfo()
{
	cout << "i�ڵ���Ŀ: " << inode_num << endl;
	cout << "i�ڵ�ʣ����Ŀ: " << inode_remain << endl;
	cout << "���̿���: " << block_num << endl;
	cout << "���̿�ʣ����Ŀ: " << block_remain << endl;
	cout << "iNode��Ĵ��ƫ��λ��: " << inode_table << endl;
	cout << "iNode�ڵ�λͼ����λͼƫ��λ��: " << inodemap_pos << endl;
	cout << "���п�λͼƫ��λ��: " << bitmap_pos << endl;
	cout << "���С: " << blockSize << endl;
	cout << "���Сռ��λ��: " << blockSize_bit << endl;
	cout << "�ļ�����С: " << maxBytes << endl;
	cout << "��һ�����ݿ�ƫ��λ��: " << first_data_block << endl;
	cout << "��һ�����ݿ��: " << first_data_block_no << endl;
}