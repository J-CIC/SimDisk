#include "stdafx.h"
#include "FileSystem.h"
const string FileSystem::fileName = "disk.txt";


FileSystem::FileSystem()
{
	long int diskSize = 100 * 1024 * 1024 * 1;//磁盘大小,Byte
	fileDisk.open(fileName, ios::binary | ios::in| ios::out);
	if (!fileDisk){//文件不存在
		fileDisk.open(fileName, ios::out | ios::binary);
		//生成新超级块的信息,并写入
		superBlock sp;
		sp.init();
		fileDisk.write((char*)&sp, sizeof(sp));
		fileDisk.seekg(diskSize, ios::beg);//偏移磁盘大小
		fileDisk << "1";//写入内容使得文件达到目标大小
		int ret = alloc_inode(1024,root);//设置根节点
	}
	fileDisk.seekg(0, ios::beg);//到文件头
	fileDisk.read((char*)&s_block, sizeof(s_block));
	s_block.printInfo();
}
FileSystem::~FileSystem()
{
	cout << sizeof(iNode)<< endl;
	fileDisk.close();
}

//申请iNode节点,size单位为Byte
int FileSystem::alloc_inode(unsigned long size, iNode &node,bool is_dentry=false)
{
	if (size == 0||size>s_block.maxBytes||s_block.inode_remain==0){
		return -1;
	}
	int blocks_needed = ceil(size / s_block.blockSize);//需要存储内容的块数
	fileDisk.seekg(s_block.inodemap_pos, ios::beg);
	for (int i = 1; i <= ceil(s_block.inode_num/8); i++){
		char bit;
		fileDisk.read(&bit,8);
	}
	node = iNode();
	int block_node_num = s_block.blockSize / sizeof(unsigned int);//每块可存的iNode的id的数目
	if (blocks_needed <= 10){
		//可以在10个直接块中放下
	}
	else if (blocks_needed <= (10 + block_node_num)){
		//加上一个一次间接块

	}
	else if (blocks_needed <= (10 + block_node_num + block_node_num*block_node_num)){
		//加上一个二次间接块
	}
	else if (blocks_needed <= (10 + block_node_num + block_node_num*block_node_num + block_node_num*block_node_num*block_node_num)){
		//加上一个三次间接块
	}
	return 1;
}