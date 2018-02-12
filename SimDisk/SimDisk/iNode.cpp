#include "stdafx.h"
#include "iNode.h"
#include "toolkit.h"


iNode::iNode()
{
}


iNode::~iNode()
{
}

iNode::iNode(unsigned int no, unsigned long size,unsigned long block,vector<unsigned int> zone) :ino(no),i_size(size),i_blocks(block){
	i_time = time(NULL);
	i_atime = i_time;
	i_mtime = i_time;
	i_count = 1;
	i_nlink = 1;
	i_uid = 1;
	i_gid = 1;
	int index = 0;
	for (auto b_no : zone){
		i_zone[index++] = b_no;
	}
	for (; index < 13; index++){
		i_zone[index] = 0;
	}
}

void iNode::printInfo()
{
	cout << "i�ڵ��: " << ino << endl;
	cout << "�ļ�ģʽ: " << num2bin(i_mode) << endl;
	cout << "�ļ���С����λ�ֽ�: " << i_size << endl;
	cout << "�ļ�����ʱ��: " << i_time << endl;
	cout << "�ļ�����޸�ʱ��: " << i_mtime << endl;
	cout << "�ļ����һ�η���ʱ��: " << i_atime << endl;
	cout << "�ļ���ռ����: " << i_blocks << endl;
	cout << "�����û�: " << i_uid << endl;
	cout << "�����û���: " << i_gid << endl;
	cout << "���ü���: " << i_count << endl;
	cout << "��ýڵ㽨�����ӵ��ļ���(Ӳ������): " << i_nlink << endl;
}
