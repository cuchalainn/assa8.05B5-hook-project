// 檔案: UIManager.cpp
#include "pch.h"
#include "UIManager.h"
#include "Globals.h"
#include "helpers.h" // 【重點】引用 helpers.h
#include "RiddleDatabase.h"

// --- 【新增】用於儲存我們要操作的控制項句柄的全域靜態變數 ---
// 'static' 關鍵字讓這些變數僅在此 .cpp 檔案內可見
static HWND g_hFastWalk = NULL;
static HWND g_hFastWalkParent = NULL;
static HWND g_hWallHack = NULL;
static HWND g_hAutoRiddle = NULL;
static HWND g_hAutoRiddleParent = NULL;
static HWND g_hDataDisplay = NULL;
static HWND g_hDataDisplayParent = NULL;

#define ID_BUTTON_MY_NEW_BUTTON 9004

// 獨立功能函式實作
void encountON() {
    WriteCommandToMemory({ 0x2F,0x00,0x65,0x00,0x6E,0x00,0x63,0x00,0x6F,0x00,0x75,0x00,0x6E,0x00,0x74,0x00,0x20,0x00,0x6F,0x00,0x6E,0x00,0x00,0x00,0x20,0x00,0x20 });
}
void accompany() {
    WriteCommandToMemory({ 0x2F,0x00,0x61,0x00,0x63,0x00,0x63,0x00,0x6F,0x00,0x6D,0x00,0x70,0x00,0x61,0x00,0x6E,0x00,0x79,0x00,0x00,0x00,0x00,0x00,0x20,0x00 });
}

// 主要視窗處理程序
LRESULT CALLBACK UniversalCustomWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WNDPROC pOldProc = (WNDPROC)GetPropW(hwnd, L"OldWndProc");
    if (!pOldProc) {
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    if (uMsg == WM_LBUTTONUP) {
        wchar_t controlText[256] = { 0 };
        GetWindowTextW(hwnd, controlText, 256);

        if (wcscmp(controlText, L"啟動石器") == 0) {
            LRESULT result = CallWindowProcW(pOldProc, hwnd, uMsg, wParam, lParam);
            Sleep(300);
            Sa_PID = ReadSapid();
            if (Sa_PID != 0) {
                Sa_HWND = GetMainWindowForProcess(Sa_PID);
                Sa_base = GetRemoteModuleHandle(Sa_PID, L"sa_8002a.exe");
            }
            else {
                Sa_HWND = NULL; Sa_base = NULL;
            }
            if (!Sa_HWND || !Sa_base) {
                Sa_PID = 0; Sa_HWND = NULL; Sa_base = NULL;
            }
            return result;
        }
        // 【新增】處理我們自訂按鈕的點擊事件
        else if (GetDlgCtrlID(hwnd) == ID_BUTTON_MY_NEW_BUTTON) {
            MessageBoxW(hwnd, L"自訂按鈕點擊成功！", L"提示", MB_OK | MB_ICONINFORMATION);
            // 您可以在此處呼叫任何功能，例如 accompany();
            return 0;
        }
    }

    return CallWindowProcW(pOldProc, hwnd, uMsg, wParam, lParam);
}

// UI初始化函式
BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam) {
    wchar_t text[256] = { 0 };
    GetWindowTextW(hwnd, text, 256);
    if (wcscmp(text, L"快速行走") == 0) {
        g_hFastWalk = hwnd;
    }
    else if (wcscmp(text, L"穿牆行走") == 0) {
        g_hWallHack = hwnd;
    }
    else if (wcscmp(text, L"自動猜迷") == 0) { 
        g_hAutoRiddle = hwnd;
    }

    const std::vector<std::pair<std::wstring, std::wstring>> mappings = {
        {L"激活石器", L"啟動石器"}, {L"自動KNPC", L"NPC對戰"}, {L"自動戰斗", L"自動戰鬥"},
        {L"快速戰斗", L"快速戰鬥"}, {L"戰斗設定", L"戰鬥設定"}, {L"戰斗設置", L"戰鬥設置"},
        {L"決斗", L"決鬥"},{L"合成|料理|精鏈", L"合成|料理|精煉"}
    };
    for (const auto& map : mappings) {
        if (wcsstr(text, map.first.c_str())) {
            SetWindowTextW(hwnd, map.second.c_str());
            break;
        }
    }
    if (wcsstr(text, L"激活石器")) {
        if (GetPropW(hwnd, L"OldWndProc") == NULL) {
            WNDPROC pOldProc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)UniversalCustomWndProc);
            if (pOldProc) { SetPropW(hwnd, L"OldWndProc", (HANDLE)pOldProc); }
        }
    }
    return TRUE;
}

void InitializeUI() {
    Assa_HWND = GetCurrentProcessMainWindow();
    if (Assa_HWND) {
        // 1. 初始化全域句柄變數
        g_hFastWalk = NULL;
        g_hWallHack = NULL;
        g_hAutoRiddle = NULL;
        //呼叫 EnumChildProc 進行遍歷，填充上面的 g_h... 變數
        EnumChildWindows(Assa_HWND, EnumChildProc, NULL);
        // --- 建立一個與遊戲原生外觀一致的新按鈕 ---
        HWND hNewButton = CreateWindowExW(
            0,
            L"ThunderRT6CommandButton", // 【修改】使用您從Spy++找到的正確類別名稱
            L"測試按鈕",                 // 按鈕標題
            WS_CHILD | WS_VISIBLE | WS_TABSTOP, // 【修改】使用從Spy++觀察到的正確樣式
            10, 10,                     // X, Y 座標
            120, 30,                    // 寬, 高
            Assa_HWND,                  // 父視窗是主程式視窗
            (HMENU)ID_BUTTON_MY_NEW_BUTTON, // 將我們定義的ID賦予給它
            GetModuleHandle(NULL),
            NULL
        );

        // 為這個新按鈕掛上我們的訊息處理程序，使其具備功能
        if (hNewButton) {
            WNDPROC pOldProc = (WNDPROC)SetWindowLongPtrW(hNewButton, GWLP_WNDPROC, (LONG_PTR)UniversalCustomWndProc);
            if (pOldProc) {
                SetPropW(hNewButton, L"OldWndProc", (HANDLE)pOldProc);
            }
        }

        //遍歷完成後，根據偵察結果執行所有UI修改
        if (g_hFastWalk && g_hAutoRiddle) { // 確保核心目標已找到

            //隱藏「快速行走」的父容器
            HWND hFastWalkParent = GetParent(g_hFastWalk);
            if (hFastWalkParent) ShowWindow(hFastWalkParent, SW_HIDE);

            //隱藏「自動猜迷」按鈕本身
            ShowWindow(g_hAutoRiddle, SW_HIDE);

            // 取得「自動猜迷」的父容器作為移動的目標容器
            HWND hNewParent = GetParent(g_hAutoRiddle);
            if (hNewParent) {

                //移動「快速行走」
                SetParent(g_hFastWalk, hNewParent);
                ShowWindow(g_hFastWalk, SW_SHOW); // 確保它可見
                MoveWindow(g_hFastWalk, 108, 22, 102, 20, TRUE); // 可自行調整寬高

                //移動「穿牆行走」
                if (g_hWallHack) {
                    SetParent(g_hWallHack, hNewParent);
                    ShowWindow(g_hWallHack, SW_SHOW);
                    MoveWindow(g_hWallHack, 5, 69, 102, 20, TRUE);
                }
            }
        }
    }
}