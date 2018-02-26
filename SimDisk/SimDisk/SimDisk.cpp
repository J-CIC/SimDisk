// SimDisk.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include "FileSystem.h"
#include <windows.h>

int _tmain(int argc, _TCHAR* argv[])
{
	// 创建互斥量
	HANDLE m_hMutex = CreateMutex(NULL, FALSE, L"simdisk");
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

