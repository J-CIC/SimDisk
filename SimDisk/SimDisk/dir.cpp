#include "stdafx.h"
#include "dir.h"


dir::dir()
{
}


dir::~dir()
{
}

dir::dir(string name, unsigned int id)
{
	//��������
	strncpy(dir_name, name.c_str(), 123);
	dir_name[123] = '\0';//�Զ��ض�
	ino = id;
}
