// �ɮ�: helpers.cpp
#include "pch.h"
#include "helpers.h"
#include "Globals.h" 
// ========== �`�Ʃw�q ==========
namespace Addr {
    constexpr DWORD TargetPidOffset = 0xF5304;
    constexpr DWORD CommandStringOffset = 0xF53D1;
}

// ========== ���U�禡��@ ==========

// �i�s�W�j�o�ӵ��c��O�M������禡����¦
struct EnumWindowData { DWORD dwProcessId; HWND hMainWindow; };

// �i�s�W�j�o�Ӧ^�I�禡�]�O�M������禡����¦
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam) {
    auto* pData = (EnumWindowData*)lParam;
    DWORD dwProcessId = 0;
    GetWindowThreadProcessId(hWnd, &dwProcessId);
    if (pData->dwProcessId == dwProcessId && IsWindowVisible(hWnd) && GetWindowTextLengthW(hWnd) > 0) {
        pData->hMainWindow = hWnd;
        return FALSE;
    }
    return TRUE;
}

// �i�s�W�j�ɥ� GetCurrentProcessMainWindow �禡
HWND GetCurrentProcessMainWindow() {
    EnumWindowData data = { GetCurrentProcessId(), NULL };
    EnumWindows(EnumWindowsProc, (LPARAM)&data);
    return data.hMainWindow;
}

// �i�s�W�j�ɥ� GetMainWindowForProcess �禡
HWND GetMainWindowForProcess(DWORD dwProcessId) {
    EnumWindowData data = { dwProcessId, NULL };
    EnumWindows(EnumWindowsProc, (LPARAM)&data);
    return data.hMainWindow;
}

// �i�s�W�j�ɥ� ReadSapid �禡
DWORD ReadSapid() {
    HMODULE hMod = GetModuleHandleW(L"Assa8.0B5.exe");
    if (!hMod) return 0;
    return *((DWORD*)((BYTE*)hMod + Addr::TargetPidOffset));
}

void WriteCommandToMemory(const std::vector<BYTE>& commandBytes) {
    HMODULE hMod = GetModuleHandleW(L"Assa8.0B5.exe");
    if (!hMod) return;
    LPVOID stringAddress = (LPVOID)((BYTE*)hMod + Addr::CommandStringOffset);
    DWORD oldProtect;
    if (VirtualProtect(stringAddress, commandBytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        memcpy(stringAddress, commandBytes.data(), commandBytes.size());
        VirtualProtect(stringAddress, commandBytes.size(), oldProtect, &oldProtect);
    }
}

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

void OverwriteStringInMemory(LPCWSTR targetAddress, const std::wstring& newString) {
    size_t bufferSize = (newString.length() + 1) * sizeof(wchar_t);
    DWORD oldProtect;
    if (VirtualProtect((LPVOID)targetAddress, bufferSize, PAGE_READWRITE, &oldProtect)) {
        wcscpy_s((wchar_t*)targetAddress, bufferSize / sizeof(wchar_t), newString.c_str());
        VirtualProtect((LPVOID)targetAddress, bufferSize, oldProtect, &oldProtect);
    }
}

// �i�s�W�j�o�Ө禡�M���Ψ�Ū���l�������y�`
HWND ReadChildWindowHandle() {
    HMODULE hMod = GetModuleHandleW(L"Assa8.0B5.exe");
    if (!hMod) return NULL;

    // �w�q��ӥi�઺�򩳫��Ц�}
    DWORD pointerOffsets[] = { 0xF69BC, 0xF69D0 };
    HWND childHwnd = NULL;

    for (DWORD offset : pointerOffsets) {
        // �}�l�ѪR�h�ū���
        // [[Assa8.0B5.exe + offset] + 0x10] + 0x40
        uintptr_t currentAddr = (uintptr_t)hMod + offset;

        // Ū���Ĥ@�h����
        if (ReadProcessMemory(g_assaInfo.hProcess, (LPCVOID)currentAddr, &currentAddr, sizeof(currentAddr), NULL)) {
            // �[�W 0x10
            currentAddr += 0x10;
            // Ū���ĤG�h����
            if (ReadProcessMemory(g_assaInfo.hProcess, (LPCVOID)currentAddr, &currentAddr, sizeof(currentAddr), NULL)) {
                // �[�W 0x40
                currentAddr += 0x40;
                // Ū���̲ת� HWND ��
                if (ReadProcessMemory(g_assaInfo.hProcess, (LPCVOID)currentAddr, &childHwnd, sizeof(childHwnd), NULL)) {
                    // �p�G���\Ū��@�ӫD�s���y�`�A�N���X�j��
                    if (childHwnd != NULL) {
                        break;
                    }
                }
            }
        }
    }
    // �p�G�̲ץy�`���ġA�������w�����A�]�N������L��
    if (childHwnd && !IsWindow(childHwnd)) {
        childHwnd = NULL;
    }
    return childHwnd;
}