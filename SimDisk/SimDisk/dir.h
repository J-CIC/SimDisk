#pragma once
//�����ϴ洢��Ŀ¼�ṹ���ֻ����124�ֽڳ�
class dir
{
public:
	char dir_name[124];
	unsigned int ino;
	dir();
	dir(string name, unsigned int id);
	~dir();
};
