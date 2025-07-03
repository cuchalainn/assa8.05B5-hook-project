// 檔案: helpers.cpp
#include "pch.h"
#include "helpers.h"
#include "Globals.h"

// ========== 常數定義 ==========
namespace Addr {
    // Assa 記憶體中儲存目標石器PID的位址偏移
    constexpr DWORD TargetPidOffset = 0xF5304;
    // Assa 記憶體中用於發送命令的字串位址偏移
    constexpr DWORD CommandStringOffset = 0xF53D1;
}

// ========== 輔助函式實作 ==========

// 用於 EnumWindows 的回呼函式所需的資料結構
struct EnumWindowData {
    DWORD dwProcessId;
    HWND hMainWindow;
};

// EnumWindows 的回呼函式，用於比對 Process ID 並找到主視窗
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam) {
    auto* pData = (EnumWindowData*)lParam;
    DWORD dwProcessId = 0;
    GetWindowThreadProcessId(hWnd, &dwProcessId);

    // 檢查PID是否匹配，且視窗可見、有標題
    if (pData->dwProcessId == dwProcessId && IsWindowVisible(hWnd) && GetWindowTextLengthW(hWnd) > 0) {
        pData->hMainWindow = hWnd;
        return FALSE; // 找到後停止列舉
    }
    return TRUE; // 繼續列舉
}

// 獲取當前行程 (Assa) 的主視窗句柄
HWND GetCurrentProcessMainWindow() {
    EnumWindowData data = { GetCurrentProcessId(), NULL };
    EnumWindows(EnumWindowsProc, (LPARAM)&data);
    return data.hMainWindow;
}

// 根據行程ID獲取其主視窗句柄
HWND GetMainWindowForProcess(DWORD dwProcessId) {
    EnumWindowData data = { dwProcessId, NULL };
    EnumWindows(EnumWindowsProc, (LPARAM)&data);
    return data.hMainWindow;
}

// 從 Assa 記憶體中讀取目標石器(SA)的 PID
DWORD ReadSapid() {
    // 【修正】改為使用在 dllmain 中初始化的全域變數 g_assaInfo.base
    if (!g_assaInfo.base) return 0;
    return *((DWORD*)((BYTE*)g_assaInfo.base + Addr::TargetPidOffset));
}

// 將命令寫入 Assa 的命令記憶體區
void WriteCommandToMemory(const std::vector<BYTE>& commandBytes) {
    // 【修正】改為使用全域變數 g_assaInfo.base，解決按鈕因改名失效的問題
    if (!g_assaInfo.base) return;
    LPVOID stringAddress = (LPVOID)((BYTE*)g_assaInfo.base + Addr::CommandStringOffset);
    DWORD oldProtect;
    if (VirtualProtect(stringAddress, commandBytes.size(), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        memcpy(stringAddress, commandBytes.data(), commandBytes.size());
        VirtualProtect(stringAddress, commandBytes.size(), oldProtect, &oldProtect);
    }
}

// 獲取遠端行程的模組句柄
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

// 在記憶體中覆寫字串
void OverwriteStringInMemory(LPCWSTR targetAddress, const std::wstring& newString) {
    size_t bufferSize = (newString.length() + 1) * sizeof(wchar_t);
    DWORD oldProtect;
    if (VirtualProtect((LPVOID)targetAddress, bufferSize, PAGE_READWRITE, &oldProtect)) {
        wcscpy_s((wchar_t*)targetAddress, bufferSize / sizeof(wchar_t), newString.c_str());
        VirtualProtect((LPVOID)targetAddress, bufferSize, oldProtect, &oldProtect);
    }
}

// 讀取子視窗句柄
HWND ReadChildWindowHandle() {
    // 【修正】改為使用全域變數 g_assaInfo.base
    if (!g_assaInfo.base) return NULL;

    DWORD pointerOffsets[] = { 0xF69BC, 0xF69D0 };
    HWND childHwnd = NULL;
    // 【優化】直接使用已儲存的當前進程句柄 g_assaInfo.hProcess
    HANDLE hCurrentProcess = g_assaInfo.hProcess;

    for (DWORD offset : pointerOffsets) {
        // 解析多級指標: [[Assa8.0B5.exe + offset] + 0x10] + 0x40
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