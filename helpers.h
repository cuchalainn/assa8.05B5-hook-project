// 檔案: helpers.h
#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

// ========== 共用輔助函式宣告 ==========

// 根據行程ID和模組名稱，獲取遠端行程的模組句柄
HMODULE GetRemoteModuleHandle(DWORD dwProcessId, const wchar_t* szModuleName);

// 將指定的位元組序列寫入 Assa 的命令記憶體區域
void WriteCommandToMemory(const std::vector<BYTE>& commandBytes);

// 在記憶體中覆寫一個 (寬字元) 字串
void OverwriteStringInMemory(LPCWSTR targetAddress, const std::wstring& newString);

// 從 Assa 記憶體中讀取目標石器(SA)的 PID
DWORD ReadSapid();

// 根據行程ID獲取其主視窗句柄
HWND GetMainWindowForProcess(DWORD dwProcessId);

// 獲取當前行程 (Assa) 的主視窗句柄
HWND GetCurrentProcessMainWindow();

// 從 Assa 記憶體中讀取子視窗的句柄
HWND ReadChildWindowHandle();