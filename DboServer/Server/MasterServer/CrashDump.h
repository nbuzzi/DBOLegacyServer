#pragma once
// CrashDump.h
#include <windows.h>
#include <DbgHelp.h>
#pragma comment(lib, "Dbghelp.lib")

inline LONG WINAPI UnhandledExceptionFilter_MiniDump(EXCEPTION_POINTERS* pException)
{
    SYSTEMTIME ti; GetLocalTime(&ti);
    char path[MAX_PATH];
    sprintf_s(path, ".\\dumps\\master_%04d-%02d-%02d_%02d-%02d-%02d.dmp",
        ti.wYear, ti.wMonth, ti.wDay, ti.wHour, ti.wMinute, ti.wSecond);

    HANDLE hFile = CreateFileA(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (hFile == INVALID_HANDLE_VALUE) return EXCEPTION_EXECUTE_HANDLER;

    MINIDUMP_EXCEPTION_INFORMATION mei{ GetCurrentThreadId(), pException, FALSE };
    MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile,
        (MINIDUMP_TYPE)(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory),
        &mei, nullptr, nullptr);

    CloseHandle(hFile);
    return EXCEPTION_EXECUTE_HANDLER;
}
