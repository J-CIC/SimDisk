#pragma once

class iNode
{
public:
	unsigned int ino;//i�ڵ��
	unsigned short i_mode;//�ļ�ģʽ16λ��ǰ4λ��ʾ�ļ����ͣ���12λ��ʾȨ��
	unsigned long i_size;//�ļ���С����λ�ֽ�
	unsigned long i_time;//�ļ�����ʱ��
	unsigned long i_mtime;//�ļ�����޸�ʱ��
	unsigned long i_atime;//�ļ����һ�η���ʱ��
	unsigned long i_blocks;//�ļ���ռ����
	unsigned short i_uid;//�����û�
	unsigned short i_gid;//�����û���
	unsigned int i_zone[13];//��ָ��
	unsigned short i_count;//���ü���
	unsigned short i_nlink;//��ýڵ㽨�����ӵ��ļ���(Ӳ������)
	iNode();
	iNode(unsigned int no, unsigned long size, unsigned long block,vector<unsigned int> zone);
	~iNode();
	void printInfo();
};
