// 檔案: helpers.h
#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

// ========== 共用輔助函式宣告 ==========
HMODULE GetRemoteModuleHandle(DWORD dwProcessId, const wchar_t* szModuleName);
void WriteCommandToMemory(const std::vector<BYTE>& commandBytes);
void OverwriteStringInMemory(LPCWSTR targetAddress, const std::wstring& newString);
DWORD ReadSapid();
HWND GetMainWindowForProcess(DWORD dwProcessId);
HWND GetCurrentProcessMainWindow();
HWND ReadChildWindowHandle(); // 【新增】宣告讀取子視窗句柄的函式