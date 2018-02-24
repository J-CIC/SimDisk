// SimDisk.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "FileSystem.h"
#include <windows.h>

int _tmain(int argc, _TCHAR* argv[])
{
	string mutex = "simdisk";
	size_t size = mutex.length();
	wchar_t *buffer = new wchar_t[size + 1];
	MultiByteToWideChar(CP_ACP, 0, mutex.c_str(), size, buffer, size * sizeof(wchar_t));
	buffer[size] = 0;  //ȷ���� '\0' ��β 
	// ����������
	HANDLE m_hMutex = CreateMutex(NULL, FALSE, buffer);
	// ���������  
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		// ������л������������ͷž������λ������  
		CloseHandle(m_hMutex);
		m_hMutex = NULL;
		// �����˳�  
		return FALSE;
	}
	FileSystem fs;
	fs.serve();
	return 0;
}

