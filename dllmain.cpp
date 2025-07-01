// 檔案: dllmain.cpp
// 說明: 這是 DLL 的主要進入點。當此 DLL 被注入到目標行程 (Assa8.0B5.exe) 時，
//       作業系統會呼叫此處的 DllMain 函式。
#include "pch.h"          // 預編譯標頭檔，包含常用的系統標頭
#include "Globals.h"      // 引用全域變數定義
#include "UIManager.h"    // 引用 UI 管理相關函式
#include "Hooks.h"        // 引用 Hook 安裝函式
#include "helpers.h"      // 引用輔助函式
#include "MinHook.h" // 【修改】引用 MinHook 標頭檔
// 函式: UiPreparationThread
// 功能: 這是一個獨立的背景執行緒函式，專門用來處理 UI 的初始化。
//       目的是為了避免在 DllMain 中執行耗時操作，防止目標程式卡死。
// 調用: 由 DllMain 在 DLL_PROCESS_ATTACH 事件中透過 CreateThread 啟動。
DWORD WINAPI UiPreparationThread(LPVOID lpParam) {
    // 循環等待，直到 Assa 的主視窗建立完成。
    // GetCurrentProcessMainWindow 是我們在 helpers.cpp 中定義的輔助函式。
    while (GetCurrentProcessMainWindow() == NULL) {
        Sleep(100); // 短暫休眠，避免 CPU 空轉
    }

    // 當視窗準備好後，呼叫 UIManager 中的 PrepareUI 函式來進行 UI 修改。
    PrepareUI();

    return 0; // 執行緒正常結束
}
// 函式: DllMain
// 功能: DLL 的主要進入點函式。
// 說明: 當 DLL 被載入到一個行程或從行程中卸載時，作業系統會呼叫此函式。
//       hModule: DLL 自己的模組句柄。
//       reason: 呼叫的原因 (載入、卸載等)。
BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
    {
        // 【新增】在DLL載入時，取得並儲存主程式的模組基底位址
        g_assaInfo.base = GetModuleHandleW(NULL);

        DisableThreadLibraryCalls(hModule);
        if (MH_Initialize() != MH_OK) {
            MessageBoxW(NULL, L"MinHook 初始化失敗！", L"嚴重錯誤", MB_OK | MB_ICONERROR);
            return FALSE;
        }
        InstallAllHooks();
        HANDLE hThread = CreateThread(NULL, 0, UiPreparationThread, NULL, 0, NULL);
        if (hThread) {
            CloseHandle(hThread);
        }
        break;
    }
    case DLL_PROCESS_DETACH:
    {
        MH_Uninitialize();
        if (g_saInfo.hProcess) {
            CloseHandle(g_saInfo.hProcess);
            g_saInfo.hProcess = NULL;
        }
        if (g_hCustomFont) {
            DeleteObject(g_hCustomFont);
            g_hCustomFont = NULL;
        }
        // 確認這兩行存在，以安全地釋放新創建的字型物件
        if (g_hSnapButtonFont) {
            DeleteObject(g_hSnapButtonFont);
            g_hSnapButtonFont = NULL;
        }
        if (g_hPoemLabelFont) {
            DeleteObject(g_hPoemLabelFont);
            g_hPoemLabelFont = NULL;
        }
        break;
    }
    }
    return TRUE;
}