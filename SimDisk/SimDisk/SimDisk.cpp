// SimDisk.cpp : 定义控制台应用程序的入口点。
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
	buffer[size] = 0;  //确保以 '\0' 结尾 
	// 创建互斥量
	HANDLE m_hMutex = CreateMutex(NULL, FALSE, buffer);
	// 检查错误代码  
	if (GetLastError() == ERROR_ALREADY_EXISTS) {
		// 如果已有互斥量存在则释放句柄并复位互斥量  
		CloseHandle(m_hMutex);
		m_hMutex = NULL;
		// 程序退出  
		return FALSE;
	}
	FileSystem fs;
	fs.serve();
	return 0;
}

