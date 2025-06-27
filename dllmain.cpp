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
    case DLL_PROCESS_ATTACH: // 當 DLL 被載入 (注入) 到目標行程時
    {
        // 禁用執行緒庫呼叫，這是一個小小的優化。
        // 它能防止系統為每個新建立的執行緒都呼叫一次 DllMain，減少不必要的開銷。
        DisableThreadLibraryCalls(hModule);
        // 【修改】在安裝任何 Hook 之前，必須先初始化 MinHook 函式庫。
        if (MH_Initialize() != MH_OK) {
            // 如果初始化失敗，通常意味著發生了嚴重錯誤，應阻止 DLL 繼續載入。
            MessageBoxW(NULL, L"MinHook 初始化失敗！", L"嚴重錯誤", MB_OK | MB_ICONERROR);
            return FALSE;
        }
        // 呼叫 Hooks.cpp 中的總安裝函式，安裝我們所有定義好的 Hook。
        InstallAllHooks();

        // 建立一個獨立執行緒來處理 UI 初始化，以避免阻塞 DllMain。
        // DllMain 中應盡量避免複雜或耗時的操作。
        HANDLE hThread = CreateThread(NULL, 0, UiPreparationThread, NULL, 0, NULL);
        if (hThread) {
            // 關閉執行緒句柄。這不會終止執行緒，只是告訴系統我們不再需要透過這個句柄來操作它。
            // 執行緒會在 UiPreparationThread 函式執行完畢後自行銷毀。
            CloseHandle(hThread);
        }
        break;
    }

    case DLL_PROCESS_DETACH: // 當 DLL 從目標行程中被卸載時
    {
        // 【修改】在 DLL 卸載前，反初始化 MinHook，它會自動移除所有已建立的 Hook。
        MH_Uninitialize();
        // 在 DLL 卸載時，安全地釋放所有我們申請的資源，防止記憶體洩漏。
        // 關閉與目標遊戲 (SA) 的行程句柄。
        if (g_saInfo.hProcess) {
            CloseHandle(g_saInfo.hProcess);
            g_saInfo.hProcess = NULL; // 設為 NULL，避免懸掛指針
        }
        // 銷毀我們在 UIManager 中建立的自訂字型物件。
        if (g_hCustomFont) {
            DeleteObject(g_hCustomFont);
            g_hCustomFont = NULL;
        }
        if (g_hInfoBoxFont) {
            DeleteObject(g_hInfoBoxFont);
            g_hInfoBoxFont = NULL;
        }
        break;
    }
    }
    return TRUE; // 必須返回 TRUE，表示 DllMain 成功執行
}