#pragma once
//磁盘上存储的目录结构，最长只可以124字节长
class dir
{
public:
	char dir_name[124];
	unsigned int ino;
	dir();
	dir(string name, unsigned int id);
	~dir();
};
