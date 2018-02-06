#include "stdafx.h"
#include "file.h"


file::file()
{
}

file::file(iNode node, vector<unsigned int>list)
{
	this->inode = node;
	this->blocks_list = list;
}


file::~file()
{
}


void file::setBlockList()
{
	
}