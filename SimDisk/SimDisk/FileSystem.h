#pragma once
#include "stdafx.h"
#include "superBlock.h"
#include "iNode.h"
#include "dentry.h"
#include "file.h"

class FileSystem
{
public:
	static const string fileName;//�������ڵ��ļ���
	superBlock s_block;//������
	iNode root;//���ڵ�
	dentry root_dentry;//��Ŀ¼
	dentry curr_dentry;//����Ŀ¼������ǰĿ¼
	FileSystem();
	~FileSystem();
private:
	fstream fileDisk;//���̵��ļ���
	int init_dentry();//��ʼ����Ŀ¼
	int alloc_inode(unsigned long size,iNode &node,bool is_dentry = false);//����iNode�ڵ�,sizeΪ�ֽ�
	int alloc_blocks(int num, vector<unsigned int> &list);
	int destroy_inode(int id);//����iNode�ڵ�
	int destroy_block(int id);//����block
	int read_indoe();//��ȡiNode�ڵ���Ϣ
	int write_inode(iNode node);//����iNode��Ϣ
	int getSubDentry(const dentry& p_dir);
	int clearBlockContent(vector<unsigned int> list);
	template<typename T> int seekAndGet(unsigned long pos, T &item);
	template<typename T> int seekAndSave(unsigned long pos, T &item);
};
