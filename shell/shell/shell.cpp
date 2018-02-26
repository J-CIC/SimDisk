// shell.cpp : �������̨Ӧ�ó������ڵ㡣
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


int parse_cmd(string cmd);//��������
void transfer_cmd(string i_cmd,string cmd);//��������
int login(bool last_wrong = false);//��¼
string auth_token;
string cmd_result;
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
	int login_result = login();//��ִ�е�¼
	while (login_result != 1){
		login_result = login(true);//ʧ�����µ�¼
	}
	system("cls");//�ɹ�������
	cout << cmd_result;//�������prompt
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
		int ret = parse_cmd(cmd);
		if (ret == -1){
			transfer_cmd("", "");//���prompt
		}
		ReleaseMutex(m_hMutex); //�ͷŻ�����  
	}
	// Close process and thread handles. 
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);
	return 0;
}

//��¼
int login(bool last_wrong){
	string usr_name, usr_pwd;
	system("cls");
	if (last_wrong){
		//�ϴε�¼����
		cout << "Login Failed!" << endl;
	}
	cout << "Input username:" << endl;
	getline(cin, usr_name);//�����û���
	cout << "Input password:" << endl;
	char character;
	usr_pwd = "";
	while ((character = _getch())!=13){
		if (character == 8){
			//�����˸�
			if (usr_pwd.size() > 0){
				cout << "\b \b";
				usr_pwd.resize(usr_pwd.size() - 1);
			}
		}
		else if (character >= 32 && character <= 126){
			usr_pwd += character;
			cout << "*";
			//δ���Ƿ�����ȣ���Ϊ˫�ֽ��Ѵ���
		}
	}
	string cmd = "auth " + usr_name + " " + usr_pwd;
	parse_cmd(cmd);
	if (auth_token == ""){
		return 0;
	}
	return 1;
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
		cmd_result = return_val;//�������
		if (cmd == "auth"){
			//�����auth�������token
			strcpy_s(return_val, (char*)lpUsr);// �������ڴ����ݿ������ַ���
			auth_token = return_val;
		}
		else{
			cout << cmd_result;
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

//��������
int parse_cmd(string cmd){
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
		exit(0);//�˳�
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
