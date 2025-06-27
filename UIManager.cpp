// 檔案: UIManager.cpp
#include "pch.h"
#include "UIManager.h"
#include "Globals.h"
#include "helpers.h"
#include <map>
#include <vector>
#include <cstdio>

// --- 全域與靜態變數 ---
HFONT g_hCustomFont = NULL;
HFONT g_hSnapButtonFont = NULL;
HFONT g_hPoemLabelFont = NULL;

static bool g_areButtonsCreated = false;
static HWND g_hFastWalk = NULL;
static HWND g_hWallHack = NULL;
static HWND g_hAutoRiddle = NULL;
static HWND g_hTargetParent = NULL;
static HWND g_hInfoContainer = NULL;

static bool g_isPilingOn = false;
static bool g_isOfflineOn = false;

// 主視窗吸附相關
static HWND g_hMapDisplayButton = NULL;
static HWND g_hMapDisplayParent = NULL;
static RECT g_mapDisplayRect = { 0 };
static bool g_isWindowSnapping = false;
static UINT_PTR g_snapTimerId = 0;

// 子視窗吸附相關
static HWND g_hHomePageButton = NULL;
static HWND g_hHomePageButtonParent = NULL;
static RECT g_homePageButtonRect = { 0 };
static bool g_isChildWindowSnapping = false;
static UINT_PTR g_childSnapTimerId = 0;
static HWND g_hChildWindow = NULL;
static HWND g_hChildSnapButton = NULL; // 【新增】儲存「子窗吸附」按鈕的控制代碼

// 按鈕 ID 定義
enum ButtonID {
    ID_ENCOUNT_ON = 9001,
    ID_ENCOUNT_OFF,
    ID_ACCOMPANY,
    ID_ADVENTURE_GUILD,
    ID_PILE,
    ID_OFFLINE_ON,
    ID_WINDOW_SNAP,
    ID_CHILD_WINDOW_SNAP
};

#define TIMER_ID_MAIN_SNAP 1
#define TIMER_ID_CHILD_SNAP 2

// --- 函式宣告 ---
void CreateAllButtons();
void OnCustomButtonClick(int button_id, HWND hButton);
LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK CustomButtonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
VOID CALLBACK SnapWindowTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
VOID CALLBACK SnapChildWindowTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);

// "啟動石器" 按鈕的視窗程序
LRESULT CALLBACK LaunchButtonWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WNDPROC pOldProc = (WNDPROC)GetPropW(hwnd, L"OldWndProc");
    if (!pOldProc) { return DefWindowProcW(hwnd, uMsg, wParam, lParam); }
    if (uMsg == WM_LBUTTONUP) {
        LRESULT result = CallWindowProcW(pOldProc, hwnd, uMsg, wParam, lParam);
        bool success = false;
        for (int i = 0; i < 20; ++i) {
            Sleep(250);
            g_saInfo.PID = ReadSapid();
            if (g_saInfo.PID != 0) {
                g_saInfo.HWND = GetMainWindowForProcess(g_saInfo.PID);
                g_saInfo.base = GetRemoteModuleHandle(g_saInfo.PID, L"sa_8002a.exe");
                if (g_saInfo.hProcess) CloseHandle(g_saInfo.hProcess);
                g_saInfo.hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, g_saInfo.PID);
                if (g_saInfo.HWND && g_saInfo.base && g_saInfo.hProcess) {
                    success = true;
                    break;
                }
            }
        }
        if (!success) {
            if (g_saInfo.hProcess) CloseHandle(g_saInfo.hProcess);
            memset(&g_saInfo, 0, sizeof(TargetInfo));
        }
        if (!g_areButtonsCreated) {
            CreateAllButtons();
            g_areButtonsCreated = true;
        }
        return result;
    }
    return CallWindowProcW(pOldProc, hwnd, uMsg, wParam, lParam);
}

// 所有自訂按鈕共用的訊息處理程序
LRESULT CALLBACK CustomButtonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WNDPROC pOldProc = (WNDPROC)GetPropW(hwnd, L"OldButtonWndProc");
    if (uMsg == WM_LBUTTONUP) {
        OnCustomButtonClick(GetDlgCtrlID(hwnd), hwnd);
    }
    if (pOldProc) {
        return CallWindowProcW(pOldProc, hwnd, uMsg, wParam, lParam);
    }
    return DefWindowProcW(hwnd, uMsg, wParam, lParam);
}

// 主視窗程序
LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WNDPROC pOldProc = (WNDPROC)GetPropW(hwnd, L"ParentOldWndProc");
    if (!pOldProc) { return DefWindowProcW(hwnd, uMsg, wParam, lParam); }
    return CallWindowProcW(pOldProc, hwnd, uMsg, wParam, lParam);
}

// 主視窗吸附的計時器處理函式
VOID CALLBACK SnapWindowTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
    if (g_assaInfo.HWND && IsWindow(g_assaInfo.HWND) && g_saInfo.HWND && IsWindow(g_saInfo.HWND)) {
        RECT gameRect;
        GetWindowRect(g_saInfo.HWND, &gameRect);
        const int targetX = gameRect.right;
        const int targetY = gameRect.top;
        RECT assaRect;
        GetWindowRect(g_assaInfo.HWND, &assaRect);
        if (assaRect.left != targetX || assaRect.top != targetY) {
            SetWindowPos(g_assaInfo.HWND, NULL, targetX, targetY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
    }
    else {
        if (g_snapTimerId != 0) {
            KillTimer(NULL, g_snapTimerId);
            g_snapTimerId = 0;
            g_isWindowSnapping = false;
        }
    }
}

// 子視窗吸附的計時器處理函式
VOID CALLBACK SnapChildWindowTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime) {
    if (g_hChildWindow && IsWindow(g_hChildWindow) && g_saInfo.HWND && IsWindow(g_saInfo.HWND)) {
        RECT gameRect;
        GetWindowRect(g_saInfo.HWND, &gameRect);
        RECT childRect;
        GetWindowRect(g_hChildWindow, &childRect);
        int childWidth = childRect.right - childRect.left;
        const int targetX = gameRect.right - childWidth;
        const int targetY = gameRect.bottom;
        if (childRect.left != targetX || childRect.top != targetY) {
            SetWindowPos(g_hChildWindow, NULL, targetX, targetY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
    }
    else {
        // 【修改】當子視窗被關閉時，自動重設按鈕狀態
        if (g_childSnapTimerId != 0) {
            KillTimer(NULL, g_childSnapTimerId);
            g_childSnapTimerId = 0;
            g_isChildWindowSnapping = false;
            g_hChildWindow = NULL;
            // 如果按鈕還存在，就將其文字恢復
            if (g_hChildSnapButton && IsWindow(g_hChildSnapButton)) {
                SetWindowTextW(g_hChildSnapButton, L"子窗吸附");
            }
        }
    }
}

// 按鈕點擊核心功能
void OnCustomButtonClick(int button_id, HWND hButton) {
    static const std::map<int, std::vector<BYTE>> commandMap = {
        { ID_ENCOUNT_ON, { 0x2F, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x75, 0x00, 0x6E, 0x00, 0x74, 0x00, 0x20, 0x00, 0x6F, 0x00, 0x6E, 0x00, 0x00, 0x00 } },
        { ID_ENCOUNT_OFF, { 0x2F, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x75, 0x00, 0x6E, 0x00, 0x74, 0x00, 0x20, 0x00, 0x6F, 0x00, 0x66, 0x00, 0x66, 0x00, 0x00, 0x00 } },
        { ID_ACCOMPANY, { 0x2F, 0x00, 0x61, 0x00, 0x63, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6D, 0x00, 0x70, 0x00, 0x61, 0x00, 0x6E, 0x00, 0x79, 0x00, 0x00, 0x00 } },
        { ID_ADVENTURE_GUILD, { 0x4D, 0x52, 0x80, 0x5F, 0x92, 0x51, 0xAA, 0x96, 0x05, 0x80, 0x4B, 0x4E, 0xF6, 0x5C, 0x00, 0x00 } },
    };
    const std::vector<BYTE> pileOnCmd = { 0x2F, 0x00, 0x70, 0x00, 0x69, 0x00, 0x6C, 0x00, 0x65, 0x00, 0x20, 0x00, 0x6F, 0x00, 0x6E, 0x00, 0x00, 0x00 };
    const std::vector<BYTE> pileOffCmd = { 0x2F, 0x00, 0x70, 0x00, 0x69, 0x00, 0x6C, 0x00, 0x65, 0x00, 0x20, 0x00, 0x6F, 0x00, 0x66, 0x00, 0x66, 0x00, 0x00, 0x00 };
    const std::vector<BYTE> offlineOnCmd = { 0x2F, 0x00, 0x6F, 0x00, 0x66, 0x00, 0x66, 0x00, 0x6C, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x65, 0x00, 0x20, 0x00, 0x6F, 0x00, 0x6E, 0x00, 0x00, 0x00 };
    const std::vector<BYTE> offlineOffCmd = { 0x2F, 0x00, 0x6F, 0x00, 0x66, 0x00, 0x66, 0x00, 0x6C, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x65, 0x00, 0x20, 0x00, 0x6F, 0x00, 0x66, 0x00, 0x66, 0x00, 0x00, 0x00 };

    switch (button_id) {
    case ID_PILE: if (!g_isPilingOn) { WriteCommandToMemory(pileOnCmd); SetWindowTextW(hButton, L"取消堆疊"); }
                else { WriteCommandToMemory(pileOffCmd); SetWindowTextW(hButton, L"開啟堆疊"); } g_isPilingOn = !g_isPilingOn; break;
    case ID_OFFLINE_ON: if (!g_isOfflineOn) { WriteCommandToMemory(offlineOnCmd); SetWindowTextW(hButton, L"取消掛機"); }
                      else { WriteCommandToMemory(offlineOffCmd); SetWindowTextW(hButton, L"開啟掛機"); } g_isOfflineOn = !g_isOfflineOn; break;
    case ID_WINDOW_SNAP:
        g_isWindowSnapping = !g_isWindowSnapping;
        if (g_isWindowSnapping) {
            if (!g_saInfo.HWND || !IsWindow(g_saInfo.HWND)) { MessageBoxW(g_assaInfo.HWND, L"尚未啟動或綁定遊戲，無法吸附！", L"提示", MB_OK | MB_ICONINFORMATION); g_isWindowSnapping = false; return; }
            SetWindowTextW(hButton, L"視窗分離");
            SnapWindowTimerProc(NULL, 0, 0, 0);
            g_snapTimerId = SetTimer(NULL, TIMER_ID_MAIN_SNAP, 100, SnapWindowTimerProc);
        }
        else {
            SetWindowTextW(hButton, L"視窗吸附");
            if (g_snapTimerId != 0) { KillTimer(NULL, g_snapTimerId); g_snapTimerId = 0; }
        }
        break;
    case ID_CHILD_WINDOW_SNAP:
        g_isChildWindowSnapping = !g_isChildWindowSnapping;
        if (g_isChildWindowSnapping) {
            g_hChildWindow = ReadChildWindowHandle();
            if (!g_hChildWindow) {
                //MessageBoxW(g_assaInfo.HWND, L"子視窗未開啟，無法吸附！", L"提示", MB_OK | MB_ICONINFORMATION);
                g_isChildWindowSnapping = false;
                return;
            }
            SetWindowTextW(hButton, L"子窗分離");
            SnapChildWindowTimerProc(NULL, 0, 0, 0);
            g_childSnapTimerId = SetTimer(NULL, TIMER_ID_CHILD_SNAP, 100, SnapChildWindowTimerProc);
        }
        else {
            SetWindowTextW(hButton, L"子窗吸附");
            if (g_childSnapTimerId != 0) {
                KillTimer(NULL, g_childSnapTimerId);
                g_childSnapTimerId = 0;
                g_hChildWindow = NULL;
            }
        }
        break;
    default: auto it = commandMap.find(button_id); if (it != commandMap.end()) { WriteCommandToMemory(it->second); } break;
    }
}

// 遍歷子視窗
BOOL CALLBACK FindAndHookControlsProc(HWND hwnd, LPARAM lParam) {
    wchar_t text[256] = { 0 };
    GetWindowTextW(hwnd, text, 256);
    if (g_hTargetParent == NULL && wcsstr(text, L"腳本制作")) {
        g_hTargetParent = GetParent(hwnd);
    }
    static const std::vector<std::pair<std::wstring, std::wstring>> mappings = {
        {L"激活石器", L"啟動石器"}, {L"自動KNPC", L"NPC對戰"}, {L"自動戰斗", L"自動戰鬥"},
        {L"快速戰斗", L"快速戰鬥"}, {L"戰斗設定", L"戰鬥設定"}, {L"戰斗設置", L"戰鬥設置"},
        {L"決斗", L"決鬥"}, {L"合成|料理|精鏈", L"合成|料理|精煉"},{L"腳本制作", L"腳本製作"},
    };
    for (const auto& map : mappings) {
        if (wcsstr(text, map.first.c_str())) {
            SetWindowTextW(hwnd, map.second.c_str());
            GetWindowTextW(hwnd, text, 256);
            break;
        }
    }
    if (wcscmp(text, L"啟動石器") == 0) {
        wchar_t className[256] = { 0 }; GetClassNameW(hwnd, className, 256);
        if (wcscmp(className, L"ThunderRT6CommandButton") == 0 && GetPropW(hwnd, L"OldWndProc") == NULL) {
            WNDPROC pOldProc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)LaunchButtonWndProc);
            if (pOldProc) { SetPropW(hwnd, L"OldWndProc", (HANDLE)pOldProc); }
        }
    }
    if (g_hMapDisplayButton == NULL && wcscmp(text, L"地圖顯示") == 0) {
        g_hMapDisplayButton = hwnd;
        g_hMapDisplayParent = GetParent(hwnd);
        GetWindowRect(hwnd, &g_mapDisplayRect);
        MapWindowPoints(NULL, g_hMapDisplayParent, (LPPOINT)&g_mapDisplayRect, 2);
        if (g_hMapDisplayParent && GetPropW(g_hMapDisplayParent, L"OldButtonWndProc") == NULL) {
            WNDPROC pOldProc = (WNDPROC)SetWindowLongPtrW(g_hMapDisplayParent, GWLP_WNDPROC, (LONG_PTR)CustomButtonProc);
            if (pOldProc) { SetPropW(g_hMapDisplayParent, L"OldButtonWndProc", (HANDLE)pOldProc); }
        }
        ShowWindow(hwnd, SW_HIDE);
    }
    if (g_hHomePageButton == NULL && wcscmp(text, L"繁化主頁") == 0) {
        g_hHomePageButton = hwnd;
        g_hHomePageButtonParent = GetParent(hwnd);
        GetWindowRect(hwnd, &g_homePageButtonRect);
        MapWindowPoints(NULL, g_hHomePageButtonParent, (LPPOINT)&g_homePageButtonRect, 2);
        if (g_hHomePageButtonParent && GetPropW(g_hHomePageButtonParent, L"OldButtonWndProc") == NULL) {
            WNDPROC pOldProc = (WNDPROC)SetWindowLongPtrW(g_hHomePageButtonParent, GWLP_WNDPROC, (LONG_PTR)CustomButtonProc);
            if (pOldProc) { SetPropW(g_hHomePageButtonParent, L"OldButtonWndProc", (HANDLE)pOldProc); }
        }
        ShowWindow(hwnd, SW_HIDE);
    }
    if (wcscmp(text, L"快速行走") == 0) g_hFastWalk = hwnd;
    else if (wcscmp(text, L"穿牆行走") == 0) g_hWallHack = hwnd;
    else if (wcscmp(text, L"自動猜迷") == 0) g_hAutoRiddle = hwnd;
    return TRUE;
}

// 建立所有自訂UI
void CreateAllButtons() {
    auto subclassButton = [](HWND hButton) {
        if (hButton && GetPropW(hButton, L"OldButtonWndProc") == NULL) {
            WNDPROC pOldProc = (WNDPROC)SetWindowLongPtrW(hButton, GWLP_WNDPROC, (LONG_PTR)CustomButtonProc);
            if (pOldProc) { SetPropW(hButton, L"OldButtonWndProc", (HANDLE)pOldProc); }
        }
        };
    // Part 1: 主要功能按鈕
    HWND hButtonParent = g_hTargetParent ? g_hTargetParent : g_assaInfo.HWND;
    if (!hButtonParent) return;
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtrW(hButtonParent, GWLP_HINSTANCE);
    if (g_hCustomFont == NULL) {
        g_hCustomFont = CreateFontW(-MulDiv(85, GetDeviceCaps(GetDC(hButtonParent), LOGPIXELSY), 720), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft JhengHei UI");
    }
    const int start_x = 5, start_y = 47, btn_w = 100, btn_h = 25, gap_x = 6, gap_y = 6;
    struct ButtonInfo { int id; const wchar_t* label; POINT pos; SIZE size; };
    ButtonInfo buttons[] = {
        { ID_ENCOUNT_ON, L"開始遇敵", {start_x, start_y}, {btn_w, btn_h} },
        { ID_ENCOUNT_OFF, L"停止遇敵", {start_x + btn_w + gap_x, start_y}, {btn_w, btn_h} },
        { ID_ACCOMPANY, L"呼喚野獸", {start_x, start_y + btn_h + gap_y}, {btn_w, btn_h} },
        { ID_ADVENTURE_GUILD, L"冒險公會", {start_x + btn_w + gap_x, start_y + btn_h + gap_y}, {btn_w, btn_h} },
        { ID_PILE, L"開啟堆疊", {start_x, start_y + 2 * (btn_h + gap_y)}, {btn_w, btn_h} },
        { ID_OFFLINE_ON, L"開啟掛機", {start_x + btn_w + gap_x, start_y + 2 * (btn_h + gap_y)}, {btn_w, btn_h} }
    };
    for (const auto& btn : buttons) {
        HWND hButton = CreateWindowExW(0, L"BUTTON", btn.label, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, btn.pos.x, btn.pos.y, btn.size.cx, btn.size.cy, hButtonParent, (HMENU)btn.id, hInstance, NULL);
        if (hButton) { if (g_hCustomFont) { SendMessage(hButton, WM_SETFONT, (WPARAM)g_hCustomFont, TRUE); } subclassButton(hButton); }
    }
    // Part 2: 視窗吸附按鈕
    if (g_hMapDisplayParent && g_hMapDisplayButton) {
        HINSTANCE hSnapInstance = (HINSTANCE)GetWindowLongPtrW(g_hMapDisplayParent, GWLP_HINSTANCE);
        if (g_hSnapButtonFont == NULL) {
            g_hSnapButtonFont = CreateFontW(-MulDiv(90, GetDeviceCaps(GetDC(g_hMapDisplayParent), LOGPIXELSY), 720), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft JhengHei UI");
        }
        HWND hSnapButton = CreateWindowExW(0, L"BUTTON", L"視窗吸附", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, g_mapDisplayRect.left, g_mapDisplayRect.top, 105, 35, g_hMapDisplayParent, (HMENU)ID_WINDOW_SNAP, hSnapInstance, NULL);
        if (hSnapButton) { if (g_hSnapButtonFont) { SendMessage(hSnapButton, WM_SETFONT, (WPARAM)g_hSnapButtonFont, TRUE); } subclassButton(hSnapButton); }
    }
    // Part 3: 子視窗吸附按鈕
    if (g_hHomePageButtonParent && g_hHomePageButton) {
        HINSTANCE hChildSnapInstance = (HINSTANCE)GetWindowLongPtrW(g_hHomePageButtonParent, GWLP_HINSTANCE);
        if (g_hSnapButtonFont == NULL) {
            g_hSnapButtonFont = CreateFontW(-MulDiv(90, GetDeviceCaps(GetDC(g_hHomePageButtonParent), LOGPIXELSY), 720), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft JhengHei UI");
        }
        // 【修改】將創建好的按鈕句柄存入 g_hChildSnapButton
        g_hChildSnapButton = CreateWindowExW(0, L"BUTTON", L"子窗吸附", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, g_homePageButtonRect.left, g_homePageButtonRect.top, 105, 35, g_hHomePageButtonParent, (HMENU)ID_CHILD_WINDOW_SNAP, hChildSnapInstance, NULL);
        if (g_hChildSnapButton) { if (g_hSnapButtonFont) { SendMessage(g_hChildSnapButton, WM_SETFONT, (WPARAM)g_hSnapButtonFont, TRUE); } subclassButton(g_hChildSnapButton); }
    }
    // Part 4: 詩句文字標籤
    if (g_hInfoContainer) {
        HINSTANCE hInfoInstance = (HINSTANCE)GetWindowLongPtrW(g_hInfoContainer, GWLP_HINSTANCE);
        if (g_hPoemLabelFont == NULL) {
            g_hPoemLabelFont = CreateFontW(-MulDiv(80, GetDeviceCaps(GetDC(g_hInfoContainer), LOGPIXELSY), 720), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft JhengHei UI");
        }
        HWND hLabel = CreateWindowExW(0, L"STATIC", L"祝你們旅途愉快\r\n     耳畔常有陽光\r\n         直至夕陽西下   by 舞丑", WS_CHILD | WS_VISIBLE | SS_LEFT, 5, 126, 205, 65, g_hInfoContainer, NULL, hInfoInstance, NULL);
        if (hLabel) {
            if (g_hPoemLabelFont) { SendMessage(hLabel, WM_SETFONT, (WPARAM)g_hPoemLabelFont, TRUE); }
            SetWindowPos(hLabel, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        }
    }
}

// UI 準備函式
void PrepareUI() {
    g_assaInfo.HWND = GetCurrentProcessMainWindow();
    g_assaInfo.hProcess = GetCurrentProcess();
    if (g_assaInfo.HWND) {
        if (GetPropW(g_assaInfo.HWND, L"ParentOldWndProc") == NULL) {
            WNDPROC pOldProc = (WNDPROC)SetWindowLongPtrW(g_assaInfo.HWND, GWLP_WNDPROC, (LONG_PTR)ParentWndProc);
            if (pOldProc) { SetPropW(g_assaInfo.HWND, L"ParentOldWndProc", (HANDLE)pOldProc); }
        }
        EnumChildWindows(g_assaInfo.HWND, FindAndHookControlsProc, NULL);
        if (g_hFastWalk && g_hAutoRiddle) {
            HWND hFastWalkParent = GetParent(g_hFastWalk);
            if (hFastWalkParent) ShowWindow(hFastWalkParent, SW_HIDE);
            ShowWindow(g_hAutoRiddle, SW_HIDE);
            HWND hNewParent = GetParent(g_hAutoRiddle);
            if (hNewParent) {
                g_hInfoContainer = hNewParent;
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