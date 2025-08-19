// 檔案: UIManager.cpp
#include "pch.h"
#include "UIManager.h"
#include "Globals.h"
#include "helpers.h"

// --- 全域與靜態變數 ---
HFONT g_hCustomFont = NULL;
HFONT g_hSnapButtonFont = NULL;
HFONT g_hPoemLabelFont = NULL;

static bool g_areButtonsCreated = false;
static HWND g_hTargetParent = NULL;
static HWND g_hInfoContainer = NULL;

// 佈局錨點
static HWND g_hAnchorTop = NULL;
static HWND g_hAnchorBottom = NULL;
static HWND g_hShieldPlayer = NULL;
static HWND g_hSuperRun = NULL;

// 待搬移的UI元件
static HWND g_hFastWalk = NULL;
static HWND g_hWallHack = NULL;
static HWND g_hAutoRiddle = NULL;

// 被替換的原始按鈕資訊
static HWND g_hMapDisplayButton = NULL;
static HWND g_hMapDisplayParent = NULL;
static RECT g_mapDisplayRect = { 0 };
static HWND g_hHomePageButton = NULL;
static HWND g_hHomePageButtonParent = NULL;
static RECT g_homePageButtonRect = { 0 };
static HWND g_hChildSnapButton = NULL;

// 狀態旗標
static bool g_isPilingOn = false;
static bool g_isOfflineOn = false;
static bool g_isWindowSnapping = false;
static UINT_PTR g_snapTimerId = 0;
static bool g_isChildWindowSnapping = false;
static UINT_PTR g_childSnapTimerId = 0;
static HWND g_hChildWindow = NULL;

// 按鈕與計時器ID
enum ButtonID {
    ID_ENCOUNT_ON = 9001, ID_ENCOUNT_OFF, ID_ACCOMPANY,
    ID_ADVENTURE_GUILD, ID_PILE, ID_OFFLINE_ON,
    ID_WINDOW_SNAP, ID_CHILD_WINDOW_SNAP
};
static constexpr UINT_PTR TIMER_ID_MAIN_SNAP = 1;
static constexpr UINT_PTR TIMER_ID_CHILD_SNAP = 2;

// --- 函式宣告 ---
void CreateAllButtons();
void OnCustomButtonClick(int button_id, HWND hButton);
LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK CustomButtonProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK LaunchButtonWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
VOID CALLBACK SnapWindowTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
VOID CALLBACK SnapChildWindowTimerProc(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
BOOL CALLBACK FindAndHookControlsProc(HWND hwnd, LPARAM lParam);


// "啟動石器" 按鈕的視窗程序 (所有創建動作的起點)
LRESULT CALLBACK LaunchButtonWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WNDPROC pOldProc = (WNDPROC)GetPropW(hwnd, L"OldWndProc");
    if (!pOldProc) { return DefWindowProcW(hwnd, uMsg, wParam, lParam); }

    if (uMsg == WM_LBUTTONUP) {
        LRESULT result = CallWindowProcW(pOldProc, hwnd, uMsg, wParam, lParam);
        bool success = false;
        for (int i = 0; i < 20; ++i) {
            Sleep(250);
            // 【關鍵修改】使用新的單一函式來獲取 PID 和 HWND
            if (ReadGameHandles()) {
                // 此時，g_saInfo.PID 和 g_saInfo.HWND 都已經被成功賦值
                // 直接使用已獲取的 PID 和 HWND 進行後續操作
                g_saInfo.base = GetRemoteModuleHandle(g_saInfo.PID, L"sa_8002a.exe");
                if (g_saInfo.hProcess) CloseHandle(g_saInfo.hProcess);
                g_saInfo.hProcess = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE | PROCESS_QUERY_INFORMATION, FALSE, g_saInfo.PID);

                if (g_saInfo.base && g_saInfo.hProcess) {
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
VOID CALLBACK SnapWindowTimerProc(HWND, UINT, UINT_PTR, DWORD) {
    if (g_assaInfo.HWND && IsWindow(g_assaInfo.HWND) && g_saInfo.HWND && IsWindow(g_saInfo.HWND)) {
        RECT gameRect, assaRect;
        GetWindowRect(g_saInfo.HWND, &gameRect);
        GetWindowRect(g_assaInfo.HWND, &assaRect);
        const int targetX = gameRect.right;
        const int targetY = gameRect.top;
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
VOID CALLBACK SnapChildWindowTimerProc(HWND, UINT, UINT_PTR, DWORD) {
    if (g_hChildWindow && IsWindow(g_hChildWindow) && g_saInfo.HWND && IsWindow(g_saInfo.HWND)) {
        RECT gameRect, childRect;
        GetWindowRect(g_saInfo.HWND, &gameRect);
        GetWindowRect(g_hChildWindow, &childRect);
        int childWidth = childRect.right - childRect.left;
        const int targetX = gameRect.right - childWidth;
        const int targetY = gameRect.bottom;
        if (childRect.left != targetX || childRect.top != targetY) {
            SetWindowPos(g_hChildWindow, NULL, targetX, targetY, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
        }
    }
    else {
        if (g_childSnapTimerId != 0) {
            KillTimer(NULL, g_childSnapTimerId);
            g_childSnapTimerId = 0;
            g_isChildWindowSnapping = false;
            g_hChildWindow = NULL;
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
        { ID_ENCOUNT_OFF,{ 0x2F, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x75, 0x00, 0x6E, 0x00, 0x74, 0x00, 0x20, 0x00, 0x6F, 0x00, 0x66, 0x00, 0x66, 0x00, 0x00, 0x00 } },
        { ID_ACCOMPANY,  { 0x2F, 0x00, 0x61, 0x00, 0x63, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6D, 0x00, 0x70, 0x00, 0x61, 0x00, 0x6E, 0x00, 0x79, 0x00, 0x00, 0x00 } },
        { ID_ADVENTURE_GUILD, { 0x4D, 0x52, 0x80, 0x5F, 0x92, 0x51, 0xAA, 0x96, 0x05, 0x80, 0x4B, 0x4E, 0xF6, 0x5C, 0x00, 0x00 } },
    };
    const std::vector<BYTE> pileOnCmd = { 0x2F, 0x00, 0x70, 0x00, 0x69, 0x00, 0x6C, 0x00, 0x65, 0x00, 0x20, 0x00, 0x6F, 0x00, 0x6E, 0x00, 0x00, 0x00 };
    const std::vector<BYTE> pileOffCmd = { 0x2F, 0x00, 0x70, 0x00, 0x69, 0x00, 0x6C, 0x00, 0x65, 0x00, 0x20, 0x00, 0x6F, 0x00, 0x66, 0x00, 0x66, 0x00, 0x00, 0x00 };
    const std::vector<BYTE> offlineOnCmd = { 0x2F, 0x00, 0x6F, 0x00, 0x66, 0x00, 0x66, 0x00, 0x6C, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x65, 0x00, 0x20, 0x00, 0x6F, 0x00, 0x6E, 0x00, 0x00, 0x00 };
    const std::vector<BYTE> offlineOffCmd = { 0x2F, 0x00, 0x6F, 0x00, 0x66, 0x00, 0x66, 0x00, 0x6C, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x65, 0x00, 0x20, 0x00, 0x6F, 0x00, 0x66, 0x00, 0x66, 0x00, 0x00, 0x00 };

    switch (button_id) {
    case ID_PILE:
        g_isPilingOn = !g_isPilingOn;
        WriteCommandToMemory(g_isPilingOn ? pileOnCmd : pileOffCmd);
        SetWindowTextW(hButton, g_isPilingOn ? L"取消堆疊" : L"開啟堆疊");
        break;
    case ID_OFFLINE_ON:
        g_isOfflineOn = !g_isOfflineOn;
        WriteCommandToMemory(g_isOfflineOn ? offlineOnCmd : offlineOffCmd);
        SetWindowTextW(hButton, g_isOfflineOn ? L"取消掛機" : L"開啟掛機");
        break;
    case ID_WINDOW_SNAP:
        g_isWindowSnapping = !g_isWindowSnapping;
        if (g_isWindowSnapping) {
            if (!g_saInfo.HWND || !IsWindow(g_saInfo.HWND)) {
                MessageBoxW(g_assaInfo.HWND, L"尚未啟動或綁定遊戲，無法吸附！", L"提示", MB_OK | MB_ICONINFORMATION);
                g_isWindowSnapping = false; return;
            }
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
                g_isChildWindowSnapping = false; return;
            }
            SetWindowTextW(hButton, L"子窗分離");
            SnapChildWindowTimerProc(NULL, 0, 0, 0);
            g_childSnapTimerId = SetTimer(NULL, TIMER_ID_CHILD_SNAP, 100, SnapChildWindowTimerProc);
        }
        else {
            SetWindowTextW(hButton, L"子窗吸附");
            if (g_childSnapTimerId != 0) { KillTimer(NULL, g_childSnapTimerId); g_childSnapTimerId = 0; g_hChildWindow = NULL; }
        }
        break;
    default:
        auto it = commandMap.find(button_id);
        if (it != commandMap.end()) { WriteCommandToMemory(it->second); }
        break;
    }
}

// 遍歷子視窗，只負責掃描、記錄和子類化
BOOL CALLBACK FindAndHookControlsProc(HWND hwnd, LPARAM) {
    wchar_t text[256] = { 0 };
    GetWindowTextW(hwnd, text, 256);

    if (g_hTargetParent == NULL && wcsstr(text, L"腳本制作")) g_hTargetParent = GetParent(hwnd);
    if (wcsstr(text, L"加速:")) g_hAnchorTop = hwnd;
    if (wcscmp(text, L"自動戰斗") == 0) g_hAnchorBottom = GetParent(hwnd);
    if (wcscmp(text, L"快速行走") == 0) g_hFastWalk = hwnd;
    if (wcscmp(text, L"穿牆行走") == 0) g_hWallHack = hwnd;
    if (wcscmp(text, L"自動猜迷") == 0) g_hAutoRiddle = hwnd;
    if (wcscmp(text, L"屏蔽人物") == 0) g_hShieldPlayer = hwnd;
    if (wcsstr(text, L"超級飛奔")) g_hSuperRun = hwnd;

    if (g_hMapDisplayButton == NULL && wcscmp(text, L"地圖顯示") == 0) {
        g_hMapDisplayButton = hwnd;
        g_hMapDisplayParent = GetParent(hwnd);
        GetWindowRect(hwnd, &g_mapDisplayRect);
        MapWindowPoints(NULL, g_hMapDisplayParent, (LPPOINT)&g_mapDisplayRect, 2);
        ShowWindow(hwnd, SW_HIDE);
    }
    if (g_hHomePageButton == NULL && wcscmp(text, L"繁化主頁") == 0) {
        g_hHomePageButton = hwnd;
        g_hHomePageButtonParent = GetParent(hwnd);
        GetWindowRect(hwnd, &g_homePageButtonRect);
        MapWindowPoints(NULL, g_hHomePageButtonParent, (LPPOINT)&g_homePageButtonRect, 2);
        ShowWindow(hwnd, SW_HIDE);
    }

    static const std::vector<std::pair<std::wstring, std::wstring>> mappings = {
        {L"激活石器", L"啟動石器"}, {L"自動KNPC", L"NPC對戰"}, {L"自動戰斗", L"自動戰鬥"},
        {L"快速戰斗", L"快速戰鬥"}, {L"戰斗設定", L"戰鬥設定"}, {L"戰斗設置", L"戰鬥設置"},
        {L"決斗", L"決鬥"}, {L"合成|料理|精鏈", L"合成|料理|修復|鑲嵌"},{L"腳本制作", L"腳本製作"},
    };
    for (const auto& map : mappings) {
        if (wcsstr(text, map.first.c_str())) {
            SetWindowTextW(hwnd, map.second.c_str());
            break;
        }
    }

    GetWindowTextW(hwnd, text, 256);
    if (wcscmp(text, L"啟動石器") == 0) {
        wchar_t className[256] = { 0 }; GetClassNameW(hwnd, className, 256);
        if (wcscmp(className, L"ThunderRT6CommandButton") == 0 && GetPropW(hwnd, L"OldWndProc") == NULL) {
            WNDPROC pOldProc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)LaunchButtonWndProc);
            if (pOldProc) { SetPropW(hwnd, L"OldWndProc", (HANDLE)pOldProc); }
        }
    }
    return TRUE;
}


// UI 準備函式 (在獨立執行緒中被呼叫)
void PrepareUI() {
    g_assaInfo.HWND = GetCurrentProcessMainWindow();
    if (!g_assaInfo.HWND) return;

    if (GetPropW(g_assaInfo.HWND, L"ParentOldWndProc") == NULL) {
        WNDPROC pOldProc = (WNDPROC)SetWindowLongPtrW(g_assaInfo.HWND, GWLP_WNDPROC, (LONG_PTR)ParentWndProc);
        if (pOldProc) { SetPropW(g_assaInfo.HWND, L"ParentOldWndProc", (HANDLE)pOldProc); }
    }

    // 這裡只負責掃描和搬移，不創建任何新 UI
    EnumChildWindows(g_assaInfo.HWND, FindAndHookControlsProc, NULL);

    if (g_hFastWalk && g_hAutoRiddle) {
        HWND hFastWalkParent = GetParent(g_hFastWalk);
        if (hFastWalkParent) ShowWindow(hFastWalkParent, SW_HIDE);

        HWND hNewParent = GetParent(g_hAutoRiddle);
        if (hNewParent) {
            g_hInfoContainer = hNewParent;

            RECT autoRiddleRect;
            GetWindowRect(g_hAutoRiddle, &autoRiddleRect);
            MapWindowPoints(NULL, hNewParent, (LPPOINT)&autoRiddleRect, 2);
            SetParent(g_hFastWalk, hNewParent);
            MoveWindow(g_hFastWalk, autoRiddleRect.left, autoRiddleRect.top,
                autoRiddleRect.right - autoRiddleRect.left,
                autoRiddleRect.bottom - autoRiddleRect.top, TRUE);
            ShowWindow(g_hFastWalk, SW_SHOW);
            ShowWindow(g_hAutoRiddle, SW_HIDE);

            if (g_hWallHack && g_hShieldPlayer && g_hSuperRun) {
                RECT shieldPlayerRect, superRunRect, wallHackRect;
                GetWindowRect(g_hShieldPlayer, &shieldPlayerRect);
                GetWindowRect(g_hSuperRun, &superRunRect);
                GetWindowRect(g_hWallHack, &wallHackRect);
                MapWindowPoints(NULL, hNewParent, (LPPOINT)&shieldPlayerRect, 2);
                MapWindowPoints(NULL, hNewParent, (LPPOINT)&superRunRect, 2);
                SetParent(g_hWallHack, hNewParent);
                MoveWindow(g_hWallHack, shieldPlayerRect.left, superRunRect.top,
                    wallHackRect.right - wallHackRect.left, wallHackRect.bottom - wallHackRect.top, TRUE);
                ShowWindow(g_hWallHack, SW_SHOW);
            }
        }
    }
}


// 建立所有自訂UI元素 (此函式在「啟動石器」後被呼叫)
void CreateAllButtons() {
    auto subclassButton = [](HWND hButton) {
        if (hButton && GetPropW(hButton, L"OldButtonWndProc") == NULL) {
            WNDPROC pOldProc = (WNDPROC)SetWindowLongPtrW(hButton, GWLP_WNDPROC, (LONG_PTR)CustomButtonProc);
            if (pOldProc) { SetPropW(hButton, L"OldButtonWndProc", (HANDLE)pOldProc); }
        }
        };

    // Part 1: 左上角六個主要功能按鈕
    HWND hButtonParent = g_hTargetParent ? g_hTargetParent : g_assaInfo.HWND;
    if (!hButtonParent) return;
    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtrW(hButtonParent, GWLP_HINSTANCE);
    if (g_hCustomFont == NULL) {
        g_hCustomFont = CreateFontW(-MulDiv(85, GetDeviceCaps(GetDC(hButtonParent), LOGPIXELSY), 720), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft JhengHei UI");
    }

    int start_y = 47, btn_h = 25;
    const int gap_y = 5;
    if (g_hAnchorTop && g_hAnchorBottom) {
        RECT topRect, bottomRect;
        GetWindowRect(g_hAnchorTop, &topRect);
        GetWindowRect(g_hAnchorBottom, &bottomRect);
        MapWindowPoints(NULL, hButtonParent, (LPPOINT)&topRect, 2);
        MapWindowPoints(NULL, hButtonParent, (LPPOINT)&bottomRect, 2);
        int top_margin = 4, bottom_margin = 6;
        start_y = topRect.bottom + top_margin;
        int available_height = bottomRect.top - start_y - bottom_margin;
        btn_h = (available_height - (2 * gap_y)) / 3;
    }

    RECT parentClientRect; GetClientRect(hButtonParent, &parentClientRect);
    const int total_padding = 16, gap_x = 6;
    const int available_width = parentClientRect.right - parentClientRect.left - total_padding;
    const int btn_w = (available_width - gap_x) / 2;
    const int start_x = parentClientRect.left + (total_padding / 2);

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
        if (hButton) { SendMessage(hButton, WM_SETFONT, (WPARAM)g_hCustomFont, TRUE); subclassButton(hButton); }
    }

    // Part 2: 右下角兩個吸附按鈕
    if (g_hMapDisplayParent && g_hHomePageButtonParent) {
        if (g_hSnapButtonFont == NULL) {
            g_hSnapButtonFont = CreateFontW(-MulDiv(90, GetDeviceCaps(GetDC(g_hMapDisplayParent), LOGPIXELSY), 720), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft JhengHei UI");
        }

        int map_width = g_mapDisplayRect.right - g_mapDisplayRect.left;
        int map_height = g_mapDisplayRect.bottom - g_mapDisplayRect.top;
        HWND hSnapButton = CreateWindowExW(0, L"BUTTON", L"視窗吸附", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            g_mapDisplayRect.left, g_mapDisplayRect.top, map_width, map_height,
            g_hMapDisplayParent, (HMENU)ID_WINDOW_SNAP, (HINSTANCE)GetWindowLongPtrW(g_hMapDisplayParent, GWLP_HINSTANCE), NULL);
        if (hSnapButton) { SendMessage(hSnapButton, WM_SETFONT, (WPARAM)g_hSnapButtonFont, TRUE); subclassButton(hSnapButton); }

        int home_width = g_homePageButtonRect.right - g_homePageButtonRect.left;
        int home_height = g_homePageButtonRect.bottom - g_homePageButtonRect.top;
        g_hChildSnapButton = CreateWindowExW(0, L"BUTTON", L"子窗吸附", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            g_homePageButtonRect.left, g_homePageButtonRect.top, home_width, home_height,
            g_hHomePageButtonParent, (HMENU)ID_CHILD_WINDOW_SNAP, (HINSTANCE)GetWindowLongPtrW(g_hHomePageButtonParent, GWLP_HINSTANCE), NULL);
        if (g_hChildSnapButton) { SendMessage(g_hChildSnapButton, WM_SETFONT, (WPARAM)g_hSnapButtonFont, TRUE); subclassButton(g_hChildSnapButton); }
    }

    // Part 3: 創建詩句
    if (g_hMapDisplayParent) {
        // 【關鍵修正】使用百分比進行最終佈局
        RECT poemParentRect;
        GetClientRect(g_hMapDisplayParent, &poemParentRect);

        int parentWidth = poemParentRect.right - poemParentRect.left;
        int parentHeight = poemParentRect.bottom - poemParentRect.top;

        int labelWidth = static_cast<int>(parentWidth * 0.95);
        int labelHeight = static_cast<int>(parentHeight * 0.095);
        int labelX = static_cast<int>(parentWidth * 0.025);
        int labelY = static_cast<int>(parentHeight * 0.85);

        HINSTANCE poem_hInstance = (HINSTANCE)GetWindowLongPtrW(g_hMapDisplayParent, GWLP_HINSTANCE);
        if (g_hPoemLabelFont == NULL) {
            g_hPoemLabelFont = CreateFontW(-MulDiv(80, GetDeviceCaps(GetDC(g_hMapDisplayParent), LOGPIXELSY), 720), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft JhengHei UI");
        }

        HWND hLabel = CreateWindowExW(0, L"STATIC", L"祝你們旅途愉快\r\n  耳畔常有陽光\r\n    直至夕陽西下  by 舞丑",
            WS_CHILD | WS_VISIBLE | SS_LEFT,
            labelX, labelY, labelWidth, labelHeight,
            g_hMapDisplayParent, NULL, poem_hInstance, NULL);
        if (hLabel) {
            SendMessage(hLabel, WM_SETFONT, (WPARAM)g_hPoemLabelFont, TRUE);
            SetWindowPos(hLabel, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        }
    }
}