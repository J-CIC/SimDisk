#pragma once
//���������ݽṹ
#include "iNode.h"
class superBlock
{
public:
	unsigned short inode_num;//i�ڵ���Ŀ
	unsigned short inode_remain;//i�ڵ�ʣ����Ŀ
	unsigned int block_num;//���̿���
	unsigned int block_remain;//���̿�ʣ����Ŀ
	unsigned int inode_table;//iNode��Ĵ��ƫ��λ��
	unsigned int inodemap_pos;//iNode�ڵ�λͼ����λͼƫ��λ��
	unsigned int bitmap_pos;//���п�λͼƫ��λ��
	unsigned short blockSize;//���С
	unsigned short blockSize_bit;//���Сռ��λ��
	unsigned long long maxBytes;//�ļ�����С
	unsigned int first_data_block;//��һ�����ݿ�ƫ��λ��
	unsigned int first_data_block_no;//��һ�����ݿ��
	superBlock();
	~superBlock();
	void printInfo();
	int init();//��ʼ���µ�superBlock
};

