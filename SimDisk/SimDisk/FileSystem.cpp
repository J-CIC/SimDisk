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

//����iNode�ڵ�,size��λΪByte
int FileSystem::alloc_inode(unsigned long size, iNode &node,bool is_dentry)
{
	unsigned int blocks_needed = ceil((double)size / s_block.blockSize);//��Ҫ�洢���ݵĿ���
	//����С�������ƣ�iNode�ڵ㲻��ʱ
	if (size>s_block.maxBytes || s_block.inode_remain == 0){
		return -1;
	}
	if (size == 0){
		blocks_needed = 10;
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
	clearBlockContent(list);//�������Ŀ��е�����
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

int FileSystem::write_inode(iNode &node){
	//��鷶Χ
	if (node.ino<1 || node.ino>s_block.inode_num){
		return -1;
	}
	node.i_mtime = time(NULL);//�޸�ʱ��
	node.i_atime = node.i_mtime;//����ʱ��
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


//�ջ�block�����޸�λͼ
int FileSystem::destroy_block(int id)
{
	unsigned int byte_pos = floor(id / 8);//ƫ���ֽ�
	int byte_idx = id % 8;//�ֽ��ڵ�ַ�±�
	if (byte_idx == 0){
		//���������һλ
		byte_pos--;
		byte_idx = 8;
	}
	char byte=0;//������ݵ�λͼ
	unsigned char mask = 255;
	unsigned char xor_mask = 1;
	xor_mask = xor_mask << (8 - byte_idx);
	mask = mask^xor_mask;
	//��ȡ����0
	fileDisk.seekg(s_block.bitmap_pos + byte_pos);
	fileDisk.read((char *)&byte, 1);
	byte = byte & mask;
	//д��
	fileDisk.seekg(s_block.bitmap_pos + byte_pos);
	fileDisk.write((char *)&byte, 1);
	return 1;
}

//�ջ�iNode
int FileSystem::destroy_inode(int id)
{
	unsigned int byte_pos = floor(id / 8);//ƫ���ֽ�
	int byte_idx = id % 8;//�ֽ��ڵ�ַ�±�
	if (byte_idx == 0){
		//���������һλ
		byte_pos--;
		byte_idx = 8;
	}
	char byte = 0;//������ݵ�λͼ
	unsigned char mask = 255;
	unsigned char xor_mask = 1;
	xor_mask = xor_mask << (8 - byte_idx);
	mask = mask^xor_mask;
	//��ȡ����0
	fileDisk.seekg(s_block.inodemap_pos + byte_pos);
	fileDisk.read((char *)&byte, 1);
	byte = byte & mask;
	//д��
	fileDisk.seekg(s_block.inodemap_pos + byte_pos);
	fileDisk.write((char *)&byte, 1);
	return 1;
}

//����������е�����
int FileSystem::clearBlockContent(vector<unsigned int> list)
{
	//���ɿ����ݲ�д��
	char bytes[1024];
	memset(bytes, 0, 1024);
	for (auto item : list)
	{
		fileDisk.seekg((item-1)*s_block.blockSize, ios::beg);
		fileDisk.write(bytes, 1024);
	}
	return 1;
}


//��ʼ����dentry
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

//��ȡ��Ŀ¼
int FileSystem::getSubDentry(dentry& p_dir)
{
	vector<unsigned int> blocks_list;
	iNode p_node = p_dir.inode;
	readBlockIds(p_node, blocks_list);//��ȡ���ݿ��б�
	InitDentry(p_dir);
	file p_file = file(p_node,blocks_list);
	return 1;
}

//��ȡ��ӦiNode�����ݿ飬��������ӿ�
int FileSystem::readBlockIds(iNode inode, vector<unsigned int> &blocks_list)
{
	unsigned int block_num = inode.i_blocks;
	//ʮ����ӿ�
	for (int i = 0; i < 10; i++){
		if (inode.i_zone[i] == 0){
			break;
		}
		blocks_list.push_back(inode.i_zone[i]);
	}
	//һ�μ�ӿ�
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
	//���μ�ӿ�
	if (inode.i_zone[11] != 0){
		int pos = (inode.i_zone[11] - 1)*s_block.blockSize;
		fileDisk.seekg(pos, ios::beg);
		int loop_times = ceil(s_block.blockSize / sizeof(unsigned int));//����ÿ���ܷ��µ���Ŀ
		vector<unsigned int> once_list;
		for (int i = 0; i < loop_times; i++){
			unsigned int blcoks_num = 0;
			fileDisk.read((char*)&blcoks_num, sizeof(unsigned int));
			if (blcoks_num>0){
				once_list.push_back(blcoks_num);//�����м仺���б�
			}
		}
		for (auto item : once_list){
			int pos = (item - 1)*s_block.blockSize;
			fileDisk.seekg(pos, ios::beg);
			for (int i = 0; i < loop_times; i++){
				unsigned int blcoks_num = 0;
				fileDisk.read((char*)&blcoks_num, sizeof(unsigned int));//����ÿ���ܷ��µ���Ŀ
				if (blcoks_num>0){
					blocks_list.push_back(blcoks_num);//�������յĿ��
				}
			}
		}
	}
	//���μ�ӿ�
	if (inode.i_zone[12] != 0){
		int pos = (inode.i_zone[11] - 1)*s_block.blockSize;
		fileDisk.seekg(pos, ios::beg);
		int loop_times = ceil(s_block.blockSize / sizeof(unsigned int));//����ÿ���ܷ��µ���Ŀ
		vector<unsigned int> twice_list, once_list;
		for (int i = 0; i < loop_times; i++){
			unsigned int blcoks_num = 0;
			fileDisk.read((char*)&blcoks_num, sizeof(unsigned int));
			if (blcoks_num>0){
				twice_list.push_back(blcoks_num);//��������м仺���б�
			}
		}
		for (auto item : twice_list){
			int pos = (item - 1)*s_block.blockSize;
			fileDisk.seekg(pos, ios::beg);
			for (int i = 0; i < loop_times; i++){
				unsigned int blcoks_num = 0;
				fileDisk.read((char*)&blcoks_num, sizeof(unsigned int));
				if (blcoks_num>0){
					once_list.push_back(blcoks_num);//����һ�λ����
				}
			}
		}
		for (auto item : once_list){
			int pos = (item - 1)*s_block.blockSize;
			fileDisk.seekg(pos, ios::beg);
			for (int i = 0; i < loop_times; i++){
				unsigned int blcoks_num = 0;
				fileDisk.read((char*)&blcoks_num, sizeof(unsigned int));//����ÿ���ܷ��µ���Ŀ
				if (blcoks_num>0){
					blocks_list.push_back(blcoks_num);//�������յĿ��
				}
			}
		}
	}
	return 1;
}

//����Ŀ¼
int FileSystem::mkdir(string filename)
{
	vector<string> dir_list;
	SplitString(filename,dir_list,"/");
	dentry *temp_dentry;//�ݴ�ı���
	string folder_name = "";
	//��ȡ�������ļ�������
	folder_name = dir_list[dir_list.size() - 1];
	int ret = findDentry(dir_list, temp_dentry, filename[0]);//�ж��Ƿ����
	if (ret == 1){
		//����
		return -1;
	}
	//ȥ���ļ������ֺ��Ŀ¼�ַ���
	dir_list.resize(dir_list.size() - 1);
	ret = findDentry(dir_list, temp_dentry,filename[0]);//Ѱ�Ҹ��ļ���
	if (ret == 0){
		return 0;
	}
	iNode dir_node;
	ret = alloc_inode(0, dir_node, true);
	if (ret == -1){
		return ret;
	}
	dir s_dir(folder_name, dir_node.ino);//����dir
	dentry created_dentry(s_dir.dir_name, dir_node);//����dentry��
	created_dentry.setParent(*temp_dentry);
	temp_dentry->addChild(created_dentry);//���븸Ŀ¼������
	SaveDentry(*temp_dentry);
	


	if (folder_name.length() <= 0){
		return -1;//�ļ������Ȳ��Ϸ�
	}
	return 1;
}

//�趨����Ŀ¼
int FileSystem::setCurrDir(vector<string> list)
{

	return 1;
}

//Ѱ��Ŀ¼��
int FileSystem::findDentry(vector<string> list,dentry *&p_dentry,char firstChar)
{
	p_dentry = curr_dentry;
	if (list.size() == 0){
		return 1;//ֱ�ӵ�ǰĿ¼�´���
	}
	if (firstChar == '/'){
		p_dentry = &root_dentry;//�Ӹ�Ŀ¼��ʼ
		list.erase(list.begin());//���ڸ�Ŀ¼��ʼɾ���׸��յ�λ��
	}
	for (auto item : list){
		if (item == ".."){//����Ŀ¼
			p_dentry = p_dentry->parent;
		}
		else if (item == "."){//��ǰĿ¼
			p_dentry = p_dentry;
		}
		else{//����Ѱ��Ŀ¼
			if (p_dentry->child_list.size() == 0){
				//��Ŀ¼��ĿΪ0����������δ��ȡĿ¼
				InitDentry(*p_dentry);
			}
			for (auto child_dentry : p_dentry->child_list){
				if (child_dentry.fileName == item){
					//������ֶ����ˣ���Ҫ�ж��ļ�����
					if (child_dentry.is_dir()){
						p_dentry = &child_dentry;
						return 1;
					}
				}
			}
		}
	}
	return 0;//û�ҵ�
}

//��ʼ��dentry
int FileSystem::InitDentry(dentry & p_dentry){
	readBlockIds(p_dentry.inode, p_dentry.block_list);//����block_list
	int dir_num_per_block = s_block.blockSize / sizeof(dir);//ÿһ�����ܴ�ŵ�dir��Ŀ
	int count = 0,max_dir = floor(p_dentry.inode.i_size / sizeof(dir));
	dir t_dir;
	for (auto b_idx : p_dentry.block_list){
		if (count == max_dir){
			break;
		}
		int base_pos = (b_idx - 1)*s_block.blockSize;//����ƫ�Ƶ�ַ
		for (int i = 0; i < dir_num_per_block&&count<max_dir; i++){
			seekAndGet<dir>(base_pos + i*sizeof(dir), t_dir);
			if (t_dir.ino>0 && t_dir.ino <= s_block.inode_num){
				dentry t_dentry;
				read_inode(t_dir.ino, t_dentry.inode);//��ȡiNode
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

//����dentry
int FileSystem::SaveDentry(dentry & p_dentry){
	p_dentry.inode.i_size = p_dentry.child_list.size()*sizeof(dir);//����ռ�ô�С
	write_inode(p_dentry.inode);//����iNode
	int dir_num_per_block = s_block.blockSize / sizeof(dir);//ÿһ�����ܴ�ŵ�dir��Ŀ
	vector<dir> save_list = p_dentry.getDirList();
	int count = 0, max_dir = save_list.size();
	for (auto b_idx : p_dentry.block_list){
		if (count == max_dir){
			break;
		}
		int base_pos = (b_idx - 1)*s_block.blockSize;//����ƫ�Ƶ�ַ
		for (int i = 0; i < dir_num_per_block && count<max_dir; i++){
			seekAndSave<dir>(base_pos + i*sizeof(dir), save_list[count]);
			count++;
		}
	}
	return 1;
}

//��ȡiNode�ڵ�
int FileSystem::read_inode(int ino, iNode &node){
	unsigned long pos = s_block.inode_table + (ino-1)*sizeof(iNode);
	seekAndGet<iNode>(pos, node);
	return 1;
}