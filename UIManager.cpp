// 檔案: UIManager.cpp
#include "pch.h"
#include "UIManager.h"
#include "Globals.h"
#include "helpers.h"

// 全域變數定義
HFONT g_hCustomFont = NULL;
HFONT g_hInfoBoxFont = NULL;

// 內部使用的靜態變數
static bool g_areButtonsCreated = false;
static HWND g_hFastWalk = NULL;
static HWND g_hWallHack = NULL;
static HWND g_hAutoRiddle = NULL;
static HWND g_hTargetParent = NULL; // 用於存放新按鈕的父容器句柄

// 按鈕 ID 定義
#define ID_BUTTON_1 9001
#define ID_BUTTON_2 9002
#define ID_BUTTON_3 9003
#define ID_BUTTON_4 9004
#define ID_BUTTON_5 9005
#define ID_BUTTON_6 9006
static HWND g_hDiceButtons[6] = { NULL };

// 函式宣告
void CreateAllButtons();
void autosay(int button_id);
LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// 專為 "啟動石器" 按鈕設計的視窗程序
LRESULT CALLBACK LaunchButtonWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WNDPROC pOldProc = (WNDPROC)GetPropW(hwnd, L"OldWndProc");
    if (!pOldProc) {
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    if (uMsg == WM_LBUTTONUP) {
        LRESULT result = CallWindowProcW(pOldProc, hwnd, uMsg, wParam, lParam);
        Sleep(300); // 等待石器時代程式啟動
        // 更新目標程式資訊
        g_saInfo.PID = ReadSapid();
        if (g_saInfo.PID != 0) {
            g_saInfo.HWND = GetMainWindowForProcess(g_saInfo.PID);
            g_saInfo.base = GetRemoteModuleHandle(g_saInfo.PID, L"sa_8002a.exe");
            if (g_saInfo.hProcess) CloseHandle(g_saInfo.hProcess);
            g_saInfo.hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, g_saInfo.PID);
            if (!g_saInfo.HWND || !g_saInfo.base || !g_saInfo.hProcess) {
                if (g_saInfo.hProcess) CloseHandle(g_saInfo.hProcess);
                memset(&g_saInfo, 0, sizeof(TargetInfo));
            }
        }
        else {
            if (g_saInfo.hProcess) CloseHandle(g_saInfo.hProcess);
            memset(&g_saInfo, 0, sizeof(TargetInfo));
        }
        // 如果按鈕尚未建立，則呼叫建立函式
        if (!g_areButtonsCreated) {
            CreateAllButtons();
            g_areButtonsCreated = true;
        }
        return result;
    }
    return CallWindowProcW(pOldProc, hwnd, uMsg, wParam, lParam);
}

// 掛載在主視窗上的視窗程序，用於接收自訂按鈕的點擊消息
LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WNDPROC pOldProc = (WNDPROC)GetPropW(hwnd, L"ParentOldWndProc");
    if (!pOldProc) {
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    if (uMsg == WM_COMMAND) {
        if (HIWORD(wParam) == BN_CLICKED) { // 判斷是否為按鈕點擊事件
            int button_id = LOWORD(wParam);
            if (button_id >= ID_BUTTON_1 && button_id <= ID_BUTTON_6) {
                autosay(button_id); // 呼叫核心功能函式
                return 0; // 消息已處理
            }
        }
    }
    return CallWindowProcW(pOldProc, hwnd, uMsg, wParam, lParam);
}

// 按鈕功能的核心函式，負責修改記憶體中的命令
void autosay(int button_id) {
    HMODULE hMod = GetModuleHandleW(L"Assa8.0B5.exe");
    if (!hMod) return;
    std::vector<BYTE> commandBytes;
    switch (button_id) {
    case ID_BUTTON_1: commandBytes = { 0x2F, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x75, 0x00, 0x6E, 0x00, 0x74, 0x00, 0x20, 0x00, 0x6F, 0x00, 0x6E, 0x00, 0x00, 0x00 }; break;
    case ID_BUTTON_2: commandBytes = { 0x2F, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x75, 0x00, 0x6E, 0x00, 0x74, 0x00, 0x20, 0x00, 0x6F, 0x00, 0x66, 0x00, 0x66, 0x00, 0x00, 0x00 }; break;
    case ID_BUTTON_3: commandBytes = { 0x2F, 0x00, 0x61, 0x00, 0x63, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6D, 0x00, 0x70, 0x00, 0x61, 0x00, 0x6E, 0x00, 0x79, 0x00, 0x00, 0x00 }; break;
    case ID_BUTTON_4: commandBytes = { 0x4D, 0x52, 0x80, 0x5F, 0x92, 0x51, 0xAA, 0x96, 0x05, 0x80, 0x4B, 0x4E, 0xF6, 0x5C, 0x00, 0x00 }; break;
    case ID_BUTTON_5: commandBytes = { 0x2F, 0x00, 0x70, 0x00, 0x69, 0x00, 0x6C, 0x00, 0x65, 0x00, 0x00, 0x00 }; break;
    case ID_BUTTON_6: commandBytes = { 0x2F, 0x00, 0x6F, 0x00, 0x66, 0x00, 0x66, 0x00, 0x6C, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x65, 0x00, 0x20, 0x00, 0x6F, 0x00, 0x6E, 0x00, 0x00, 0x00 }; break;
    default: return;
    }
    // 將命令寫入記憶體
    LPVOID stringAddress = (LPVOID)((BYTE*)hMod + 0xF53D1);
    DWORD oldProtect;
    BYTE clearBuffer[40] = { 0 };
    if (VirtualProtect(stringAddress, sizeof(clearBuffer), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        memcpy(stringAddress, clearBuffer, sizeof(clearBuffer)); // 先清空
        if (!commandBytes.empty()) {
            memcpy(stringAddress, commandBytes.data(), commandBytes.size()); // 再寫入
        }
        VirtualProtect(stringAddress, sizeof(clearBuffer), oldProtect, &oldProtect);
    }
}

// 遍歷子視窗，修改文字並掛鉤特定按鈕
BOOL CALLBACK FindAndHookControlsProc(HWND hwnd, LPARAM lParam) {
    wchar_t text[256] = { 0 };
    GetWindowTextW(hwnd, text, 256);

    // 尋找目標容器
    if (g_hTargetParent == NULL && wcsstr(text, L"腳本制作")) {
        g_hTargetParent = GetParent(hwnd);
    }
    // 修改靜態文字
    const std::vector<std::pair<std::wstring, std::wstring>> mappings = {
        {L"激活石器", L"啟動石器"}, {L"自動KNPC", L"NPC對戰"}, {L"自動戰斗", L"自動戰鬥"},
        {L"快速戰斗", L"快速戰鬥"}, {L"戰斗設定", L"戰鬥設定"}, {L"戰斗設置", L"戰鬥設置"},
        {L"決斗", L"決鬥"}, {L"合成|料理|精鏈", L"合成|料理|精煉"}
    };
    for (const auto& map : mappings) {
        if (wcsstr(text, map.first.c_str())) {
            SetWindowTextW(hwnd, map.second.c_str());
            GetWindowTextW(hwnd, text, 256);
            break;
        }
    }
    // 掛鉤「啟動石器」按鈕
    if (wcscmp(text, L"啟動石器") == 0) {
        wchar_t className[256] = { 0 };
        GetClassNameW(hwnd, className, 256);
        if (wcscmp(className, L"ThunderRT6CommandButton") == 0) {
            if (GetPropW(hwnd, L"OldWndProc") == NULL) {
                WNDPROC pOldProc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)LaunchButtonWndProc);
                if (pOldProc) { SetPropW(hwnd, L"OldWndProc", (HANDLE)pOldProc); }
            }
        }
    }
    // 儲存其他需要操作的控制項句柄
    if (wcscmp(text, L"快速行走") == 0) g_hFastWalk = hwnd;
    else if (wcscmp(text, L"穿牆行走") == 0) g_hWallHack = hwnd;
    else if (wcscmp(text, L"自動猜迷") == 0) g_hAutoRiddle = hwnd;
    return TRUE;
}

// 建立所有自訂按鈕
void CreateAllButtons() {
    HWND hParent = g_hTargetParent ? g_hTargetParent : g_assaInfo.HWND;
    if (!hParent) return;

    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtrW(hParent, GWLP_HINSTANCE);
    // 建立按鈕字型
    if (g_hCustomFont == NULL) {
        g_hCustomFont = CreateFontW(-MulDiv(85, GetDeviceCaps(GetDC(hParent), LOGPIXELSY), 720), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft JhengHei UI");
    }
    // 定義按鈕佈局
    const int start_x = 5, start_y = 47, btn_w = 100, btn_h = 25, gap_x = 6, gap_y = 6;
    POINT positions[6] = { {start_x, start_y}, {start_x + btn_w + gap_x, start_y}, {start_x, start_y + btn_h + gap_y}, {start_x + btn_w + gap_x, start_y + btn_h + gap_y}, {start_x, start_y + 2 * (btn_h + gap_y)}, {start_x + btn_w + gap_x, start_y + 2 * (btn_h + gap_y)} };
    const int ids[6] = { ID_BUTTON_1, ID_BUTTON_2, ID_BUTTON_3, ID_BUTTON_4, ID_BUTTON_5, ID_BUTTON_6 };
    const wchar_t* labels[6] = { L"開始遇敵", L"停止遇敵", L"呼喚野獸", L"冒險公會", L"堆疊道具", L"離線掛機" };

    // 迴圈建立按鈕
    for (int i = 0; i < 6; ++i) {
        g_hDiceButtons[i] = CreateWindowExW(0, L"BUTTON", labels[i], WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, positions[i].x, positions[i].y, btn_w, btn_h, hParent, (HMENU)ids[i], hInstance, NULL);
        if (g_hDiceButtons[i]) {
            if (g_hCustomFont) { SendMessage(g_hDiceButtons[i], WM_SETFONT, (WPARAM)g_hCustomFont, TRUE); }
            SetWindowPos(g_hDiceButtons[i], HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        }
    }
}

// DLL 注入後執行的 UI 準備函式
void PrepareUI() {
    g_assaInfo.HWND = GetCurrentProcessMainWindow();
    if (g_assaInfo.HWND) {
        // 掛鉤主視窗以接收消息
        if (GetPropW(g_assaInfo.HWND, L"ParentOldWndProc") == NULL) {
            WNDPROC pOldProc = (WNDPROC)SetWindowLongPtrW(g_assaInfo.HWND, GWLP_WNDPROC, (LONG_PTR)ParentWndProc);
            if (pOldProc) { SetPropW(g_assaInfo.HWND, L"ParentOldWndProc", (HANDLE)pOldProc); }
        }
        // 遍歷並準備 UI
        EnumChildWindows(g_assaInfo.HWND, FindAndHookControlsProc, NULL);
        // 重新佈局舊有 UI
        if (g_hFastWalk && g_hAutoRiddle) {
            HWND hFastWalkParent = GetParent(g_hFastWalk);
            if (hFastWalkParent) ShowWindow(hFastWalkParent, SW_HIDE);
            ShowWindow(g_hAutoRiddle, SW_HIDE);
            HWND hNewParent = GetParent(g_hAutoRiddle);
            if (hNewParent) {
                SetParent(g_hFastWalk, hNewParent);
                ShowWindow(g_hFastWalk, SW_SHOW);
                MoveWindow(g_hFastWalk, 108, 22, 102, 20, TRUE);
                if (g_hWallHack) {
                    SetParent(g_hWallHack, hNewParent);
                    ShowWindow(g_hWallHack, SW_SHOW);
                    MoveWindow(g_hWallHack, 5, 69, 102, 20, TRUE);
                }
            }
        }
    }
}