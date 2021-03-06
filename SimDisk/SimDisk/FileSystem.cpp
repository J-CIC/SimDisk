#include "stdafx.h"
#include "FileSystem.h"

const string FileSystem::fileName = "disk.bin";
#define INPUT_SIZE 4096

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
		fileDisk.seekg(diskSize, ios::beg);//偏移磁盘大小
		fileDisk.write("1",1);//写入内容使得文件达到目标大小
		fileDisk.close();
		fileDisk.open(fileName, ios::binary | ios::in | ios::out);
		int ret = alloc_inode(272384, root, true);//设置根节点
		root.i_size = 0;
		unsigned short t_mode = 1;
		t_mode = t_mode << 14;
		root.i_mode = t_mode + (7 << 8) + (7 << 4) + 7;//设定为文件夹777权限
		seekAndSave<iNode>(s_block.inode_table, root);
	}
	seekAndGet<superBlock>(0, s_block);
	seekAndGet<iNode>(s_block.inode_table, root);
	init_root_dentry();//初始化目录
	init_user();//初始化用户
}
FileSystem::~FileSystem()
{
	fileDisk.close();
}


//初始化用户
int FileSystem::init_user()
{
	//初始化root用户
	dentry * temp;
	if (findDentryWithName("/etc/shadow", temp, 2) != 2){
		//shadow文件不存在
		User rootUser = User("root", "root", 0);//默认账户
		currUser = rootUser;
		userLists.push_back(rootUser);//加入用户列表
		User testUser = User("test", "test", userLists[userLists.size()-1].u_id+1);//测试账户
		userLists.push_back(testUser);//加入用户列表
		mkdir("/etc");
		int id = newfile("/etc/shadow", 10240);
		save_user();
		return 1;
	}
	findDentryWithName("/etc/shadow", temp, 2);
	vector<unsigned int> block_lists;//内容块
	readBlockIds(temp->inode, block_lists);//读取内容块
	int cnt_per_block = s_block.blockSize / sizeof(User);
	int block_idx = 0, cnt = 0;
	int user_count = temp->inode.i_size / sizeof(User);
	for (int i = 0; i < user_count;i++){
		if (cnt >= cnt_per_block){
			cnt = 0;
			block_idx += 1;
		}
		unsigned long base_pos = (block_lists[block_idx] - 1) * s_block.blockSize;
		User *user = new User();
		seekAndGet<User>(base_pos + cnt*sizeof(User), *user);
		userLists.push_back(*user);//推入列表中
		cnt++;
	}
	return 1;
}

//保存用户
int FileSystem::save_user()
{
	dentry * temp;
	if (findDentryWithName("/etc/shadow", temp, 2) != 2){
		mkdir("/etc");
		int id = newfile("/etc/shadow", 10240);
	}
	temp->inode.i_mode = 7 << 8;
	temp->inode.i_size = userLists.size()*sizeof(User);
	vector<unsigned int> block_lists;//内容块
	readBlockIds(temp->inode, block_lists);//读取内容块
	int cnt_per_block = s_block.blockSize / sizeof(User);
	int block_idx = 0, cnt = 0;
	for (auto user : userLists){
		if (cnt >= cnt_per_block){
			cnt = 0;
			block_idx += 1;
		}
		unsigned long base_pos = (block_lists[block_idx] - 1) * s_block.blockSize;
		seekAndSave<User>(base_pos + cnt*sizeof(User), user);
		cnt++;
	}
	write_inode(temp->inode);//保存iNode
	return 1;
}

//服务
int FileSystem::serve(){
	srand(time(NULL));//置随机数种子用于生成token
	HANDLE m_command;           //客户端通知服务器
	HANDLE m_return;           //服务器通知客户端
	HANDLE p_mutex;           //用于同步客户端的 mutex

	//设定事件初始化，若没创建事件则创建
	m_command = OpenEvent(EVENT_ALL_ACCESS, NULL, L"shell_input");
	if (m_command == NULL){
		m_command = CreateEvent(NULL, FALSE, FALSE, L"shell_input");
	}
	m_return = OpenEvent(EVENT_ALL_ACCESS, NULL, L"shell_return");
	if (m_return == NULL){
		m_return = CreateEvent(NULL, FALSE, FALSE, L"shell_return");
	}
	// 创建共享文件句柄 
	hMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,   // 物理文件句柄
		NULL,   // 默认安全级别
		PAGE_READWRITE,   // 可读可写
		0,   // 高位文件大小
		INPUT_SIZE,   // 低位文件大小
		L"ShareMemory"   // 共享内存名称
		);
	usrMapFile = CreateFileMapping(
		INVALID_HANDLE_VALUE,   // 物理文件句柄
		NULL,   // 默认安全级别
		PAGE_READWRITE,   // 可读可写
		0,   // 高位文件大小
		INPUT_SIZE,   // 低位文件大小
		L"usrShareMemory"   // 共享内存名称
		);
	for (;;){
		DWORD dw = WaitForSingleObject(m_command, INFINITE);//无限等待
		// 映射缓存区视图 , 得到指向共享内存的指针
		LPVOID lpBase = MapViewOfFile(
			hMapFile,            // 共享内存的句柄
			FILE_MAP_ALL_ACCESS, // 可读写许可
			0,
			0,
			INPUT_SIZE
			);
		if (dw == WAIT_TIMEOUT){
			break;
		}
		// 将共享内存数据拷贝到字符串
		char cmd_inital[INPUT_SIZE] = { 0 };
		strcpy_s(cmd_inital, (char*)lpBase);
		string cmd = cmd_inital;
		stringstream redirect_stream;//设定cout的重定向留
		streambuf * backup;//备份
		cout.rdbuf(redirect_stream.rdbuf());//重定向
		parseCmd(cmd);//处理命令
		outputPrompt();//输出提示符
		string output;//输出
		output = redirect_stream.str();//复制到输出
		strcpy_s((char*)lpBase, INPUT_SIZE, output.c_str());//写入共享内存
		ResetEvent(m_command);//重置事件
		SetEvent(m_return);//通知客户端处理
		UnmapViewOfFile(lpBase);// 解除文件映射
	}

	
	// 关闭内存映射文件对象句柄
	CloseHandle(hMapFile);
	CloseHandle(usrMapFile);


	return 1;
}

//检查权限，需要的权限access_type用rwx的整数表示
bool FileSystem::checkAccess(int access_type, iNode node){
	bool ret_val = false;
	if (currUser.u_id == 0){
		//管理员
		return true;
	}
	if (currUser.u_id == node.i_uid){
		//属于当前用户的权限
		int access = node.i_mode;//所拥有的权限
		int mask = (1 << 2) + (1 << 1) + 1;
		mask = mask << 8;//获取本用户权限的掩码
		ret_val = (access_type << 8)&(mask&access);
	}
	else if (currUser.g_id == node.i_gid){
		//属于当前用户组的权限
		int access = node.i_mode;//所拥有的权限
		int mask = (1 << 2) + (1 << 1) + 1;
		mask = mask << 4;//获取本用户权限的掩码
		ret_val = (access_type << 4)&(mask&access);
	}
	else{
		//其他人的权限
		int access = node.i_mode;//所拥有的权限
		int mask = (1 << 2) + (1 << 1) + 1;//获取本用户权限的掩码
		ret_val = access_type&(mask&access);
	}
	return ret_val;
}

//解析命令
int FileSystem::parseCmd(string cmd)
{
	int ret = 0;
	vector<string>cmd_list;
	stringstream s(cmd);
	string total = cmd;
	s >> cmd;
	string temp;
	while (s >> temp){
		cmd_list.push_back(temp);
	}
	//如果用户执行登录操作，则进行单独判断
	if (cmd == "auth"){
		int u_id = auth(cmd_list[0], cmd_list[1]);
		if (u_id == -1){
			generate_token(-1);//生成失败空token，并保存至内存中
			return ret;
		}
		else{
			generate_token(u_id);//生成校验token，并保存至内存中
			ret = 1;
		}
	}
	else{
		//获取本次的shell_user
		if (get_shell_user() == -1){
			cout << "invalid user" << endl;
			return ret;
		}
		//处理命令
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
				cout << "dir accept less than one parameter" << endl;
			}
		}
		else if (cmd == "cd"){
			if (cmd_list.size() >= 1){
				ret = cd(cmd_list[0]);
				if (ret == 2 || ret == 0){
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
						cout << "file: " << name << " not found " << endl;
					}
					else if (ret == -1){
						cout << "file: " << name << " is not file" << endl;
					}
				}
			}
		}
		else if (cmd == "md"){
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
						//非空目录
						cout << "the directory is not empty, force delete? " << "Y/N :" << endl;
						string choice;
						getline(cin, choice);
						if (choice == "Y" || choice == "y"){
							ret = rd(name, true);//强制删除
						}
					}
					if (ret == 0 || ret == 2){
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
				if (ret == 0 || ret == 1){
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
				cout << "copy require two parameters" << endl;
			}
		}
		else if (cmd=="chmod"){
			stringstream temp(cmd_list[1]);
			int mode;
			temp >> mode;
			ret = chmod(cmd_list[0], mode);
			if (ret == 0){
				cout << "file not exists" << endl;
			}
			else if (ret == -1){
				cout << "invalid mode" << endl;
			}
		}
		else if (cmd == "exit"){
			exit(0);//退出
		}
		else{
			if (cmd != "")
				cout << "unknown command" << endl;
		}
	}
	return ret;
}

//生成登录的token
int FileSystem::generate_token(int uid){
	string token = "";//最终保存的token
	if (uid != -1){
		//登录成功才产生token，否则token为空
		unsigned long c_time = time(NULL);
		stringstream buf;//生成流用以转换类型
		buf << c_time;//流中输入信息
		string time_string;
		buf >> time_string;//输出到字符
		for (int i = 0; i < time_string.length(); i++){
			int rand_num = rand() % 26;//26个字母
			char character = rand_num + 65 + (rand() % 2) * 32;//随机大小写字母
			token += character;
			token += time_string[i];//插值
		}
		loginUserLists.insert(pair<string, int>(token, uid));
		if (uid < userLists.size()){
			currUser = userLists[uid];
		}
	}
	// 映射缓存区视图 , 得到指向共享内存的指针
	LPVOID lpBase = MapViewOfFile(
		usrMapFile,            // 共享内存的句柄
		FILE_MAP_ALL_ACCESS, // 可读写许可
		0,
		0,
		INPUT_SIZE
		);
	strcpy_s((char*)lpBase, INPUT_SIZE, token.c_str());//写入共享内存
	return 1;
}

//获取本次命令的User
int FileSystem::get_shell_user(){
	char token[INPUT_SIZE] = { 0 };
	// 映射缓存区视图 , 得到指向共享内存的指针
	LPVOID lpBase = MapViewOfFile(
		usrMapFile,            // 共享内存的句柄
		FILE_MAP_ALL_ACCESS, // 可读写许可
		0,
		0,
		INPUT_SIZE
		);
	strcpy_s(token, (char*)lpBase);//写入共享内存
	string key = token;//token复制
	int u_id = -1;//用户id
	auto item = loginUserLists.find(key);//寻找对应id
	if (item != loginUserLists.end()){
		//找到了登录用户
		u_id = item->second;
		if (u_id < userLists.size()){
			currUser = userLists[u_id];//设定当前用户
		}
		else{
			return -1;//异常状况，按道理不可能出现，以防万一
		}
	}
	return u_id;
}

//输出提示符
void FileSystem::outputPrompt(){
	cout << currUser.username << ":" << curr_dentry->getPathName() << "# ";
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
	if (blocks_needed > s_block.block_remain){
		//超过剩余量
		return -1;
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
					fileDisk.flush();//写回文件
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
	node.i_mode = (6<<8)+(6<<4)+4;//664的默认权限
	if (is_dentry){
		unsigned short mode = 1;
		mode = mode << 14;
		node.i_mode = mode + (7 << 8) + (5 << 4) + 5;//设定为文件夹755的默认权限
	}
	node.i_uid = currUser.u_id;
	node.i_gid = currUser.u_id;
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
	fileDisk.flush();//写回文件
	return 1;
}

//删除文件后的收回文件对应的节点和块
int FileSystem::withdraw_node(iNode node){
	vector<unsigned int> block_list;
	readBlockIds(node, block_list);
	destroy_inode(node.ino);//修改inode位图
	s_block.inode_remain++;//修改剩余iNode数目
	for (auto idx : block_list){
		destroy_block(idx);//修改block位图
	}
	s_block.block_remain += block_list.size();//修改剩余block数目
	seekAndSave<superBlock>(0, s_block);
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
	fileDisk.flush();//写回文件
	vector<unsigned int> list;//清空块内容
	list.push_back(id);//清空块内容
	clearBlockContent(list);//清空块内容
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
	fileDisk.flush();//写回文件
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
		fileDisk.flush();//写回文件
	}
	return 1;
}


//初始化根dentry
int FileSystem::init_root_dentry()
{
	root_dentry = dentry();
	root_dentry.inode = root;
	root_dentry.fileName = "/";
	root_dentry.setParent(&root_dentry);
	curr_dentry = &root_dentry;
	vector<unsigned int> blocks_list;
	iNode p_node = root_dentry.inode;
	readBlockIds(p_node, blocks_list);//读取内容块列表
	InitDentry(root_dentry);
	for (auto item : root_dentry.child_list){
		InitDentry(*item);
	}
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

//修改文件权限，mode为10进制的数字
int FileSystem::chmod(string name, int mode){
	dentry * temp_dentry;
	int ret = findDentryWithName(name, temp_dentry);
	if (ret <= 0){
		return ret;
	}
	//检查权限
	if (!checkAccess(WRITE_ACCESS, temp_dentry->inode)){
		cout << "Access Deny" << endl;
		return ACCESS_DENY;
	}
	if (mode <= 0 || mode > 1000){
		return -1;//mode无效
	}
	int idx_3 = (mode % 1000) / 100;
	int idx_2 = (mode % 100) / 10;
	int idx_1 = mode % 10;
	if (idx_1 > 7 || idx_2 > 7 || idx_3 > 7){
		return -1;//mode无效
	}
	if (temp_dentry->is_dir()){
		unsigned short t_mode = 1;
		t_mode = t_mode << 14;
		temp_dentry->inode.i_mode = t_mode + (idx_3 << 8) + (idx_2 << 4) + idx_1;//设定为文件夹权限
	}
	else{
		temp_dentry->inode.i_mode = (idx_3 << 8) + (idx_2 << 4) + idx_1;//设定为文件权限
	}
	return 1;
}

//登录认证操作，登录失败返回-1，否则返回对应用户的内存下标
int FileSystem::auth(string username, string pwd)
{
	if (username.length() > 16 || pwd.length() > 32){
		//当用户名或密码长度超过最大限制的时候，应该在shell处限制
	}
	char usr_name[17] = { 0 };
	char usr_pwd[33] = { 0 };
	strcpy_s(usr_name, 16, username.c_str());//复制用户名
	strcpy_s(usr_pwd, 16, pwd.c_str());//复制密码
	for (int i = 0; i < userLists.size(); i++){
		User user = userLists[i];
		if (user.auth(usr_name, usr_pwd)){
			return i;
		}
	}
	return -1;
}

//复制文件，可从本机或模拟磁盘中复制
int FileSystem::copy(string from, string to){
	string host_cmd = "<host>";
	if (from.compare(0, host_cmd.size(), host_cmd)==0){
		//以<host>开头的指令
		from = from.substr(host_cmd.size());
		fstream file_from(from, ios::binary | ios::in | ios::out);
		if (!file_from){
			//文件不存在
			return 0;
		}
		else{
			file_from.seekg(0,ios::end);
			unsigned long size = file_from.tellg();//文件大小，Byte
			file_from.seekg(0, ios::beg);
			if (size > s_block.maxBytes){
				file_from.close();
				return -2;//超过最大文件大小
			}
			int ret = newfile(to,size);
			if (ret <0){
				file_from.close();
				return ret;//申请失败
			}
			iNode des_node;
			read_inode(ret,des_node);//读取要存放的iNode
			vector<unsigned int> block_list; 
			readBlockIds(des_node, block_list);//获取写入的块
			//声明读取和暂存的块变量并进行初始化
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
	else if (to.compare(0, host_cmd.size(), host_cmd) == 0){
		//从模拟系统中复制到本机
		to = to.substr(host_cmd.size());
		fstream file_to(to, ios::binary | ios::out);
		dentry *fromDentry;
		int ret = findDentryWithName(from, fromDentry, FILE_TYPE);
		if (ret == 0){
			//未找到文件
			return 0;
		}
		//权限检查
		if (!checkAccess(READ_ACCESS, fromDentry->inode)){
			//无权限
			cout << "Access Deny" << endl;
			return ACCESS_DENY;
		}
		//获取读取的内容
		vector<unsigned int> read_list;
		readBlockIds(fromDentry->inode, read_list);
		//初始化内容块
		char * content = new char[s_block.blockSize];
		memset(content, 0, s_block.blockSize);
		//获得文件大小
		unsigned long size = fromDentry->inode.i_size;
		int read_size = 0;
		for (int i = 0; i < read_list.size(); i++){
			//计算一次读取的大小
			if (size > s_block.blockSize){
				read_size = s_block.blockSize;
				size -= s_block.blockSize;
			}
			else{
				read_size = size;
				size = 0;
			}
			unsigned long read_pos = (read_list[i] - 1)*s_block.blockSize;
			fileDisk.seekg(read_pos, ios::beg);
			fileDisk.read((char *)content, read_size);
			file_to.write((char *)content, read_size);
		}
		file_to.close();
	}else{
		//从模拟系统中复制到模拟系统
		dentry *fromDentry, *toDentry;
		int ret = findDentryWithName(from, fromDentry, FILE_TYPE);
		if (ret == 0){
			//未找到文件
			return 0;
		}
		//判断是否有读权限
		if (!checkAccess(READ_ACCESS, fromDentry->inode)){
			cout << "Access Deny" << endl;
			return ACCESS_DENY;
		}
		ret = newfile(to, fromDentry->inode.i_size);
		if (ret < 0){
			//iNode数目不足或block数目不足
			return ret;
		}
		findDentryWithName(to, toDentry, FILE_TYPE);
		//获取读写的block
		vector<unsigned int> read_list, write_list;
		readBlockIds(fromDentry->inode, read_list);
		readBlockIds(toDentry->inode, write_list);
		//初始化内容块
		char * content = new char[s_block.blockSize];
		memset(content, 0, s_block.blockSize);
		//获得文件大小
		unsigned long size = fromDentry->inode.i_size;
		int read_size = 0;
		for (int i = 0; i < write_list.size(); i++){
			//计算一次读取的大小
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

//创建文件夹,返回值为iNode的id
int FileSystem::newfile(string filename,unsigned long size)
{
	vector<string> dir_list;
	SplitString(filename, dir_list, "/");
	dentry *temp_dentry;//暂存的变量
	string file_name = "";
	//获取创建的文件名字
	file_name = dir_list[dir_list.size() - 1];
	if (file_name.length() <= 0){
		return -2;//文件名长度不合法
	}
	int ret = findDentry(dir_list, temp_dentry, filename[0],FILE_TYPE);//用于判断是否存在
	//去掉文件夹名字后的目录字符串
	dir_list.resize(dir_list.size() - 1);
	int ret2 = findDentry(dir_list, temp_dentry, filename[0]);//用于寻找父文件夹
	if (ret2 == 0){
		return 0;
	}
	//判断是否有写权限
	if (!checkAccess(WRITE_ACCESS, temp_dentry->inode)){
		cout << "Access Deny" << endl;
		return ACCESS_DENY;
	}
	if (ret == FILE_TYPE || ret == FOLDER_TYPE){
		//存在文件或文件夹同名
		return -1;
	}
	iNode file_node;
	ret = alloc_inode(size, file_node);
	if (ret == -1){
		return -3;
	}
	dir s_dir(file_name, file_node.ino);//生成file
	dentry *created_dentry = new dentry(s_dir.dir_name, file_node);//生成dentry项
	created_dentry->setParent(temp_dentry);
	temp_dentry->addChild(created_dentry);//加入父目录的子项
	SaveDentry(*temp_dentry);
	return file_node.ino;
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
	if (folder_name.length() <= 0){
		return -2;//文件名长度不合法
	}
	int ret = findDentry(dir_list, temp_dentry, filename[0]);//用于判断是否存在
	//去掉文件夹名字后的目录字符串
	dir_list.resize(dir_list.size() - 1);
	int ret2 = findDentry(dir_list, temp_dentry,filename[0]);//用于寻找父文件夹
	if (ret2 == 0){
		return 0;
	}
	//判断是否有写权限
	if (!checkAccess(WRITE_ACCESS, temp_dentry->inode)){
		cout << "Access Deny" << endl;
		return ACCESS_DENY;
	}
	if (ret == FILE_TYPE || ret == FOLDER_TYPE){
		//存在文件夹或文件同名
		return -1;
	}
	iNode dir_node;
	ret = alloc_inode(0, dir_node, true);
	if (ret == -1){//申请失败
		return -3;
	}
	dir s_dir(folder_name, dir_node.ino);//生成dir
	dentry *created_dentry = new dentry(s_dir.dir_name, dir_node);//生成dentry项
	created_dentry->setParent(temp_dentry);
	temp_dentry->addChild(created_dentry);//加入父目录的子项
	SaveDentry(*temp_dentry);
	return 1;
}

//切换目录
int FileSystem::cd(string filename){
	dentry *temp;
	int ret = findDentryWithName(filename, temp);
	//判断是否有执行权限
	if (!checkAccess(EXEC_ACCESS, temp->inode)){
		//无权限
		cout << "Access Deny" << endl;
		return ACCESS_DENY;
	}
	if (ret == 1){
		curr_dentry = temp;
	}
	return ret;
}

//删除目录,返回1表示成功,返回2表示目录不为空,需要确认
int FileSystem::rd(string filename,bool force)
{
	vector<string> dir_list;
	SplitString(filename, dir_list, "/");
	dentry *temp_dentry;//暂存的变量
	string folder_name = "";
	//获取创建的文件夹名字
	folder_name = dir_list[dir_list.size() - 1];
	int ret = findDentry(dir_list, temp_dentry, filename[0]);//判断是否存在
	if (ret == 0){
		//不存在文件
		return 0;
	}
	else if (ret == 2){
		//是文件类型
		return 2;
	}
	else if (ret == 1){
		//判断是否有写权限
		if (!checkAccess(WRITE_ACCESS, temp_dentry->inode)){
			//无权限
			cout << "Access Deny" << endl;
			return ACCESS_DENY;
		}
		//文件夹格式
		if (temp_dentry->inode.i_size == 0){
			//空目录
			dentry *p_dentry = temp_dentry->parent;
			withdraw_node(temp_dentry->inode);//收回iNode节点
			p_dentry->removeChild(temp_dentry);//移除内存内的项
			SaveDentry(*(temp_dentry->parent));//保存父目录的信息修改
			delete temp_dentry;
		}
		else{
			if (force){
				//非空且强制删除
				vector<dentry*> temp = temp_dentry->child_list;//中间缓存，因为删除操作会修改temp_dentry的内容
				for (auto item : temp){
					if (item->is_dir()){
						//强制删除子目录
						rd(item->getPathName(), force);
					}
					else{
						//删除文件
						del(item->getPathName());
					}
				}
				dentry *p_dentry = temp_dentry->parent;
				withdraw_node(temp_dentry->inode);//收回iNode节点
				p_dentry->removeChild(temp_dentry);//移除内存内的项
				SaveDentry(*(temp_dentry->parent));//保存父目录的信息修改
				return 1;
			} 
			return 3;
		}
	}
	return 1;
}

//删除文件
int FileSystem::del(string filename){
	vector<string> dir_list;
	SplitString(filename, dir_list, "/");
	dentry *temp_dentry;//暂存的变量
	string folder_name = "";
	//获取创建的文件夹名字
	folder_name = dir_list[dir_list.size() - 1];
	int ret = findDentry(dir_list, temp_dentry, filename[0],FILE_TYPE);//判断是否存在
	if (ret == 0){
		//不存在文件
		return 0;
	}
	else if (ret == 2){
		//是文件类型
		//判断是否有写权限
		if (!checkAccess(WRITE_ACCESS, temp_dentry->inode)){
			//无权限
			cout << "Access Deny" << endl;
			return ACCESS_DENY;
		}
		withdraw_node(temp_dentry->inode);//收回iNode节点
		temp_dentry->parent->removeChild(temp_dentry);//移除内存内的项
		SaveDentry(*temp_dentry->parent);//保存父目录的信息修改
		delete temp_dentry;
	}
	else if (ret == 1){
		//文件夹格式
		return -1;
	}
	return 1;
}

//读取文件
int FileSystem::cat(string filename){
	dentry *temp;
	int ret = findDentryWithName(filename, temp, FILE_TYPE);
	if (ret == 0||ret==1){
		return ret;
	}
	//判断是否有读权限
	if (!checkAccess(READ_ACCESS, temp->inode)){
		//无权限
		cout << "Access Deny" << endl;
		return ACCESS_DENY;
	}
	readBlockIds(temp->inode, temp->block_list);//读取block
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

//展示目录
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

//获取用户列表
vector<string> FileSystem::getUsers()
{
	vector<string> users;
	for (auto user : userLists){
		users.push_back(user.username);
	}
	return users;
}

//通过名字寻找目录项,返回1表示找到文件夹，返回2表示找到文件，0表示未找到，type为1找文件夹，type为2找文件
int FileSystem::findDentryWithName(string name, dentry *&p_dentry, int type)
{
	int ret = 0;
	vector<string> dir_list;
	SplitString(name, dir_list, "/");
	ret = findDentry(dir_list, p_dentry, name[0], type);
	return ret;
}


//通过路径vector寻找目录项,返回1表示找到文件夹，返回2表示找到文件，0表示未找到，type为1找文件夹，type为2找文件
int FileSystem::findDentry(vector<string> list,dentry *&p_dentry,char firstChar,int type)
{
	int ret = 0;
	p_dentry = curr_dentry;
	if (firstChar == '/'){
		p_dentry = &root_dentry;//从根目录开始
		list.erase(list.begin());//若第一个字符是/则split后第一位为空
	}
	if (list.size() == 0){
		return 1;//直接当前目录下创建
	}
	for (auto item : list){
		ret = 0;
		if (item == ".."){//父层目录
			p_dentry = p_dentry->parent;
			ret = 1;
		}
		else if (item == "."){//当前目录
			ret = 1;
		}
		else{//遍历寻找目录
			for (auto child_dentry : p_dentry->child_list){
				if (child_dentry->fileName == item){
					//如果名字对上了，还要判断文件类型
					if (child_dentry->is_dir()){
						p_dentry = child_dentry;
						if (p_dentry->child_list.size() == 0){
							InitDentry(*p_dentry);//初始化自身及子目录
						}
						for (auto item : p_dentry->child_list){
							InitDentry(*item);
						}
						ret = FOLDER_TYPE;//找到了文件夹 
						if (item == list[list.size() - 1]){
							break;//找到了字符串路径末尾，直接退出
						}
					}
					else{
						p_dentry = child_dentry;
						ret = FILE_TYPE;//找到了文件夹 
						//若是路径上的最后一个，则看是不是寻找的文件
						if (item == list[list.size() - 1]){
							break;//找到了字符串路径末尾，直接退出
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

//初始化dentry
int FileSystem::InitDentry(dentry & p_dentry){
	if (p_dentry.child_list.size() == 0){
		readBlockIds(p_dentry.inode, p_dentry.block_list);//保存block_list
		int dir_num_per_block = s_block.blockSize / sizeof(dir);//每一个块能存放的dir数目
		int count = 0, max_dir = floor(p_dentry.inode.i_size / sizeof(dir));
		dir t_dir;
		for (auto b_idx : p_dentry.block_list){
			if (count == max_dir){
				break;
			}
			int base_pos = (b_idx - 1)*s_block.blockSize;//基础偏移地址
			for (int i = 0; i < dir_num_per_block&&count<max_dir; i++){
				seekAndGet<dir>(base_pos + i*sizeof(dir), t_dir);
				if (t_dir.ino>0 && t_dir.ino <= s_block.inode_num){
					dentry* t_dentry = new dentry();
					read_inode(t_dir.ino, t_dentry->inode);//读取iNode
					t_dentry->fileName = t_dir.dir_name;
					t_dentry->setParent(&p_dentry);
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

//保存dentry
int FileSystem::SaveDentry(dentry & p_dentry){
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