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
	}
	seekAndGet<superBlock>(0,s_block);
	seekAndGet<iNode>(s_block.inode_table, root);
	s_block.printInfo();
}
FileSystem::~FileSystem()
{
	root.printInfo();
	root.printBlock();
	fileDisk.close();
}

//申请iNode节点,size单位为Byte
int FileSystem::alloc_inode(unsigned long size, iNode &node,bool is_dentry)
{
	unsigned int blocks_needed = ceil((double)size / s_block.blockSize);//需要存储内容的块数
	//最大大小超过限制，iNode节点不足时
	if (size == 0 || size>s_block.maxBytes || s_block.inode_remain == 0){
		return -1;
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

int FileSystem::write_inode(iNode node){
	//检查范围
	if (node.ino<1 || node.ino>s_block.inode_num){
		return -1;
	}
	fileDisk.seekg(s_block.inode_table + (node.ino - 1)*sizeof(iNode), ios::beg);
	fileDisk.write((char *)&node, sizeof(iNode));
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