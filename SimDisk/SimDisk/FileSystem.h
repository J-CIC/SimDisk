#pragma once
#include "stdafx.h"
#include "superBlock.h"
#include "iNode.h"
#include "dentry.h"
#include "User.h"
#include <Windows.h>
#include <map>
#include "toolkit.h"

class FileSystem
{
public:
	const static int FOLDER_TYPE = 1;
	const static int FILE_TYPE = 2;
	HANDLE hMapFile;//消息共享内存
	HANDLE usrMapFile;//用户token共享内存
	static const string fileName;//磁盘所在的文件名
	User currUser;//当前用户名字
	int curr_uid;//当前用户id
	vector<User> userLists;//用户列表
	map<string,int> loginUserLists;//登录的用户列表
	superBlock s_block;//超级块
	iNode root;//根节点
	dentry root_dentry;//根目录
	dentry *curr_dentry;//工作目录，即当前目录
	int serve();//服务
	int parseCmd(string cmd);//解析命令
	void outputPrompt();//解析命令输出提示符
	FileSystem();
	~FileSystem();
private:
	fstream fileDisk;//磁盘的文件流
	int init_root_dentry();//初始化根目录
	int init_user();//初始化用户
	int save_user();//保存用户
	int alloc_inode(unsigned long size,iNode &node,bool is_dentry = false);//申请iNode节点,size为字节
	int alloc_blocks(int num, vector<unsigned int> &list);
	int destroy_inode(int id);//销毁iNode节点
	int destroy_block(int id);//销毁block
	int withdraw_node(iNode node);//删除文件后的收回文件对应的节点和块
	int read_inode(int ino, iNode &node);//读取iNode节点信息
	int write_inode(iNode &node);//更新iNode信息
	int clearBlockContent(vector<unsigned int> list);//清空块内容
	int auth(string username, string pwd);//登录认证操作
	int generate_token(int u_id);//生成认证token，用于每次校验
	int get_shell_user();//获取当前的shell的用户，返回的是userlists中的下标
	int copy(string from, string to);
	int newfile(string name, unsigned long size=0);//创建文件
	int mkdir(string name);//创建文件夹
	int rd(string filename, bool force=false);//删除文件夹
	int del(string filename);//删除文件
	int cat(string filename);//读取文件并显示
	int cd(string filename);//切换工作目录
	int ls(string filename="");//展示目录
	vector<string> getUsers();//获取用户列表
	//用名称寻找目录或文件，指针引用，因为可能要修改地址，type默认寻找文件夹
	int findDentryWithName(string name, dentry *&p_dentry, int type = FOLDER_TYPE);
	//寻找目录或文件，指针引用，因为可能要修改地址，type默认寻找文件夹
	int findDentry(vector<string> list, dentry *&p_dentry, char firstChar, int type = FOLDER_TYPE);
	int InitDentry(dentry& p_dentry);//初始化dentry项
	int SaveDentry(dentry& p_dentry);//初始化dentry项
	template<typename T> int seekAndGet(unsigned long pos, T &item);//定位指针并读取
	template<typename T> int seekAndSave(unsigned long pos, T &item);//定位指针并存储
	int readBlockIds(iNode inode, vector<unsigned int> &blocks_list);//读取对应iNode的内容块，不包含间接块
};
