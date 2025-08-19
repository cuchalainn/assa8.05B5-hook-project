// �ɮ�: helpers.cpp
#include "pch.h"
#include "helpers.h"
#include "Globals.h"

// ========== �`�Ʃw�q ==========
namespace Addr {
    // �s�W�C�� HWND �������q
    constexpr DWORD TargetHwndOffset = 0xF5300;
    // Assa �O���餤�x�s�ؼХ۾�PID����}����
    constexpr DWORD TargetPidOffset = 0xF5304;
    // Assa �O���餤�Ω�o�e�R�O���r���}����
    constexpr DWORD CommandStringOffset = 0xF53D1;
}

// ========== ���U�禡��@ ==========

// �Ω� EnumWindows ���^�I�禡�һݪ���Ƶ��c
struct EnumWindowData {
    DWORD dwProcessId;
    HWND hMainWindow;
};

// EnumWindows ���^�I�禡�A�Ω��� Process ID �ç��D����
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam) {
    auto* pData = (EnumWindowData*)lParam;
    DWORD dwProcessId = 0;
    GetWindowThreadProcessId(hWnd, &dwProcessId);

    // �ˬdPID�O�_�ǰt�A�B�����i���B�����D
    if (pData->dwProcessId == dwProcessId && IsWindowVisible(hWnd) && GetWindowTextLengthW(hWnd) > 0) {
        pData->hMainWindow = hWnd;
        return FALSE; // ���ᰱ��C�|
    }
    return TRUE; // �~��C�|
}

// �����e��{ (Assa) ���D�����y�`
HWND GetCurrentProcessMainWindow() {
    EnumWindowData data = { GetCurrentProcessId(), NULL };
    EnumWindows(EnumWindowsProc, (LPARAM)&data);
    return data.hMainWindow;
}

// �ھڦ�{ID�����D�����y�`
HWND GetMainWindowForProcess(DWORD dwProcessId) {
    EnumWindowData data = { dwProcessId, NULL };
    EnumWindows(EnumWindowsProc, (LPARAM)&data);
    return data.hMainWindow;
}

// �i����ק�j��@�s�� ReadGameHandles �禡
// �o�Ө禡�|�����ק���쪺 g_saInfo �ܼ�
bool ReadGameHandles() {
    // �T�O�D���ݤw��l��
    if (!g_assaInfo.base) return false;

    // 1. Ū�� PID
    // �q (�D���ݰ� + PID����) ����}Ū�� DWORD (4-byte)
    g_saInfo.PID = *((DWORD*)((BYTE*)g_assaInfo.base + Addr::TargetPidOffset));

    // 2. Ū�� HWND
    // �q (�D���ݰ� + HWND����) ����}Ū�� HWND (���Фj�p)
    g_saInfo.HWND = *((HWND*)((BYTE*)g_assaInfo.base + Addr::TargetHwndOffset));

    // 3. ²������Ū���쪺�ȬO�_����
    if (g_saInfo.PID != 0 && g_saInfo.HWND != NULL && IsWindow(g_saInfo.HWND)) {
        return true;
    }

    // �p�G���@���L�ġA�h�M�Ũê�^����
    g_saInfo.PID = 0;
    g_saInfo.HWND = NULL;
    return false;
}

// �N�R�O�g�J Assa ���R�O�O�����
void WriteCommandToMemory(const std::vector<BYTE>& commandBytes) {
    // �i�ץ��j�אּ�ϥΥ����ܼ� g_assaInfo.base�A�ѨM���s�]��W���Ī����D
    if (!g_assaInfo.base) return;
    LPVOID stringAddress = (LPVOID)((BYTE*)g_assaInfo.base + Addr::CommandStringOffset);
    DWORD oldProtect;
    if (VirtualProtect(stringAddress, commandBytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        memcpy(stringAddress, commandBytes.data(), commandBytes.size());
        VirtualProtect(stringAddress, commandBytes.size(), oldProtect, &oldProtect);
    }
}

// ������ݦ�{���Ҳեy�`
HMODULE GetRemoteModuleHandle(DWORD dwProcessId, const wchar_t* szModuleName) {
    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwProcessId);
    if (NULL == hProcess) return NULL;

    HMODULE hMods[1024];
    DWORD cbNeeded;
    HMODULE hModule = NULL;

    if (EnumProcessModules(hProcess, hMods, sizeof(hMods), &cbNeeded)) {
        for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
            wchar_t szModName[MAX_PATH];
            if (GetModuleBaseName(hProcess, hMods[i], szModName, sizeof(szModName) / sizeof(wchar_t))) {
                if (wcscmp(szModName, szModuleName) == 0) {
                    hModule = hMods[i];
                    break;
                }
            }
        }
    }
    CloseHandle(hProcess);
    return hModule;
}

// �b�O���餤�мg�r��
void OverwriteStringInMemory(LPCWSTR targetAddress, const std::wstring& newString) {
    size_t bufferSize = (newString.length() + 1) * sizeof(wchar_t);
    DWORD oldProtect;
    if (VirtualProtect((LPVOID)targetAddress, bufferSize, PAGE_READWRITE, &oldProtect)) {
        wcscpy_s((wchar_t*)targetAddress, bufferSize / sizeof(wchar_t), newString.c_str());
        VirtualProtect((LPVOID)targetAddress, bufferSize, oldProtect, &oldProtect);
    }
}

// Ū���l�����y�`
HWND ReadChildWindowHandle() {
    // �i�ץ��j�אּ�ϥΥ����ܼ� g_assaInfo.base
    if (!g_assaInfo.base) return NULL;

    DWORD pointerOffsets[] = { 0xF69BC, 0xF69D0 };
    HWND childHwnd = NULL;
    // �i�u�ơj�����ϥΤw�x�s����e�i�{�y�` g_assaInfo.hProcess
    HANDLE hCurrentProcess = g_assaInfo.hProcess;

    for (DWORD offset : pointerOffsets) {
        // �ѪR�h�ū���: [[Assa8.0B5.exe + offset] + 0x10] + 0x40
        uintptr_t currentAddr = (uintptr_t)g_assaInfo.base + offset;

        if (ReadProcessMemory(hCurrentProcess, (LPCVOID)currentAddr, &currentAddr, sizeof(currentAddr), NULL)) {
            currentAddr += 0x10;
            if (ReadProcessMemory(hCurrentProcess, (LPCVOID)currentAddr, &currentAddr, sizeof(currentAddr), NULL)) {
                currentAddr += 0x40;
                if (ReadProcessMemory(hCurrentProcess, (LPCVOID)currentAddr, &childHwnd, sizeof(childHwnd), NULL)) {
                    if (childHwnd != NULL) {
                        break;
                    }
                }
            }
        }
    }

    if (childHwnd && !IsWindow(childHwnd)) {
        childHwnd = NULL;
    }
    return childHwnd;
}