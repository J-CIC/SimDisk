// shell.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <windows.h>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
using namespace std;
#define INPUT_SIZE 4096


void parse_cmd(string cmd);

int _tmain(int argc, _TCHAR* argv[])
{

	HANDLE m_command;           //客户端通知服务器
	HANDLE m_return;           //服务器通知客户端
	HANDLE p_mutex;           //用于同步客户端的 mutex
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
		CREATE_NEW_CONSOLE,              // No creation flags
		//CREATE_NO_WINDOW,              // No creation flags
		NULL,           // Use parent's environment block
		NULL,           // Use parent's starting directory 
		&si,            // Pointer to STARTUPINFO structure
		&pi)           // Pointer to PROCESS_INFORMATION structure
		)
	{
		cout << "CreateProcess failed" << GetLastError() << endl;
		return 0;
	}
	string cmd;
	//设定事件初始化，若没创建事件则创建
	m_command = OpenEvent(EVENT_ALL_ACCESS, NULL, L"shell_input");
	if (m_command == NULL){
		m_command = CreateEvent(NULL, FALSE, FALSE, L"shell_input");
	}
	while (getline(cin, cmd)){
		// 创建互斥量
		HANDLE m_hMutex = CreateMutex(NULL, FALSE, L"my_shell");
		// 检查错误代码  
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			// 已有互斥量存在
		}
		WaitForSingleObject(m_hMutex, INFINITE);//无限等待
		parse_cmd(cmd);
		SetEvent(m_command);//通知服务器


		ReleaseMutex(m_hMutex); //释放互斥锁  
	}
	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return 0;
}


void parse_cmd(string cmd){
	string initial_cmd = cmd;//最初的指令
	vector<string>cmd_list;
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
			return;
		}
	}
	else if (cmd == "dir"){
		if (cmd_list.size() <= 1){

		}
		else{
			cout << "dir accept less than one parameter" << endl;
			return;
		}
	}
	else if (cmd == "cd"){
		if (cmd_list.size() == 1){

		}
		else{
			cout << "cd only accept one parameter" << endl;
			return;
		}
	}
	else if (cmd == "del"){
		if (cmd_list.size() != 1){
			cout << "del accept only one parameter" << endl;
			return;
		}
		else{

		}
	}
	else if (cmd == "md"){
		if (cmd_list.size() != 1){
			cout << "mkdir accept only one parameter" << endl;
			return;
		}
		else{

		}
	}
	else if (cmd == "rd"){
		if (cmd_list.size() != 1){
			cout << "del accept only one parameter" << endl;
			return;
		}
		else{

		}
	}
	else if (cmd == "cat"){
		if (cmd_list.size() != 1){
			cout << "cat accept only one parameter" << endl;
			return;
		}
		else{

		}
	}
	else if (cmd == "info"){
		if (cmd_list.size() > 0){
			cout << "info command doesn't accept any parameter" << endl;
			return;
		}
	}
	else if (cmd == "copy"){
		if (cmd_list.size() == 2){

		}
		else{
			cout << "copy require two parameters" << endl;
			return;
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
		return;
	}
	// 打开共享的文件对象
	HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, NULL, L"ShareMemory");
	if (hMapFile)
	{
		LPVOID lpBase = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, INPUT_SIZE);
		// 复制对应命令 
		strcpy_s((char*)lpBase, INPUT_SIZE,initial_cmd.c_str() );

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
