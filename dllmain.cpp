// 檔案: dllmain.cpp
#include "pch.h"
#include "Globals.h"
#include "UIManager.h"
#include "Hooks.h"
#include "helpers.h"

// 需要 UIManager.h 來取得全域字型句柄，以便釋放資源
#include "UIManager.h"

// 背景執行緒，負責執行安全的 UI 準備工作
DWORD WINAPI UiPreparationThread(LPVOID lpParam) {
    // 循環等待，直到主程式的視窗建立完成
    while (GetCurrentProcessMainWindow() == NULL) {
        Sleep(100);
    }
    PrepareUI();
    return 0;
}

// DLL 入口點
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
    {
        DisableThreadLibraryCalls(hModule);
        InstallAllHooks(); // 安裝所有功能的 Hook
        // 建立一個獨立執行緒來處理 UI 初始化，避免阻塞目標程式
        HANDLE hThread = CreateThread(NULL, 0, UiPreparationThread, NULL, 0, NULL);
        if (hThread) {
            CloseHandle(hThread);
        }
        break;
    }

    case DLL_PROCESS_DETACH:
    {
        // 在 DLL 卸載時，安全地釋放所有資源
        if (g_saInfo.hProcess) {
            CloseHandle(g_saInfo.hProcess);
            g_saInfo.hProcess = NULL;
        }
        // 銷毀我們建立的字型物件，防止記憶體洩漏
        if (g_hCustomFont) {
            DeleteObject(g_hCustomFont);
        }
        if (g_hInfoBoxFont) {
            DeleteObject(g_hInfoBoxFont);
        }
        break;
    }
    }
    return TRUE;
}