// shell.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <windows.h>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
#include <conio.h>  
using namespace std;
#define INPUT_SIZE 4096


int parse_cmd(string cmd);//解析命令
void transfer_cmd(string i_cmd,string cmd);//发送命令
int login(bool last_wrong = false);//登录
string auth_token;
string cmd_result;
HANDLE m_command;           //客户端通知服务器
HANDLE m_return;           //服务器通知客户端
HANDLE p_mutex;           //用于同步客户端的 mutex

int _tmain(int argc, _TCHAR* argv[])
{

	//创建进程
	STARTUPINFO si;
	PROCESS_INFORMATION pi;
	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	ZeroMemory(&pi, sizeof(pi));
	// Start the child process. 
	if (!CreateProcess(
		L"F:\\Git\\github\\SimDisk\\shell\\Debug\\SimDisk.exe",
		argv[1],        // Command line
		NULL,           // Process handle not inheritable
		NULL,           // Thread handle not inheritable
		FALSE,          // Set handle inheritance to FALSE
		//CREATE_NEW_CONSOLE,              // No creation flags
		CREATE_NO_WINDOW,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi)           // Pointer to PROCESS_INFORMATION structure
		)
	{
		cout << "CreateProcess failed" << GetLastError() << endl;
		return 0;
	}
	auth_token = "";//初始化token
	//设定事件初始化，若没创建事件则创建
	m_command = OpenEvent(EVENT_ALL_ACCESS, NULL, L"shell_input");
	if (m_command == NULL){
		m_command = CreateEvent(NULL, FALSE, FALSE, L"shell_input");
	}
	m_return = OpenEvent(EVENT_ALL_ACCESS, NULL, L"shell_return");
	if (m_return == NULL){
		m_return = CreateEvent(NULL, FALSE, FALSE, L"shell_return");
	}
	int login_result = login();//先执行登录
	while (login_result != 1){
		login_result = login(true);//失败重新登录
	}
	system("cls");//成功后清屏
	cout << cmd_result;//补充输出prompt
	//输入命令
	string cmd;
	while (getline(cin, cmd)){
		// 创建互斥量
		HANDLE m_hMutex = CreateMutex(NULL, FALSE, L"my_shell");
		// 检查错误代码  
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			// 已有互斥量存在
		}
		WaitForSingleObject(m_hMutex, INFINITE);//无限等待
		int ret = parse_cmd(cmd);
		if (ret == -1){
			transfer_cmd("", "");//获得prompt
		}
		ReleaseMutex(m_hMutex); //释放互斥锁  
	}
	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return 0;
}

//登录
int login(bool last_wrong){
	string usr_name, usr_pwd;
	system("cls");
	if (last_wrong){
		//上次登录错误
		cout << "Login Failed!" << endl;
	}
	cout << "Input username:" << endl;
	getline(cin, usr_name);//输入用户名
	cout << "Input password:" << endl;
	char character;
	usr_pwd = "";
	while ((character = _getch())!=13){
		if (character == 8){
			//处理退格
			if (usr_pwd.size() > 0){
				cout << "\b \b";
				usr_pwd.resize(usr_pwd.size() - 1);
			}
		}
		else if (character >= 32 && character <= 126){
			usr_pwd += character;
			cout << "*";
			//未考虑方向键等，因为双字节难处理
		}
	}
	string cmd = "auth " + usr_name + " " + usr_pwd;
	parse_cmd(cmd);
	if (auth_token == ""){
		return 0;
	}
	return 1;
}

//传输命令
void transfer_cmd(string initial_cmd,string cmd){
	// 打开共享的文件对象
	HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, NULL, L"ShareMemory");//消息传递共享内存
	HANDLE usrMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, NULL, L"usrShareMemory");//用户token共享内存
	if (hMapFile&&usrMapFile)
	{
		LPVOID lpBase = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, INPUT_SIZE);//映射文件
		LPVOID lpUsr = MapViewOfFile(usrMapFile, FILE_MAP_ALL_ACCESS, 0, 0, INPUT_SIZE);//映射文件
		// 复制对应命令 
		strcpy_s((char*)lpBase, INPUT_SIZE, initial_cmd.c_str());
		strcpy_s((char*)lpUsr, INPUT_SIZE, auth_token.c_str());
		SetEvent(m_command);//通知服务器
		WaitForSingleObject(m_return, INFINITE);//服务器处理完毕通知客户端
		char return_val[INPUT_SIZE] = { 0 };
		strcpy_s(return_val, (char*)lpBase);// 将共享内存数据拷贝到字符串
		cmd_result = return_val;//保留输出
		if (cmd == "auth"){
			//如果是auth命令，更新token
			strcpy_s(return_val, (char*)lpUsr);// 将共享内存数据拷贝到字符串
			auth_token = return_val;
		}
		else{
			cout << cmd_result;
		}
		ResetEvent(m_return);//重置事件
		// 解除文件映射
		UnmapViewOfFile(lpBase);
		// 关闭内存映射文件对象句柄
		CloseHandle(hMapFile);
	}
	else
	{
		// 打开共享内存句柄失败
		printf("OpenMapping Error");
	}
}

//解析命令
int parse_cmd(string cmd){
	string initial_cmd = cmd;//最初的指令
	vector<string>cmd_list;//参数列表
	stringstream s(cmd);
	string total = cmd;
	s >> cmd;
	string temp;
	while (s >> temp){
		cmd_list.push_back(temp);
	}
	if (cmd == "newfile"){
		if (cmd_list.size() == 1){

		}
		else{
			cout << "newfile accept one parameter" << endl;
			return -1;
		}
	}
	else if (cmd == "auth"){
		if (cmd_list.size() == 2){

		}
		else{
			cout << "wrong usage" << endl;
			return -1;
		}
	}
	else if (cmd == "dir"){
		if (cmd_list.size() <= 1){

		}
		else{
			cout << "dir accept less than one parameter" << endl;
			return -1;
		}
	}
	else if (cmd == "cd"){
		if (cmd_list.size() == 1){

		}
		else{
			cout << "cd only accept one parameter" << endl;
			return -1;
		}
	}
	else if (cmd == "del"){
		if (cmd_list.size() != 1){
			cout << "del accept only one parameter" << endl;
			return -1;
		}
		else{

		}
	}
	else if (cmd == "md"){
		if (cmd_list.size() != 1){
			cout << "mkdir accept only one parameter" << endl;
			return -1;
		}
		else{

		}
	}
	else if (cmd == "rd"){
		if (cmd_list.size() != 1){
			cout << "del accept only one parameter" << endl;
			return -1;
		}
		else{

		}
	}
	else if (cmd == "cat"){
		if (cmd_list.size() != 1){
			cout << "cat accept only one parameter" << endl;
			return -1;
		}
		else{

		}
	}
	else if (cmd == "info"){
		if (cmd_list.size() > 0){
			cout << "info command doesn't accept any parameter" << endl;
			return -1;
		}
	}
	else if (cmd == "copy"){
		if (cmd_list.size() == 2){

		}
		else{
			cout << "copy require two parameters" << endl;
			return -1;
		}
	}
	else if (cmd == "chmod"){
		if (cmd_list.size() != 2){
			cout << "chmod only accept 2 parameter" << endl;
			return -1;
		}
	}
	else if (cmd == "cls"){
		system("cls");
	}
	else if (cmd == "exit"){
		exit(0);//退出
	}
	else{
		if (cmd != "")
		{
			cout << "unknown command" << endl;
		}
		return -1;
	}
	transfer_cmd(initial_cmd,cmd);
	return 0;
}
