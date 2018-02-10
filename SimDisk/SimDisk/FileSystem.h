#pragma once
#include "stdafx.h"
#include "superBlock.h"
#include "iNode.h"
#include "dentry.h"
#include "file.h"
#include "toolkit.h"

class FileSystem
{
public:
	static const string fileName;//磁盘所在的文件名
	superBlock s_block;//超级块
	iNode root;//根节点
	dentry root_dentry;//根目录
	dentry *curr_dentry;//工作目录，即当前目录
	FileSystem();
	~FileSystem();
private:
	fstream fileDisk;//磁盘的文件流
	int init_root_dentry();//初始化根目录
	int alloc_inode(unsigned long size,iNode &node,bool is_dentry = false);//申请iNode节点,size为字节
	int alloc_blocks(int num, vector<unsigned int> &list);
	int destroy_inode(int id);//销毁iNode节点
	int destroy_block(int id);//销毁block
	int read_inode(int ino, iNode &node);//读取iNode节点信息
	int write_inode(iNode &node);//更新iNode信息
	int getSubDentry(dentry& p_dir);//获取子目录
	int clearBlockContent(vector<unsigned int> list);//清空块内容
	int mkdir(string name);//创建文件夹
	int setCurrDir(vector<string> list);//切换当前目录
	int findDentry(vector<string> list, dentry *&p_dentry , char firstChar);//寻找目录，指针引用，因为可能要修改地址
	int InitDentry(dentry& p_dentry);//初始化dentry项
	int SaveDentry(dentry& p_dentry);//初始化dentry项
	template<typename T> int seekAndGet(unsigned long pos, T &item);//定位指针并读取
	template<typename T> int seekAndSave(unsigned long pos, T &item);//定位指针并存储
	int readBlockIds(iNode inode, vector<unsigned int> &blocks_list);//读取对应iNode的内容块，不包含间接块
};
