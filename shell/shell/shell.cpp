// shell.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include <windows.h>
#include <string>
#include <iostream>
#include <vector>
#include <sstream>
using namespace std;
#define INPUT_SIZE 4096


void parse_cmd(string cmd);//��������
void transfer_cmd(string i_cmd,string cmd);//��������
string auth_token;
HANDLE m_command;           //�ͻ���֪ͨ������
HANDLE m_return;           //������֪ͨ�ͻ���
HANDLE p_mutex;           //����ͬ���ͻ��˵� mutex

int _tmain(int argc, _TCHAR* argv[])
{

	//��������
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
	auth_token = "";//��ʼ��token
	//�趨�¼���ʼ������û�����¼��򴴽�
	m_command = OpenEvent(EVENT_ALL_ACCESS, NULL, L"shell_input");
	if (m_command == NULL){
		m_command = CreateEvent(NULL, FALSE, FALSE, L"shell_input");
	}
	m_return = OpenEvent(EVENT_ALL_ACCESS, NULL, L"shell_return");
	if (m_return == NULL){
		m_return = CreateEvent(NULL, FALSE, FALSE, L"shell_return");
	}
	//��������
	string cmd;
	while (getline(cin, cmd)){
		// ����������
		HANDLE m_hMutex = CreateMutex(NULL, FALSE, L"my_shell");
		// ���������  
		if (GetLastError() == ERROR_ALREADY_EXISTS) {
			// ���л���������
		}
		WaitForSingleObject(m_hMutex, INFINITE);//���޵ȴ�
		parse_cmd(cmd);
		ReleaseMutex(m_hMutex); //�ͷŻ�����  
	}
	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return 0;
}

//��������
void transfer_cmd(string initial_cmd,string cmd){
	// �򿪹�����ļ�����
	HANDLE hMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, NULL, L"ShareMemory");//��Ϣ���ݹ����ڴ�
	HANDLE usrMapFile = OpenFileMapping(FILE_MAP_ALL_ACCESS, NULL, L"usrShareMemory");//�û�token�����ڴ�
	if (hMapFile&&usrMapFile)
	{
		LPVOID lpBase = MapViewOfFile(hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, INPUT_SIZE);//ӳ���ļ�
		LPVOID lpUsr = MapViewOfFile(usrMapFile, FILE_MAP_ALL_ACCESS, 0, 0, INPUT_SIZE);//ӳ���ļ�
		// ���ƶ�Ӧ���� 
		strcpy_s((char*)lpBase, INPUT_SIZE, initial_cmd.c_str());
		strcpy_s((char*)lpUsr, INPUT_SIZE, auth_token.c_str());
		SetEvent(m_command);//֪ͨ������
		WaitForSingleObject(m_return, INFINITE);//�������������֪ͨ�ͻ���
		char return_val[INPUT_SIZE] = { 0 };
		strcpy_s(return_val, (char*)lpBase);// �������ڴ����ݿ������ַ���
		cout << return_val;
		if (cmd == "auth"){
			//�����auth�������token
			strcpy_s(return_val, (char*)lpBase);// �������ڴ����ݿ������ַ���
			auth_token = return_val;
			cout << auth_token << endl;
		}
		ResetEvent(m_return);//�����¼�
		// ����ļ�ӳ��
		UnmapViewOfFile(lpBase);
		// �ر��ڴ�ӳ���ļ�������
		CloseHandle(hMapFile);
	}
	else
	{
		// �򿪹����ڴ���ʧ��
		printf("OpenMapping Error");
	}
}


void parse_cmd(string cmd){
	string initial_cmd = cmd;//�����ָ��
	vector<string>cmd_list;//�����б�
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
		exit(0);//�˳�
	}
	else{
		if (cmd != "")
		{
			cout << "unknown command" << endl;
		}
		return;
	}
	transfer_cmd(initial_cmd,cmd);
}
