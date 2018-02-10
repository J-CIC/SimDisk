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
		s_block = sp;
		seekAndSave<superBlock>(fileDisk.tellg(), sp);
		//生成位图
		fileDisk.seekg(sp.inodemap_pos, ios::beg);
		unsigned char * map = new unsigned char[sp.blockSize];
		memset(map, 0, sp.blockSize*sizeof(char));
		for (int i = 0; i < ceil(sp.inode_num / (8 * 1024)); i++){
			fileDisk.write((char *)map, sp.blockSize);
		}
		fileDisk.seekg(sp.bitmap_pos, ios::beg);
		for (int i = 0; i < ceil(sp.block_num / (8 * 1024)); i++){
			fileDisk.write((char *)map, sp.blockSize);
		}
		fileDisk.seekg(1000*1024, ios::beg);//偏移磁盘大小
		fileDisk.write("1",1);//写入内容使得文件达到目标大小
		fileDisk.close();
		fileDisk.open(fileName, ios::binary | ios::in | ios::out);
		int ret = alloc_inode(272384, root, true);//设置根节点
		root.i_size = 0;
		seekAndSave<iNode>(s_block.inode_table, root);
	}
	seekAndGet<superBlock>(0, s_block);
	seekAndGet<iNode>(s_block.inode_table, root);
	init_root_dentry();
	int ret = mkdir("var"); 
	root_dentry.showDentry();
	cout << "ret is : " << ret << endl;
}
FileSystem::~FileSystem()
{
	//s_block.printInfo();
	//root.printInfo();
	//root.printBlock();
	fileDisk.close();
}

//申请iNode节点,size单位为Byte
int FileSystem::alloc_inode(unsigned long size, iNode &node,bool is_dentry)
{
	unsigned int blocks_needed = ceil((double)size / s_block.blockSize);//需要存储内容的块数
	//最大大小超过限制，iNode节点不足时
	if (size>s_block.maxBytes || s_block.inode_remain == 0){
		return -1;
	}
	if (size == 0){
		blocks_needed = 10;
	}
	fileDisk.seekg(s_block.inodemap_pos, ios::beg);//挪动到位图位置
	bool is_end = false;
	int inode_no = 1;//使用的iNode号码
	unsigned char* bytes = new unsigned char[s_block.blockSize];//读取的数组

	int block_node_num = s_block.blockSize / sizeof(unsigned int);//每块可存的iNode的id的数目
	if (blocks_needed <= 10){
		//可以在10个直接块中放下
	}
	else if (blocks_needed <= (10 + block_node_num)){
		//加上一个一次间接块
		blocks_needed += 1;
	}
	else if (blocks_needed <= (10 + block_node_num + block_node_num*block_node_num)){
		//加上一个二次间接块
		blocks_needed += 2 + block_node_num;
	}
	else if (blocks_needed <= (10 + block_node_num + block_node_num*block_node_num + block_node_num*block_node_num*block_node_num)){
		//加上一个三次间接块
		blocks_needed += 3 + 2 * block_node_num + block_node_num*block_node_num;
	}

	//寻找空闲的iNode
	for (int i = 0; i < ceil(s_block.inode_num / (8 * s_block.blockSize)); i++){
		if (is_end)
			break;//遍历完所有位图后退出
		fileDisk.read((char *)bytes, s_block.blockSize);//一次读取一块
		unsigned char mask = 1;
		mask = mask << 7;
		for (int index = 0; index<s_block.blockSize; index++){
			if (is_end)
				break;//遍历完所有位图后退出
			unsigned char byte = bytes[index];
			for (int j = 0; j < 8; j++){
				if (!((byte << j)&mask)){
					//若有空位
					bytes[index] = byte | (1 << (7-j));
					fileDisk.seekg(s_block.inodemap_pos + i*s_block.blockSize, ios::beg);
					fileDisk.write((char *)bytes, s_block.blockSize);//位图改变整块写回
					is_end = true;//结束
					break;
				}
				inode_no += 1;
				if (inode_no>s_block.inode_num){
					is_end = true;
					break;
				}
			}
		}
	}
	vector<unsigned int> block_list;
	alloc_blocks(blocks_needed, block_list);//申请磁盘块
	node = iNode(inode_no,size,blocks_needed,block_list);
	if (is_dentry){
		unsigned short mode = 1;
		mode = mode << 14;
		node.i_mode = mode;//设定为文件夹
	}
	write_inode(node);
	//更新相应的超级块信息
	s_block.inode_remain--;
	s_block.block_remain -= blocks_needed;
	seekAndSave<superBlock>(0, s_block);
	return 1;
}

int FileSystem::alloc_blocks(int num, vector<unsigned int> &list){
	fileDisk.seekg(s_block.bitmap_pos, ios::beg);//挪动到位图位置
	bool is_end = false;
	unsigned int block_no = 1;//使用的block号码
	unsigned char* bytes = new unsigned char[s_block.blockSize];//读取的数组
	//寻找空闲的block
	for (int i = 0; i < ceil(s_block.block_num / (8 * s_block.blockSize)); i++){
		if (is_end)
			break;//遍历完所有位图后退出
		fileDisk.read((char *)bytes, s_block.blockSize);//一次读取一块
		bool modify = false;//是否有更改需要写回磁盘
		unsigned char mask = 1;
		mask = mask << 7;
		for (int index = 0; index<s_block.blockSize; index++){
			if (is_end)
				break;//遍历完所有位图后退出
			unsigned char byte = bytes[index];
			for (int j = 0; j < 8; j++){
				if (block_no < s_block.first_data_block_no){
					block_no++;
					continue;
				}
				if (!((byte << j)&mask)){
					//若有空位
					bytes[index] = byte | (1 << (7 - j));
					byte = bytes[index];//更新byte
					list.push_back(block_no);
					modify = true;
					if (num == list.size()){
						is_end = true;//结束
						break;
					}
				}
				block_no += 1;
				if (block_no>s_block.block_num){
					is_end = true;
					break;
				}
			}
		}
		if (modify){
			fileDisk.seekg(s_block.bitmap_pos + i*s_block.blockSize, ios::beg);
			fileDisk.write((char *)bytes, s_block.blockSize);//位图改变整块写回
		}
	}
	clearBlockContent(list);//清除分配的块中的内容
	//排放block
	int block_per_num = s_block.blockSize / sizeof(unsigned int);//每块可存的block的id的数目
	if (list.size() <= 10){
		//可以在10个直接块中放下
	}
	else if (list.size() <= (11 + block_per_num)){
		//加上一个一次间接块
		int cnt = 0, i;//写入次数和写入下标
		unsigned int once_no = list[10];
		fileDisk.seekg((once_no-1)*s_block.blockSize);//去到正确的偏移位置
		for (i = 11; cnt< block_per_num&&i<list.size(); i++, cnt++){
			fileDisk.write((char*)&list[i], sizeof(unsigned int));
		}
		list.resize(11);
	}
	else if (list.size() <= (12 + 2*block_per_num + block_per_num*block_per_num)){
		//加上一个一次间接块
		int cnt=0,i;//写入次数和写入下标
		unsigned int once_no = list[10];
		unsigned int twice_no = list[11];
		fileDisk.seekg((once_no - 1)*s_block.blockSize);//去到正确的偏移位置
		for (i = 12; cnt <block_per_num ; i++,cnt++){
			fileDisk.write((char*)&list[i], sizeof(unsigned int));
		}
		//加上一个二次间接块
		vector<unsigned int> once_in_twice;//二次间接块中的一次间接块
		fileDisk.seekg((twice_no - 1)*s_block.blockSize);//去到正确的偏移位置
		cnt = 0;
		for (; cnt<block_per_num; i++,cnt++){
			once_in_twice.push_back(list[i]);
			fileDisk.write((char*)&list[i], sizeof(unsigned int));
		}
		//二次间接块的一次间接块
		for (auto item : once_in_twice){
			fileDisk.seekg((item - 1)*s_block.blockSize);//去到正确的偏移位置
			cnt = 0;
			for (; cnt<block_per_num&&i<list.size(); i++, cnt++){
				once_in_twice.push_back(list[i]);
				fileDisk.write((char*)&list[i], sizeof(unsigned int));
			}
		}
		list.resize(12);
	}
	else if (list.size() <= (13 + 3*block_per_num + 2*block_per_num*block_per_num + block_per_num*block_per_num*block_per_num)){
		//加上一个一次间接块
		int cnt = 0, i;//写入次数和写入下标
		unsigned int once_no = list[10];
		unsigned int twice_no = list[11];
		unsigned int third_no = list[12];
		fileDisk.seekg((once_no - 1)*s_block.blockSize);//去到正确的偏移位置
		for (i = 13; cnt <block_per_num; i++, cnt++){
			fileDisk.write((char*)&list[i], sizeof(unsigned int));
		}
		//加上一个二次间接块
		vector<unsigned int> once_in_twice;//二次间接块中的一次间接块
		fileDisk.seekg((twice_no - 1)*s_block.blockSize);//去到正确的偏移位置
		cnt = 0;
		for (; cnt<block_per_num; i++, cnt++){
			once_in_twice.push_back(list[i]);
			fileDisk.write((char*)&list[i], sizeof(unsigned int));
		}
		//二次间接块的一次间接块
		for (auto item : once_in_twice){
			fileDisk.seekg((item - 1)*s_block.blockSize);//去到正确的偏移位置
			cnt = 0;
			for (; cnt<block_per_num; i++, cnt++){
				once_in_twice.push_back(list[i]);
				fileDisk.write((char*)&list[i], sizeof(unsigned int));
			}
		}
		//加上一个三次间接块
		vector<unsigned int> twice_in_third;//三次间接块中的一次间接块
		fileDisk.seekg((third_no - 1)*s_block.blockSize);//去到正确的偏移位置
		cnt = 0;
		for (; cnt<block_per_num; i++, cnt++){
			twice_in_third.push_back(list[i]);
			fileDisk.write((char*)&list[i], sizeof(unsigned int));
		}
		once_in_twice.clear();
		for (auto item : twice_in_third){
			fileDisk.seekg((item - 1)*s_block.blockSize);//去到正确的偏移位置
			cnt = 0;
			for (; cnt<block_per_num; i++, cnt++){
				once_in_twice.push_back(list[i]);
				fileDisk.write((char*)&list[i], sizeof(unsigned int));
			}
		}
		for (auto item : once_in_twice){
			fileDisk.seekg((item - 1)*s_block.blockSize);//去到正确的偏移位置
			cnt = 0;
			for (; cnt<block_per_num&&i<list.size(); i++, cnt++){
				once_in_twice.push_back(list[i]);
				fileDisk.write((char*)&list[i], sizeof(unsigned int));
			}
		}
		list.resize(13);
	}
	return 1;
}

int FileSystem::write_inode(iNode &node){
	//检查范围
	if (node.ino<1 || node.ino>s_block.inode_num){
		return -1;
	}
	node.i_mtime = time(NULL);//修改时间
	node.i_atime = node.i_mtime;//访问时间
	seekAndSave<iNode>(s_block.inode_table + (node.ino - 1)*sizeof(iNode), node);
	return 1;
}

template<typename T>
int FileSystem::seekAndGet(unsigned long pos, T& item){
	fileDisk.seekg(pos,ios::beg);
	fileDisk.read((char*)&item, sizeof(T));
	return 1;
}

template<typename T>
int FileSystem::seekAndSave(unsigned long pos, T& item){
	fileDisk.seekg(pos, ios::beg);
	fileDisk.write((char*)&item, sizeof(T));
	return 1;
}


//收回block，即修改位图
int FileSystem::destroy_block(int id)
{
	unsigned int byte_pos = floor(id / 8);//偏移字节
	int byte_idx = id % 8;//字节内地址下标
	if (byte_idx == 0){
		//整除则回退一位
		byte_pos--;
		byte_idx = 8;
	}
	char byte=0;//存放内容的位图
	unsigned char mask = 255;
	unsigned char xor_mask = 1;
	xor_mask = xor_mask << (8 - byte_idx);
	mask = mask^xor_mask;
	//读取并置0
	fileDisk.seekg(s_block.bitmap_pos + byte_pos);
	fileDisk.read((char *)&byte, 1);
	byte = byte & mask;
	//写回
	fileDisk.seekg(s_block.bitmap_pos + byte_pos);
	fileDisk.write((char *)&byte, 1);
	return 1;
}

//收回iNode
int FileSystem::destroy_inode(int id)
{
	unsigned int byte_pos = floor(id / 8);//偏移字节
	int byte_idx = id % 8;//字节内地址下标
	if (byte_idx == 0){
		//整除则回退一位
		byte_pos--;
		byte_idx = 8;
	}
	char byte = 0;//存放内容的位图
	unsigned char mask = 255;
	unsigned char xor_mask = 1;
	xor_mask = xor_mask << (8 - byte_idx);
	mask = mask^xor_mask;
	//读取并置0
	fileDisk.seekg(s_block.inodemap_pos + byte_pos);
	fileDisk.read((char *)&byte, 1);
	byte = byte & mask;
	//写回
	fileDisk.seekg(s_block.inodemap_pos + byte_pos);
	fileDisk.write((char *)&byte, 1);
	return 1;
}

//批量清理块中的内容
int FileSystem::clearBlockContent(vector<unsigned int> list)
{
	//生成空内容并写入
	char bytes[1024];
	memset(bytes, 0, 1024);
	for (auto item : list)
	{
		fileDisk.seekg((item-1)*s_block.blockSize, ios::beg);
		fileDisk.write(bytes, 1024);
	}
	return 1;
}


//初始化根dentry
int FileSystem::init_root_dentry()
{
	root_dentry = dentry();
	root_dentry.inode = root;
	root_dentry.fileName = "/";
	root_dentry.setParent(root_dentry);
	curr_dentry = &root_dentry;
	getSubDentry(root_dentry);
	return 1;
}

//读取子目录
int FileSystem::getSubDentry(dentry& p_dir)
{
	vector<unsigned int> blocks_list;
	iNode p_node = p_dir.inode;
	readBlockIds(p_node, blocks_list);//读取内容块列表
	InitDentry(p_dir);
	file p_file = file(p_node,blocks_list);
	return 1;
}

//读取对应iNode的内容块，不包含间接块
int FileSystem::readBlockIds(iNode inode, vector<unsigned int> &blocks_list)
{
	unsigned int block_num = inode.i_blocks;
	//十个间接块
	for (int i = 0; i < 10; i++){
		if (inode.i_zone[i] == 0){
			break;
		}
		blocks_list.push_back(inode.i_zone[i]);
	}
	//一次间接块
	if (inode.i_zone[10] != 0){
		int pos = (inode.i_zone[10] - 1)*s_block.blockSize;
		fileDisk.seekg(pos, ios::beg);
		int loop_times = ceil(s_block.blockSize / sizeof(unsigned int));
		for (int i = 0; i < loop_times; i++){
			unsigned int blcoks_num = 0;
			fileDisk.read((char*)&blcoks_num, sizeof(unsigned int));
			if (blcoks_num>0){
				blocks_list.push_back(blcoks_num);
			}
		}
	}
	//二次间接块
	if (inode.i_zone[11] != 0){
		int pos = (inode.i_zone[11] - 1)*s_block.blockSize;
		fileDisk.seekg(pos, ios::beg);
		int loop_times = ceil(s_block.blockSize / sizeof(unsigned int));//计算每块能放下的数目
		vector<unsigned int> once_list;
		for (int i = 0; i < loop_times; i++){
			unsigned int blcoks_num = 0;
			fileDisk.read((char*)&blcoks_num, sizeof(unsigned int));
			if (blcoks_num>0){
				once_list.push_back(blcoks_num);//存入中间缓存列表
			}
		}
		for (auto item : once_list){
			int pos = (item - 1)*s_block.blockSize;
			fileDisk.seekg(pos, ios::beg);
			for (int i = 0; i < loop_times; i++){
				unsigned int blcoks_num = 0;
				fileDisk.read((char*)&blcoks_num, sizeof(unsigned int));//计算每块能放下的数目
				if (blcoks_num>0){
					blocks_list.push_back(blcoks_num);//读出最终的块号
				}
			}
		}
	}
	//三次间接块
	if (inode.i_zone[12] != 0){
		int pos = (inode.i_zone[11] - 1)*s_block.blockSize;
		fileDisk.seekg(pos, ios::beg);
		int loop_times = ceil(s_block.blockSize / sizeof(unsigned int));//计算每块能放下的数目
		vector<unsigned int> twice_list, once_list;
		for (int i = 0; i < loop_times; i++){
			unsigned int blcoks_num = 0;
			fileDisk.read((char*)&blcoks_num, sizeof(unsigned int));
			if (blcoks_num>0){
				twice_list.push_back(blcoks_num);//存入二次中间缓存列表
			}
		}
		for (auto item : twice_list){
			int pos = (item - 1)*s_block.blockSize;
			fileDisk.seekg(pos, ios::beg);
			for (int i = 0; i < loop_times; i++){
				unsigned int blcoks_num = 0;
				fileDisk.read((char*)&blcoks_num, sizeof(unsigned int));
				if (blcoks_num>0){
					once_list.push_back(blcoks_num);//放入一次缓存表
				}
			}
		}
		for (auto item : once_list){
			int pos = (item - 1)*s_block.blockSize;
			fileDisk.seekg(pos, ios::beg);
			for (int i = 0; i < loop_times; i++){
				unsigned int blcoks_num = 0;
				fileDisk.read((char*)&blcoks_num, sizeof(unsigned int));//计算每块能放下的数目
				if (blcoks_num>0){
					blocks_list.push_back(blcoks_num);//读出最终的块号
				}
			}
		}
	}
	return 1;
}

//创建目录
int FileSystem::mkdir(string filename)
{
	vector<string> dir_list;
	SplitString(filename,dir_list,"/");
	dentry *temp_dentry;//暂存的变量
	string folder_name = "";
	//获取创建的文件夹名字
	folder_name = dir_list[dir_list.size() - 1];
	int ret = findDentry(dir_list, temp_dentry, filename[0]);//判断是否存在
	if (ret == 1){
		//存在
		return -1;
	}
	//去掉文件夹名字后的目录字符串
	dir_list.resize(dir_list.size() - 1);
	ret = findDentry(dir_list, temp_dentry,filename[0]);//寻找父文件夹
	if (ret == 0){
		return 0;
	}
	iNode dir_node;
	ret = alloc_inode(0, dir_node, true);
	if (ret == -1){
		return ret;
	}
	dir s_dir(folder_name, dir_node.ino);//生成dir
	dentry created_dentry(s_dir.dir_name, dir_node);//生成dentry项
	created_dentry.setParent(*temp_dentry);
	temp_dentry->addChild(created_dentry);//加入父目录的子项
	SaveDentry(*temp_dentry);
	


	if (folder_name.length() <= 0){
		return -1;//文件名长度不合法
	}
	return 1;
}

//设定工作目录
int FileSystem::setCurrDir(vector<string> list)
{

	return 1;
}

//寻找目录项
int FileSystem::findDentry(vector<string> list,dentry *&p_dentry,char firstChar)
{
	p_dentry = curr_dentry;
	if (list.size() == 0){
		return 1;//直接当前目录下创建
	}
	if (firstChar == '/'){
		p_dentry = &root_dentry;//从根目录开始
		list.erase(list.begin());//由于根目录开始删除首个空的位置
	}
	for (auto item : list){
		if (item == ".."){//父层目录
			p_dentry = p_dentry->parent;
		}
		else if (item == "."){//当前目录
			p_dentry = p_dentry;
		}
		else{//遍历寻找目录
			if (p_dentry->child_list.size() == 0){
				//子目录数目为0，可能是尚未读取目录
				InitDentry(*p_dentry);
			}
			for (auto child_dentry : p_dentry->child_list){
				if (child_dentry.fileName == item){
					//如果名字对上了，还要判断文件类型
					if (child_dentry.is_dir()){
						p_dentry = &child_dentry;
						return 1;
					}
				}
			}
		}
	}
	return 0;//没找到
}

//初始化dentry
int FileSystem::InitDentry(dentry & p_dentry){
	readBlockIds(p_dentry.inode, p_dentry.block_list);//保存block_list
	int dir_num_per_block = s_block.blockSize / sizeof(dir);//每一个块能存放的dir数目
	int count = 0,max_dir = floor(p_dentry.inode.i_size / sizeof(dir));
	dir t_dir;
	for (auto b_idx : p_dentry.block_list){
		if (count == max_dir){
			break;
		}
		int base_pos = (b_idx - 1)*s_block.blockSize;//基础偏移地址
		for (int i = 0; i < dir_num_per_block&&count<max_dir; i++){
			seekAndGet<dir>(base_pos + i*sizeof(dir), t_dir);
			if (t_dir.ino>0 && t_dir.ino <= s_block.inode_num){
				dentry t_dentry;
				read_inode(t_dir.ino, t_dentry.inode);//读取iNode
				t_dentry.fileName = t_dir.dir_name;
				t_dentry.setParent(p_dentry);
				p_dentry.addChild(t_dentry);
				count++;
				if (count == max_dir){
					break;
				}
			}
		}
	}
	return 1;
}

//保存dentry
int FileSystem::SaveDentry(dentry & p_dentry){
	p_dentry.inode.i_size = p_dentry.child_list.size()*sizeof(dir);//更新占用大小
	write_inode(p_dentry.inode);//保存iNode
	int dir_num_per_block = s_block.blockSize / sizeof(dir);//每一个块能存放的dir数目
	vector<dir> save_list = p_dentry.getDirList();
	int count = 0, max_dir = save_list.size();
	for (auto b_idx : p_dentry.block_list){
		if (count == max_dir){
			break;
		}
		int base_pos = (b_idx - 1)*s_block.blockSize;//基础偏移地址
		for (int i = 0; i < dir_num_per_block && count<max_dir; i++){
			seekAndSave<dir>(base_pos + i*sizeof(dir), save_list[count]);
			count++;
		}
	}
	return 1;
}

//读取iNode节点
int FileSystem::read_inode(int ino, iNode &node){
	unsigned long pos = s_block.inode_table + (ino-1)*sizeof(iNode);
	seekAndGet<iNode>(pos, node);
	return 1;
}