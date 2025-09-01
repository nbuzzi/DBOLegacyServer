//***********************************************************************************
//
//	File		:	NtlDebug.cpp
//
//	Begin		:	2005-12-06
//
//	Copyright	:	�� NTL-Inc Co., Ltd
//
//	Author		:	Hyun Woo, Koo   ( zeroera@ntl-inc.com )
//
//	Desc		:	����� ���� ��ƿ��Ƽ
//
//***********************************************************************************

#include "Stdafx.h"
#include "NtlDebug.h"
#include "NtlMutex.h"


#ifdef __NTL_DEBUG_PRINT__

#include <stdarg.h>
#include <string.h>
#include <tchar.h>


//-----------------------------------------------------------------------------------
// static variable
//-----------------------------------------------------------------------------------
const unsigned int PRINT_BUF_SIZE	= 2048;
unsigned int s_dwCurFlag				= 0xFFFFFFFF;
FILE * s_curStream					= stderr;
//-----------------------------------------------------------------------------------


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void NtlSetPrintStream(FILE * fp)
{
	s_curStream = fp;
}


//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void NtlSetPrintFlag(unsigned int dwFlag)
{
	s_dwCurFlag = dwFlag;
}

//-----------------------------------------------------------------------------------
//		Purpose	:
//		Return	:
//-----------------------------------------------------------------------------------
void NtlDebugPrint(unsigned int dwFlag, LPCTSTR lpszText, ...)
{
	if (dwFlag & s_dwCurFlag)
	{
		// Windows console color setup
		HANDLE hConsole = GetStdHandle(STD_ERROR_HANDLE);
		CONSOLE_SCREEN_BUFFER_INFO consoleInfo;
		WORD saved_attributes = 0;
		if (hConsole != INVALID_HANDLE_VALUE && GetConsoleScreenBufferInfo(hConsole, &consoleInfo))
			saved_attributes = consoleInfo.wAttributes;

		WORD color = saved_attributes;
		// Define your severity flags (customize as needed)
		if (dwFlag == 0x01) // PRINT_SYSTEM
			color = (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // Default console color
		else if (dwFlag == 0x02) // PRINT_WARNING
			color = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
		else if (dwFlag == 0x04) // PRINT_ERROR
			color = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
		else if (dwFlag == 0x08) // PRINT_APP
			color = FOREGROUND_GREEN | FOREGROUND_INTENSITY;

		if (hConsole != INVALID_HANDLE_VALUE)
			SetConsoleTextAttribute(hConsole, color);

		TCHAR szLogBuffer[PRINT_BUF_SIZE + 1] = { 0x00, };
		int nBuffSize = sizeof(szLogBuffer);
		int nWriteSize = 0;

		SYSTEMTIME systemTime;
		GetLocalTime(&systemTime);
		nWriteSize += _stprintf_s(szLogBuffer + nWriteSize, nBuffSize - nWriteSize, TEXT("[%d-%02d-%02d %d:%d:%d:%d] "), systemTime.wYear, systemTime.wMonth, systemTime.wDay, systemTime.wHour, systemTime.wMinute, systemTime.wSecond, systemTime.wMilliseconds);

		va_list args;
		va_start(args, lpszText);
		nWriteSize += _vstprintf_s(szLogBuffer + nWriteSize, nBuffSize - nWriteSize, lpszText, args);
		va_end(args);

		fprintf(stderr, "%s\n", szLogBuffer);
		fflush(stderr);

		if (hConsole != INVALID_HANDLE_VALUE)
			SetConsoleTextAttribute(hConsole, saved_attributes);

		if (s_curStream && s_curStream != stderr)
		{
			fprintf(s_curStream, "%s\n", szLogBuffer);
			fflush(s_curStream);
		}
	}
}


#endif // __NTL_DEBUG_PRINT__
