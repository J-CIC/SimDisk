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
	
}
FileSystem::~FileSystem()
{
	fileDisk.close();
}

//����
int FileSystem::serve(){
	string cmd;
	outputPrompt();
	while (getline(cin, cmd)){
		if (cmd == "exit"){
			break;
		}
		else{
			parseCmd(cmd);
		}
		outputPrompt();
	}
	return 1;
}

//��������
int FileSystem::parseCmd(string cmd)
{
	int ret = 0;
	vector<string>cmd_list;
	stringstream s(cmd);
	s >> cmd;
	string temp;
	while (s >> temp){
		cmd_list.push_back(temp);
	}
	if (cmd == "newfile"){
		if (cmd_list.size() == 1){
			ret = newfile(cmd_list[0]);
			if (ret == -1){
				cout << "file exists !" << endl;
			}
			else if (ret == -2){
				cout << "name length is not valid" << endl;
			}
			else if (ret == -3){
				cout << "No enough iNode or blocks" << endl;
			}
		}
		else{
			cout << "newfile accept one parameter" << endl;
		}
	}
	else if (cmd == "dir"){
		if (cmd_list.size() == 1){
			ret = ls(cmd_list[0]);
			if (ret == 0 || ret == 2){
				cout << "No such directory : " << cmd_list[0] << endl;
			}
		}
		else if (cmd_list.size() == 0){
			ret = ls();
		}
		else{
			cout << "dir accept less than one parameter"<<endl;
		}
	}
	else if (cmd == "cd"){
		if (cmd_list.size() >= 1){
			ret = cd(cmd_list[0]);
			if (ret == 2||ret ==0){
				cout << "No such directory : " << cmd_list[0] << endl;
			}
		}
	}
	else if (cmd == "del"){
		if (cmd_list.size() == 0){
			cout << "del accept at least one parameter" << endl;
		}
		else{
			for (auto name : cmd_list){
				int ret = del(name);
				if (ret == 0){
					cout << "file: " <<name <<" not found " << endl;
				}
				else if (ret == -1){
					cout << "file: " << name << "is not file" << endl;
				}
			}
		}
	}
	else if (cmd == "mkdir"){
		if (cmd_list.size() == 0){
			cout << "mkdir accept at least one parameter" << endl;
		}
		else{
			for (auto name : cmd_list){
				ret = mkdir(name);
				if (ret == -1){
					cout << "folder exists !" << endl;
				}
				else if (ret == -2){
					cout << "name length is not valid" << endl;
				}
				else if (ret == -3){
					cout << "No enough iNode or blocks" << endl;
				}
			}
		}
	}
	else if (cmd == "rd"){
		if (cmd_list.size() == 0){
			cout << "del accept at least one parameter" << endl;
		}
		else{
			for (auto name : cmd_list){
				ret = rd(name);
				if (ret == 3){
					//�ǿ�Ŀ¼
					cout << "the directory is not empty, force delete? " << "Y/N :" << endl;
					string choice;
					cin >> choice;
					if (choice == "Y"||choice=="y"){
						ret = rd(name, true);//ǿ��ɾ��
					}
				}
				if (ret == 0|| ret == 2){
					cout << "No such folder: " << name << endl;
				}
			}
		}
	}
	else if (cmd == "cat"){
		if (cmd_list.size() != 1){
			cout << "cat accept only one parameter" << endl;
		}
		else{
			ret = cat(cmd_list[0]);
			if (ret == 0||ret == 1){
				cout << "No such file: " << cmd_list[0] << endl;
			}
		}
	}
	else if (cmd == "info"){
		s_block.printInfo();
	}
	else if (cmd == "copy"){
		if (cmd_list.size() == 2){
			ret = copy(cmd_list[0], cmd_list[1]);
			if (ret == 0){
				cout << "No such file: " << cmd_list[0] << endl;
			}
			else if (ret == -1){
				cout << "file exists !" << endl;
			}
			else if (ret == -2){
				cout << "name length is not valid" << endl;
			}
			else if (ret == -3){
				cout << "No enough iNode or blocks" << endl;
			}
		}
		else{
			cout << "copy require two parameters"<<endl;
		}
	}
	else{
		if (cmd != "")
			cout << "unknown command" << endl;
	}
	return ret;
}

//�����ʾ��
void FileSystem::outputPrompt(){
	cout << "root:" << curr_dentry->getPathName() << "# ";
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
	if (blocks_needed > s_block.block_remain){
		//����ʣ����
		return -1;
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
	node.i_mode = 1911;//777
	if (is_dentry){
		unsigned short mode = 1;
		mode = mode << 14;
		node.i_mode = mode+1911;//�趨Ϊ�ļ���
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

//ɾ���ļ�����ջ��ļ���Ӧ�Ľڵ�Ϳ�
int FileSystem::withdraw_node(iNode node){
	vector<unsigned int> block_list;
	readBlockIds(node, block_list);
	destroy_inode(node.ino);//�޸�inodeλͼ
	s_block.inode_remain++;//�޸�ʣ��iNode��Ŀ
	for (auto idx : block_list){
		destroy_block(idx);//�޸�blockλͼ
	}
	s_block.block_remain += block_list.size();//�޸�ʣ��block��Ŀ
	seekAndSave<superBlock>(0, s_block);
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
	vector<unsigned int> blocks_list;
	iNode p_node = root_dentry.inode;
	readBlockIds(p_node, blocks_list);//��ȡ���ݿ��б�
	InitDentry(root_dentry);
	for (auto item : root_dentry.child_list){
		InitDentry(*item);
	}
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

//�����ļ����ɴӱ�����ģ������и���
int FileSystem::copy(string from, string to){
	string host_cmd = "<host>";
	if (from.compare(0, host_cmd.size(), host_cmd)==0){
		//��<host>��ͷ��ָ��
		from = from.substr(host_cmd.size());
		fstream file_from(from, ios::binary | ios::in | ios::out);
		if (!file_from){
			//�ļ�������
			return 0;
		}
		else{
			file_from.seekg(0,ios::end);
			unsigned long size = file_from.tellg();//�ļ���С��Byte
			file_from.seekg(0, ios::beg);
			if (size > s_block.maxBytes){
				file_from.close();
				return -2;//��������ļ���С
			}
			int ret = newfile(to,size);
			if (ret <0){
				file_from.close();
				return ret;//����ʧ��
			}
			iNode des_node;
			read_inode(ret,des_node);//��ȡҪ��ŵ�iNode
			vector<unsigned int> block_list; 
			readBlockIds(des_node, block_list);//��ȡд��Ŀ�
			//������ȡ���ݴ�Ŀ���������г�ʼ��
			char * content = new char[s_block.blockSize];
			memset(content, 0, s_block.blockSize);
			for (auto block_id : block_list){
				unsigned long pos = (block_id - 1)*s_block.blockSize;
				fileDisk.seekg(pos,ios::beg);
				file_from.read((char *)content, s_block.blockSize);
				int real_size = file_from.gcount();
				fileDisk.write((char *)content, real_size);
			}
			file_from.close();
		}
	}
	else{
		//��ģ��ϵͳ�и��Ƶ�ģ��ϵͳ
		dentry *fromDentry, *toDentry;
		int ret = findDentryWithName(from, fromDentry, FILE_TYPE);
		if (ret == 0){
			//δ�ҵ��ļ�
			return 0;
		}
		ret = newfile(to, fromDentry->inode.i_size);
		if (ret == -1){
			//iNode��Ŀ�����block��Ŀ����
			return ret;
		}
		findDentryWithName(to, toDentry, FILE_TYPE);
		//��ȡ��д��block
		vector<unsigned int> read_list, write_list;
		readBlockIds(fromDentry->inode, read_list);
		readBlockIds(toDentry->inode, write_list);
		//��ʼ�����ݿ�
		char * content = new char[s_block.blockSize];
		memset(content, 0, s_block.blockSize);
		//����ļ���С
		unsigned long size = fromDentry->inode.i_size;
		int read_size = 0;
		for (int i = 0; i < write_list.size(); i++){
			//����һ�ζ�ȡ�Ĵ�С
			if (size > s_block.blockSize){
				read_size = s_block.blockSize;
				size -= s_block.blockSize;
			}
			else{
				read_size = size;
				size = 0;
			}
			unsigned long read_pos = (read_list[i] - 1)*s_block.blockSize;
			unsigned long write_pos = (write_list[i] - 1)*s_block.blockSize;
			fileDisk.seekg(read_pos, ios::beg);
			fileDisk.read((char *)content, read_size);
			fileDisk.seekg(write_pos, ios::beg);
			fileDisk.write((char *)content, read_size);
		}
	}
	return 1;
}

//�����ļ���,����ֵΪiNode��id
int FileSystem::newfile(string filename,unsigned long size)
{
	vector<string> dir_list;
	SplitString(filename, dir_list, "/");
	dentry *temp_dentry;//�ݴ�ı���
	string file_name = "";
	//��ȡ�������ļ�����
	file_name = dir_list[dir_list.size() - 1];
	if (file_name.length() <= 0){
		return -2;//�ļ������Ȳ��Ϸ�
	}
	int ret = findDentry(dir_list, temp_dentry, filename[0],FILE_TYPE);//�ж��Ƿ����
	if (ret == 2){
		//�����ļ�
		return -1;
	}
	//ȥ���ļ������ֺ��Ŀ¼�ַ���
	dir_list.resize(dir_list.size() - 1);
	ret = findDentry(dir_list, temp_dentry, filename[0]);//Ѱ�Ҹ��ļ���
	if (ret == 0){
		return 0;
	}
	iNode file_node;
	ret = alloc_inode(size, file_node);
	if (ret == -1){
		return -3;
	}
	dir s_dir(file_name, file_node.ino);//����file
	dentry *created_dentry = new dentry(s_dir.dir_name, file_node);//����dentry��
	created_dentry->setParent(*temp_dentry);
	temp_dentry->addChild(created_dentry);//���븸Ŀ¼������
	SaveDentry(*temp_dentry);
	return file_node.ino;
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
	if (folder_name.length() <= 0){
		return -2;//�ļ������Ȳ��Ϸ�
	}
	int ret = findDentry(dir_list, temp_dentry, filename[0]);//�ж��Ƿ����
	if (ret == 1){
		//�����ļ���
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
	if (ret == -1){//����ʧ��
		return -3;
	}
	dir s_dir(folder_name, dir_node.ino);//����dir
	dentry *created_dentry = new dentry(s_dir.dir_name, dir_node);//����dentry��
	created_dentry->setParent(*temp_dentry);
	temp_dentry->addChild(created_dentry);//���븸Ŀ¼������
	SaveDentry(*temp_dentry);
	return 1;
}

//�л�Ŀ¼
int FileSystem::cd(string filename){
	dentry *temp;
	int ret = findDentryWithName(filename, temp);
	if (ret == 1){
		curr_dentry = temp;
	}
	return ret;
}

//ɾ��Ŀ¼,����1��ʾ�ɹ�,����2��ʾĿ¼��Ϊ��,��Ҫȷ��
int FileSystem::rd(string filename,bool force)
{
	vector<string> dir_list;
	SplitString(filename, dir_list, "/");
	dentry *temp_dentry;//�ݴ�ı���
	string folder_name = "";
	//��ȡ�������ļ�������
	folder_name = dir_list[dir_list.size() - 1];
	int ret = findDentry(dir_list, temp_dentry, filename[0]);//�ж��Ƿ����
	if (ret == 0){
		//�������ļ�
		return 0;
	}
	else if (ret == 2){
		//���ļ�����
		return 2;
	}
	else if (ret == 1){
		//�ļ��и�ʽ
		if (temp_dentry->inode.i_size == 0){
			//��Ŀ¼
			dentry *p_dentry = temp_dentry->parent;
			withdraw_node(temp_dentry->inode);//�ջ�iNode�ڵ�
			p_dentry->removeChild(temp_dentry);//�Ƴ��ڴ��ڵ���
			SaveDentry(*(temp_dentry->parent));//���游Ŀ¼����Ϣ�޸�
		}
		else{
			if (force){
				//�ǿ���ǿ��ɾ��
				vector<dentry*> temp = temp_dentry->child_list;//�м仺�棬��Ϊɾ���������޸�temp_dentry������
				for (auto item : temp){
					if (item->is_dir()){
						//ǿ��ɾ����Ŀ¼
						rd(item->getPathName(), force);
					}
					else{
						//ɾ���ļ�
						del(item->getPathName());
					}
				}
				dentry *p_dentry = temp_dentry->parent;
				withdraw_node(temp_dentry->inode);//�ջ�iNode�ڵ�
				p_dentry->removeChild(temp_dentry);//�Ƴ��ڴ��ڵ���
				SaveDentry(*(temp_dentry->parent));//���游Ŀ¼����Ϣ�޸�
				return 1;
			} 
			return 3;
		}
	}
	return 1;
}

//ɾ���ļ�
int FileSystem::del(string filename){
	vector<string> dir_list;
	SplitString(filename, dir_list, "/");
	dentry *temp_dentry;//�ݴ�ı���
	string folder_name = "";
	//��ȡ�������ļ�������
	folder_name = dir_list[dir_list.size() - 1];
	int ret = findDentry(dir_list, temp_dentry, filename[0],FILE_TYPE);//�ж��Ƿ����
	if (ret == 0){
		//�������ļ�
		return 0;
	}
	else if (ret == 2){
		//���ļ�����
		withdraw_node(temp_dentry->inode);//�ջ�iNode�ڵ�
		temp_dentry->parent->removeChild(temp_dentry);//�Ƴ��ڴ��ڵ���
		SaveDentry(*temp_dentry->parent);//���游Ŀ¼����Ϣ�޸�
	}
	else if (ret == 1){
		//�ļ��и�ʽ
		return -1;
	}
	return 1;
}

//��ȡ�ļ�
int FileSystem::cat(string filename){
	dentry *temp;
	int ret = findDentryWithName(filename, temp, FILE_TYPE);
	if (ret == 0||ret==1){
		return ret;
	}
	readBlockIds(temp->inode, temp->block_list);//��ȡblock
	char *content = new char[s_block.blockSize];
	memset(content, 0, s_block.blockSize);
	unsigned long size = temp->inode.i_size;
	int read_size = 0;
	for (auto block_id : temp->block_list){
		if (size > s_block.blockSize){
			read_size = s_block.blockSize;
			size -= s_block.blockSize;
		}
		else{
			read_size = size;
			size = 0;
		}
		unsigned long pos = (block_id - 1)*s_block.blockSize;
		fileDisk.seekg(pos, ios::beg);
		fileDisk.read((char *)content, read_size);
		int real_size = fileDisk.gcount();
		string t(content, content + read_size);
		cout << t;
	}
	return FILE_TYPE;
}

//չʾĿ¼
int FileSystem::ls(string filename){
	dentry *temp;
	if (filename == ""){
		curr_dentry->showDentry(this->getUsers());
	}
	else{
		int ret = findDentryWithName(filename, temp);
		if (ret == 0 || ret == 2){
			return ret;
		}
		temp->showDentry(this->getUsers());
	}
	return 1;
}

//��ȡ�û��б�
vector<string> FileSystem::getUsers()
{
	vector<string> users;
	users.push_back("");//��0λ����
	users.push_back("root");
	return users;
}

//ͨ������Ѱ��Ŀ¼��,����1��ʾ�ҵ��ļ��У�����2��ʾ�ҵ��ļ���0��ʾδ�ҵ���typeΪ1���ļ��У�typeΪ2���ļ�
int FileSystem::findDentryWithName(string name, dentry *&p_dentry, int type)
{
	int ret = 0;
	vector<string> dir_list;
	SplitString(name, dir_list, "/");
	ret = findDentry(dir_list, p_dentry, name[0], type);
	return ret;
}


//ͨ��·��vectorѰ��Ŀ¼��,����1��ʾ�ҵ��ļ��У�����2��ʾ�ҵ��ļ���0��ʾδ�ҵ���typeΪ1���ļ��У�typeΪ2���ļ�
int FileSystem::findDentry(vector<string> list,dentry *&p_dentry,char firstChar,int type)
{
	int ret = 0;
	p_dentry = curr_dentry;
	if (list.size() == 0){
		return 1;//ֱ�ӵ�ǰĿ¼�´���
	}
	if (firstChar == '/'){
		p_dentry = &root_dentry;//�Ӹ�Ŀ¼��ʼ
		list.erase(list.begin());//����һ���ַ���/��split���һλΪ��
	}
	for (auto item : list){
		ret = 0;
		if (item == ".."){//����Ŀ¼
			p_dentry = p_dentry->parent;
			ret = 1;
		}
		else if (item == "."){//��ǰĿ¼
			ret = 1;
		}
		else{//����Ѱ��Ŀ¼
			for (auto child_dentry : p_dentry->child_list){
				if (child_dentry->fileName == item){
					//������ֶ����ˣ���Ҫ�ж��ļ�����
					if (child_dentry->is_dir()){
						p_dentry = child_dentry;
						if (p_dentry->child_list.size() == 0){
							InitDentry(*p_dentry);//��ʼ��������Ŀ¼
						}
						for (auto item : p_dentry->child_list){
							InitDentry(*item);
						}
						//������·���ϵ����һ������������ļ�����
						if (type == FOLDER_TYPE || item != list[list.size() - 1]){
							ret = FOLDER_TYPE;//�ҵ����ļ���
							break;
						}
					}
					else{
						p_dentry = child_dentry;
						//����·���ϵ����һ�������ǲ���Ѱ�ҵ��ļ�
						if (type == FILE_TYPE && item == list[list.size() - 1]){
							ret = FILE_TYPE;//�ҵ����ļ�
							break;
						}
					}
				}
			}
		}
		if (ret == 0){
			return ret;
		}
	}
	return ret;
}

//��ʼ��dentry
int FileSystem::InitDentry(dentry & p_dentry){
	if (p_dentry.child_list.size() == 0){
		readBlockIds(p_dentry.inode, p_dentry.block_list);//����block_list
		int dir_num_per_block = s_block.blockSize / sizeof(dir);//ÿһ�����ܴ�ŵ�dir��Ŀ
		int count = 0, max_dir = floor(p_dentry.inode.i_size / sizeof(dir));
		dir t_dir;
		for (auto b_idx : p_dentry.block_list){
			if (count == max_dir){
				break;
			}
			int base_pos = (b_idx - 1)*s_block.blockSize;//����ƫ�Ƶ�ַ
			for (int i = 0; i < dir_num_per_block&&count<max_dir; i++){
				seekAndGet<dir>(base_pos + i*sizeof(dir), t_dir);
				if (t_dir.ino>0 && t_dir.ino <= s_block.inode_num){
					dentry* t_dentry = new dentry();
					read_inode(t_dir.ino, t_dentry->inode);//��ȡiNode
					t_dentry->fileName = t_dir.dir_name;
					t_dentry->setParent(p_dentry);
					p_dentry.addChild(t_dentry);
					count++;
					if (count == max_dir){
						break;
					}
				}
			}
		}
	}
	return 1;
}

//����dentry
int FileSystem::SaveDentry(dentry & p_dentry){
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