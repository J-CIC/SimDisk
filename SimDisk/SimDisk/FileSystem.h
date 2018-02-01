#pragma once
#include "stdafx.h"
#include "superBlock.h"
#include "iNode.h"

class FileSystem
{
public:
	static const string fileName;//�������ڵ��ļ���
	superBlock s_block;//������
	iNode root;//���ڵ�
	FileSystem();
	~FileSystem();
private:
	fstream fileDisk;//���̵��ļ���
	int alloc_inode(unsigned long size,iNode &node,bool is_dentry = false);//����iNode�ڵ�,sizeΪ�ֽ�
	int alloc_blocks(int num, vector<unsigned int> &list);
	int destroy_inode(int id);//����iNode�ڵ�
	int read_indoe();//��ȡiNode�ڵ���Ϣ
	int write_inode(iNode node);//����iNode��Ϣ
	template<typename T> int seekAndGet(unsigned long pos, T &item);
	template<typename T> int seekAndSave(unsigned long pos, T &item);
};
