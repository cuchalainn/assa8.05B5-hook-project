// �ɮ�: helpers.h
#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

// ========== �@�λ��U�禡�ŧi ==========
HMODULE GetRemoteModuleHandle(DWORD dwProcessId, const wchar_t* szModuleName);
void WriteCommandToMemory(const std::vector<BYTE>& commandBytes);
void OverwriteStringInMemory(LPCWSTR targetAddress, const std::wstring& newString);
DWORD ReadSapid();
HWND GetMainWindowForProcess(DWORD dwProcessId);
HWND GetCurrentProcessMainWindow();
HWND ReadChildWindowHandle(); // �i�s�W�j�ŧiŪ���l�����y�`���禡