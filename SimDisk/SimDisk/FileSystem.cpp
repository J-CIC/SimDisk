#include "stdafx.h"
#include "FileSystem.h"
const string FileSystem::fileName = "disk.txt";


FileSystem::FileSystem()
{
	long int diskSize = 100 * 1024 * 1024 * 1;//���̴�С,Byte
	fileDisk.open(fileName, ios::binary | ios::in| ios::out);
	if (!fileDisk){//�ļ�������
		fileDisk.open(fileName, ios::out | ios::binary);
		//�����³��������Ϣ,��д��
		superBlock sp;
		sp.init();
		fileDisk.write((char*)&sp, sizeof(sp));
		fileDisk.seekg(diskSize, ios::beg);//ƫ�ƴ��̴�С
		fileDisk << "1";//д������ʹ���ļ��ﵽĿ���С
		int ret = alloc_inode(1024,root);//���ø��ڵ�
	}
	fileDisk.seekg(0, ios::beg);//���ļ�ͷ
	fileDisk.read((char*)&s_block, sizeof(s_block));
	s_block.printInfo();
}
FileSystem::~FileSystem()
{
	cout << sizeof(iNode)<< endl;
	fileDisk.close();
}

//����iNode�ڵ�,size��λΪByte
int FileSystem::alloc_inode(unsigned long size, iNode &node,bool is_dentry=false)
{
	if (size == 0||size>s_block.maxBytes||s_block.inode_remain==0){
		return -1;
	}
	int blocks_needed = ceil(size / s_block.blockSize);//��Ҫ�洢���ݵĿ���
	fileDisk.seekg(s_block.inodemap_pos, ios::beg);
	for (int i = 1; i <= ceil(s_block.inode_num/8); i++){
		char bit;
		fileDisk.read(&bit,8);
	}
	node = iNode();
	int block_node_num = s_block.blockSize / sizeof(unsigned int);//ÿ��ɴ��iNode��id����Ŀ
	if (blocks_needed <= 10){
		//������10��ֱ�ӿ��з���
	}
	else if (blocks_needed <= (10 + block_node_num)){
		//����һ��һ�μ�ӿ�

	}
	else if (blocks_needed <= (10 + block_node_num + block_node_num*block_node_num)){
		//����һ�����μ�ӿ�
	}
	else if (blocks_needed <= (10 + block_node_num + block_node_num*block_node_num + block_node_num*block_node_num*block_node_num)){
		//����һ�����μ�ӿ�
	}
	return 1;
}