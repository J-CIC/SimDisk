// SimDisk.cpp : �������̨Ӧ�ó������ڵ㡣
//

#include "stdafx.h"
#include "FileSystem.h"
#include <windows.h>

int _tmain(int argc, _TCHAR* argv[])
{
	// ����������
	HANDLE m_hMutex = CreateMutex(NULL, FALSE, L"simdisk");
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

