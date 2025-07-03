// 檔案: dllmain.cpp
// 說明: 這是 DLL 的主要進入點。當此 DLL 被注入到目標行程 (Assa8.0B5.exe) 時，
//       作業系統會呼叫此處的 DllMain 函式。
#include "pch.h"
#include "Globals.h"      // 引用全域變數
#include "UIManager.h"    // 引用 UI 管理函式
#include "Hooks.h"        // 引用 Hook 安裝函式
#include "helpers.h"      // 引用輔助函式
#include "include/MinHook.h" // 引用 MinHook 標頭檔

// 函式: UiPreparationThread
// 功能: 在獨立背景執行緒中處理 UI 初始化，避免 DllMain 阻塞。
DWORD WINAPI UiPreparationThread(LPVOID) {
    // 循環等待，直到 Assa 的主視窗建立完成。
    while (GetCurrentProcessMainWindow() == NULL) {
        Sleep(100); // 短暫休眠，避免 CPU 空轉
    }

    // 當視窗準備好後，呼叫 UIManager 中的 PrepareUI 函式來進行 UI 修改。
    PrepareUI();

    return 0; // 執行緒正常結束
}

// 函式: DllMain
// 功能: DLL 的主要進入點函式。
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
    {
        // 當 DLL 載入時
        DisableThreadLibraryCalls(hModule);

        // 初始化 Assa 自身資訊
        g_assaInfo.base = GetModuleHandleW(NULL);
        g_assaInfo.PID = GetCurrentProcessId();
        g_assaInfo.hProcess = GetCurrentProcess();

        // 初始化 MinHook 函式庫
        if (MH_Initialize() != MH_OK) {
            MessageBoxW(NULL, L"MinHook 初始化失敗！", L"嚴重錯誤", MB_OK | MB_ICONERROR);
            return FALSE;
        }

        // 安裝所有定義在 Hooks.cpp 中的功能掛鉤
        InstallAllHooks();

        // 建立一個新執行緒來處理UI，防止阻塞主程式
        HANDLE hThread = CreateThread(NULL, 0, UiPreparationThread, NULL, 0, NULL);
        if (hThread) {
            CloseHandle(hThread); // 關閉執行緒句柄，執行緒會繼續在背景執行
        }
        break;
    }
    case DLL_PROCESS_DETACH:
    {
        // 當 DLL 卸載時
        MH_Uninitialize(); // 反初始化 MinHook，移除所有 Hook

        // 釋放資源
        if (g_saInfo.hProcess) {
            CloseHandle(g_saInfo.hProcess);
            g_saInfo.hProcess = NULL;
        }
        // 釋放 UI 建立的字型物件
        if (g_hCustomFont) DeleteObject(g_hCustomFont);
        if (g_hSnapButtonFont) DeleteObject(g_hSnapButtonFont);
        if (g_hPoemLabelFont) DeleteObject(g_hPoemLabelFont);

        break;
    }
    }
    return TRUE;
}