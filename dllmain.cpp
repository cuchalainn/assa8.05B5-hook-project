// 檔案: dllmain.cpp
#include "pch.h"
#include "Globals.h"
#include "UIManager.h"
#include "Hooks.h"

// 主邏輯執行緒
DWORD WINAPI MainLogicThread(LPVOID) {
    InitializeUI();
    return 0;
}

// DLL 入口點
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        InstallAllHooks();
        HANDLE hThread = CreateThread(NULL, 0, MainLogicThread, NULL, 0, NULL);
        if (hThread) {
            CloseHandle(hThread);
        }
    }
    return TRUE;
}