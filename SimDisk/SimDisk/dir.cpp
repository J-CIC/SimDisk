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
	//复制名字
	strncpy_s(dir_name, name.c_str(), 123);
	dir_name[123] = '\0';//自动截断
	ino = id;
}
