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
	const static int FOLDER_TYPE = 1;
	const static int FILE_TYPE = 2;
	static const string fileName;//�������ڵ��ļ���
	superBlock s_block;//������
	iNode root;//���ڵ�
	dentry root_dentry;//��Ŀ¼
	dentry *curr_dentry;//����Ŀ¼������ǰĿ¼
	int serve();//����
	int parseCmd(string cmd);//��������
	void outputPrompt();
	FileSystem();
	~FileSystem();
private:
	fstream fileDisk;//���̵��ļ���
	int init_root_dentry();//��ʼ����Ŀ¼
	int alloc_inode(unsigned long size,iNode &node,bool is_dentry = false);//����iNode�ڵ�,sizeΪ�ֽ�
	int alloc_blocks(int num, vector<unsigned int> &list);
	int destroy_inode(int id);//����iNode�ڵ�
	int destroy_block(int id);//����block
	int withdraw_node(iNode node);//ɾ���ļ�����ջ��ļ���Ӧ�Ľڵ�Ϳ�
	int read_inode(int ino, iNode &node);//��ȡiNode�ڵ���Ϣ
	int write_inode(iNode &node);//����iNode��Ϣ
	int getSubDentry(dentry& p_dir);//��ȡ��Ŀ¼
	int clearBlockContent(vector<unsigned int> list);//��տ�����
	int copy(string from, string to);
	int newfile(string name, unsigned long size=0);//�����ļ�
	int mkdir(string name);//�����ļ���
	int rd(string filename, bool force=false);//ɾ���ļ���
	int del(string filename);//ɾ���ļ�
	int cat(string filename);//��ȡ�ļ�����ʾ
	int cd(string filename);//�л�����Ŀ¼
	//������Ѱ��Ŀ¼���ļ���ָ�����ã���Ϊ����Ҫ�޸ĵ�ַ��typeĬ��Ѱ���ļ���
	int findDentryWithName(string name, dentry *&p_dentry, int type = FOLDER_TYPE);
	//Ѱ��Ŀ¼���ļ���ָ�����ã���Ϊ����Ҫ�޸ĵ�ַ��typeĬ��Ѱ���ļ���
	int findDentry(vector<string> list, dentry *&p_dentry, char firstChar, int type = FOLDER_TYPE);
	int InitDentry(dentry& p_dentry);//��ʼ��dentry��
	int SaveDentry(dentry& p_dentry);//��ʼ��dentry��
	template<typename T> int seekAndGet(unsigned long pos, T &item);//��λָ�벢��ȡ
	template<typename T> int seekAndSave(unsigned long pos, T &item);//��λָ�벢�洢
	int readBlockIds(iNode inode, vector<unsigned int> &blocks_list);//��ȡ��ӦiNode�����ݿ飬��������ӿ�
};
