#pragma once
#include "iNode.h"
#include "dir.h"

//��һ���ļ�������һ��file����
//���д���ļ������������е�ƫ��֮�����Ϣ
class file
{
public:
	iNode inode;//��Ӧ��iNode�ڵ�
	unsigned long pos;//��Ӧ��ƫ��λ��
	vector<unsigned int>blocks_list;
	void setBlockList();//����block�б�
	file();
	file(iNode inode, vector<unsigned int>blocks_list);
	~file();
};
