// �ɮ�: helpers.h
#pragma once
#include <windows.h>
#include <string>
#include <vector>
#include <psapi.h>

#pragma comment(lib, "psapi.lib")

// ========== �@�λ��U�禡�ŧi ==========

// �ھڦ�{ID�M�ҲզW�١A������ݦ�{���Ҳեy�`
HMODULE GetRemoteModuleHandle(DWORD dwProcessId, const wchar_t* szModuleName);

// �N���w���줸�էǦC�g�J Assa ���R�O�O����ϰ�
void WriteCommandToMemory(const std::vector<BYTE>& commandBytes);

// �b�O���餤�мg�@�� (�e�r��) �r��
void OverwriteStringInMemory(LPCWSTR targetAddress, const std::wstring& newString);

// �q Assa �O���餤Ū���ؼХ۾�(SA)�� PID
DWORD ReadSapid();

// �ھڦ�{ID�����D�����y�`
HWND GetMainWindowForProcess(DWORD dwProcessId);

// �����e��{ (Assa) ���D�����y�`
HWND GetCurrentProcessMainWindow();

// �q Assa �O���餤Ū���l�������y�`
HWND ReadChildWindowHandle();