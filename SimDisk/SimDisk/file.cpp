#include "stdafx.h"
#include "file.h"


file::file(FileSystem *fs, iNode node)
{
	this->fs = fs;
	this->inode = node;
	pos = 0;
}


file::~file()
{
}
