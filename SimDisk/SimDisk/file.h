#pragma once
#include "iNode.h"
#include "FileSystem.h"
//��һ���ļ�������һ��file����
//���д���ļ������������е�ƫ��֮�����Ϣ
class file
{
public:
	iNode inode;//��Ӧ��iNode�ڵ�
	unsigned long pos;//��Ӧ��ƫ��λ��
	FileSystem *fs;
	file(FileSystem *fs, iNode node);
	~file();
};

