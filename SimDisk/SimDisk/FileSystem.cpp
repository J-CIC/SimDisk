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
		s_block = sp;
		seekAndSave<superBlock>(fileDisk.tellg(), sp);
		//����λͼ
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
		fileDisk.seekg(1000*1024, ios::beg);//ƫ�ƴ��̴�С
		fileDisk.write("1",1);//д������ʹ���ļ��ﵽĿ���С
		fileDisk.close();
		fileDisk.open(fileName, ios::binary | ios::in | ios::out);
		int ret = alloc_inode(272384, root, true);//���ø��ڵ�
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

//����iNode�ڵ�,size��λΪByte
int FileSystem::alloc_inode(unsigned long size, iNode &node,bool is_dentry)
{
	unsigned int blocks_needed = ceil((double)size / s_block.blockSize);//��Ҫ�洢���ݵĿ���
	//����С�������ƣ�iNode�ڵ㲻��ʱ
	if (size == 0 || size>s_block.maxBytes || s_block.inode_remain == 0){
		return -1;
	}

	fileDisk.seekg(s_block.inodemap_pos, ios::beg);//Ų����λͼλ��
	bool is_end = false;
	int inode_no = 1;//ʹ�õ�iNode����
	unsigned char* bytes = new unsigned char[s_block.blockSize];//��ȡ������
	int block_node_num = s_block.blockSize / sizeof(unsigned int);//ÿ��ɴ��iNode��id����Ŀ

	if (blocks_needed <= 10){
		//������10��ֱ�ӿ��з���
	}
	else if (blocks_needed <= (10 + block_node_num)){
		//����һ��һ�μ�ӿ�
		blocks_needed += 1;
	}
	else if (blocks_needed <= (10 + block_node_num + block_node_num*block_node_num)){
		//����һ�����μ�ӿ�
		blocks_needed += 2 + block_node_num;
	}
	else if (blocks_needed <= (10 + block_node_num + block_node_num*block_node_num + block_node_num*block_node_num*block_node_num)){
		//����һ�����μ�ӿ�
		blocks_needed += 3 + 2 * block_node_num + block_node_num*block_node_num;
	}
	//Ѱ�ҿ��е�iNode
	for (int i = 0; i < ceil(s_block.inode_num / (8 * s_block.blockSize)); i++){
		if (is_end)
			break;//����������λͼ���˳�
		fileDisk.read((char *)bytes, s_block.blockSize);//һ�ζ�ȡһ��
		unsigned char mask = 1;
		mask = mask << 7;
		for (int index = 0; index<s_block.blockSize; index++){
			if (is_end)
				break;//����������λͼ���˳�
			unsigned char byte = bytes[index];
			for (int j = 0; j < 8; j++){
				if (!((byte << j)&mask)){
					//���п�λ
					bytes[index] = byte | (1 << (7-j));
					fileDisk.seekg(s_block.inodemap_pos + i*s_block.blockSize, ios::beg);
					fileDisk.write((char *)bytes, s_block.blockSize);//λͼ�ı�����д��
					is_end = true;//����
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
	alloc_blocks(blocks_needed, block_list);//������̿�
	node = iNode(inode_no,size,blocks_needed,block_list);
	if (is_dentry){
		unsigned short mode = 1;
		mode = mode << 14;
		node.i_mode = mode;//�趨Ϊ�ļ���
	}
	write_inode(node);
	//������Ӧ�ĳ�������Ϣ
	s_block.inode_remain--;
	s_block.block_remain -= blocks_needed;
	seekAndSave<superBlock>(0, s_block);
	return 1;
}

int FileSystem::alloc_blocks(int num, vector<unsigned int> &list){
	fileDisk.seekg(s_block.bitmap_pos, ios::beg);//Ų����λͼλ��
	bool is_end = false;
	unsigned int block_no = 1;//ʹ�õ�block����
	unsigned char* bytes = new unsigned char[s_block.blockSize];//��ȡ������
	//Ѱ�ҿ��е�block
	for (int i = 0; i < ceil(s_block.block_num / (8 * s_block.blockSize)); i++){
		if (is_end)
			break;//����������λͼ���˳�
		fileDisk.read((char *)bytes, s_block.blockSize);//һ�ζ�ȡһ��
		bool modify = false;//�Ƿ��и�����Ҫд�ش���
		unsigned char mask = 1;
		mask = mask << 7;
		for (int index = 0; index<s_block.blockSize; index++){
			if (is_end)
				break;//����������λͼ���˳�
			unsigned char byte = bytes[index];
			for (int j = 0; j < 8; j++){
				if (block_no < s_block.first_data_block_no){
					block_no++;
					continue;
				}
				if (!((byte << j)&mask)){
					//���п�λ
					bytes[index] = byte | (1 << (7 - j));
					byte = bytes[index];//����byte
					list.push_back(block_no);
					modify = true;
					if (num == list.size()){
						is_end = true;//����
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
			fileDisk.write((char *)bytes, s_block.blockSize);//λͼ�ı�����д��
		}
	}

	//�ŷ�block
	int block_per_num = s_block.blockSize / sizeof(unsigned int);//ÿ��ɴ��block��id����Ŀ
	if (list.size() <= 10){
		//������10��ֱ�ӿ��з���
	}
	else if (list.size() <= (11 + block_per_num)){
		//����һ��һ�μ�ӿ�
		int cnt = 0, i;//д�������д���±�
		unsigned int once_no = list[10];
		fileDisk.seekg((once_no-1)*s_block.blockSize);//ȥ����ȷ��ƫ��λ��
		for (i = 11; cnt< block_per_num&&i<list.size(); i++, cnt++){
			fileDisk.write((char*)&list[i], sizeof(unsigned int));
		}
		list.resize(11);
	}
	else if (list.size() <= (12 + 2*block_per_num + block_per_num*block_per_num)){
		//����һ��һ�μ�ӿ�
		int cnt=0,i;//д�������д���±�
		unsigned int once_no = list[10];
		unsigned int twice_no = list[11];
		fileDisk.seekg((once_no - 1)*s_block.blockSize);//ȥ����ȷ��ƫ��λ��
		for (i = 12; cnt <block_per_num ; i++,cnt++){
			fileDisk.write((char*)&list[i], sizeof(unsigned int));
		}
		//����һ�����μ�ӿ�
		vector<unsigned int> once_in_twice;//���μ�ӿ��е�һ�μ�ӿ�
		fileDisk.seekg((twice_no - 1)*s_block.blockSize);//ȥ����ȷ��ƫ��λ��
		cnt = 0;
		for (; cnt<block_per_num; i++,cnt++){
			once_in_twice.push_back(list[i]);
			fileDisk.write((char*)&list[i], sizeof(unsigned int));
		}
		//���μ�ӿ��һ�μ�ӿ�
		for (auto item : once_in_twice){
			fileDisk.seekg((item - 1)*s_block.blockSize);//ȥ����ȷ��ƫ��λ��
			cnt = 0;
			for (; cnt<block_per_num&&i<list.size(); i++, cnt++){
				once_in_twice.push_back(list[i]);
				fileDisk.write((char*)&list[i], sizeof(unsigned int));
			}
		}
		list.resize(12);
	}
	else if (list.size() <= (13 + 3*block_per_num + 2*block_per_num*block_per_num + block_per_num*block_per_num*block_per_num)){
		//����һ��һ�μ�ӿ�
		int cnt = 0, i;//д�������д���±�
		unsigned int once_no = list[10];
		unsigned int twice_no = list[11];
		unsigned int third_no = list[12];
		fileDisk.seekg((once_no - 1)*s_block.blockSize);//ȥ����ȷ��ƫ��λ��
		for (i = 13; cnt <block_per_num; i++, cnt++){
			fileDisk.write((char*)&list[i], sizeof(unsigned int));
		}
		//����һ�����μ�ӿ�
		vector<unsigned int> once_in_twice;//���μ�ӿ��е�һ�μ�ӿ�
		fileDisk.seekg((twice_no - 1)*s_block.blockSize);//ȥ����ȷ��ƫ��λ��
		cnt = 0;
		for (; cnt<block_per_num; i++, cnt++){
			once_in_twice.push_back(list[i]);
			fileDisk.write((char*)&list[i], sizeof(unsigned int));
		}
		//���μ�ӿ��һ�μ�ӿ�
		for (auto item : once_in_twice){
			fileDisk.seekg((item - 1)*s_block.blockSize);//ȥ����ȷ��ƫ��λ��
			cnt = 0;
			for (; cnt<block_per_num; i++, cnt++){
				once_in_twice.push_back(list[i]);
				fileDisk.write((char*)&list[i], sizeof(unsigned int));
			}
		}
		//����һ�����μ�ӿ�
		vector<unsigned int> twice_in_third;//���μ�ӿ��е�һ�μ�ӿ�
		fileDisk.seekg((third_no - 1)*s_block.blockSize);//ȥ����ȷ��ƫ��λ��
		cnt = 0;
		for (; cnt<block_per_num; i++, cnt++){
			twice_in_third.push_back(list[i]);
			fileDisk.write((char*)&list[i], sizeof(unsigned int));
		}
		once_in_twice.clear();
		for (auto item : twice_in_third){
			fileDisk.seekg((item - 1)*s_block.blockSize);//ȥ����ȷ��ƫ��λ��
			cnt = 0;
			for (; cnt<block_per_num; i++, cnt++){
				once_in_twice.push_back(list[i]);
				fileDisk.write((char*)&list[i], sizeof(unsigned int));
			}
		}
		for (auto item : once_in_twice){
			fileDisk.seekg((item - 1)*s_block.blockSize);//ȥ����ȷ��ƫ��λ��
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
	//��鷶Χ
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