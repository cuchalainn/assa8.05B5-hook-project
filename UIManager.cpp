// �ɮ�: UIManager.cpp
#include "pch.h"
#include "UIManager.h"
#include "Globals.h"
#include "helpers.h"
#include <map>

// --- ����P�R�A�ܼ� ---
HFONT g_hCustomFont = NULL;
HFONT g_hInfoBoxFont = NULL;

static bool g_areButtonsCreated = false;
static HWND g_hFastWalk = NULL;
static HWND g_hWallHack = NULL;
static HWND g_hAutoRiddle = NULL;
static HWND g_hTargetParent = NULL;

// ���s ID �w�q
enum ButtonID {
    ID_ENCOUNT_ON = 9001,
    ID_ENCOUNT_OFF,
    ID_ACCOMPANY,
    ID_ADVENTURE_GUILD,
    ID_PILE,
    ID_OFFLINE_ON
};

// --- �禡�ŧi ---
void CreateAllButtons();
void OnCustomButtonClick(int button_id);
LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ContainerWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// "�Ұʥ۾�" ���s�������{��
LRESULT CALLBACK LaunchButtonWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WNDPROC pOldProc = (WNDPROC)GetPropW(hwnd, L"OldWndProc");
    if (!pOldProc) {
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

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

// �e�������{�ǡA�M���B�z�s���s���I���ƥ�
LRESULT CALLBACK ContainerWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WNDPROC pOldProc = (WNDPROC)GetPropW(hwnd, L"ContainerOldWndProc");
    if (!pOldProc) {
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    if (uMsg == WM_COMMAND && HIWORD(wParam) == BN_CLICKED) {
        int button_id = LOWORD(wParam);
        if (button_id >= ID_ENCOUNT_ON && button_id <= ID_OFFLINE_ON) {
            OnCustomButtonClick(button_id);
            return 0;
        }
    }
    return CallWindowProcW(pOldProc, hwnd, uMsg, wParam, lParam);
}


// �D�����������{��
LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WNDPROC pOldProc = (WNDPROC)GetPropW(hwnd, L"ParentOldWndProc");
    if (!pOldProc) {
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    return CallWindowProcW(pOldProc, hwnd, uMsg, wParam, lParam);
}

// ���s�I�����֤ߥ\��
void OnCustomButtonClick(int button_id) {
    static const std::map<int, std::vector<BYTE>> commandMap = {
        { ID_ENCOUNT_ON,      { 0x2F, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x75, 0x00, 0x6E, 0x00, 0x74, 0x00, 0x20, 0x00, 0x6F, 0x00, 0x6E, 0x00, 0x00, 0x00 } },
        { ID_ENCOUNT_OFF,     { 0x2F, 0x00, 0x65, 0x00, 0x6E, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x75, 0x00, 0x6E, 0x00, 0x74, 0x00, 0x20, 0x00, 0x6F, 0x00, 0x66, 0x00, 0x66, 0x00, 0x00, 0x00 } },
        { ID_ACCOMPANY,       { 0x2F, 0x00, 0x61, 0x00, 0x63, 0x00, 0x63, 0x00, 0x6F, 0x00, 0x6D, 0x00, 0x70, 0x00, 0x61, 0x00, 0x6E, 0x00, 0x79, 0x00, 0x00, 0x00 } },
        { ID_ADVENTURE_GUILD, { 0x4D, 0x52, 0x80, 0x5F, 0x92, 0x51, 0xAA, 0x96, 0x05, 0x80, 0x4B, 0x4E, 0xF6, 0x5C, 0x00, 0x00 } },
        { ID_PILE,            { 0x2F, 0x00, 0x70, 0x00, 0x69, 0x00, 0x6C, 0x00, 0x65, 0x00, 0x00, 0x00 } },
        { ID_OFFLINE_ON,      { 0x2F, 0x00, 0x6F, 0x00, 0x66, 0x00, 0x66, 0x00, 0x6C, 0x00, 0x69, 0x00, 0x6E, 0x00, 0x65, 0x00, 0x20, 0x00, 0x6F, 0x00, 0x6E, 0x00, 0x00, 0x00 } }
    };

    auto it = commandMap.find(button_id);
    if (it != commandMap.end()) {
        WriteCommandToMemory(it->second);
    }
}

// �M���l�����A�ק��r�B�M���ª�UI�ñ��_���s
BOOL CALLBACK FindAndHookControlsProc(HWND hwnd, LPARAM lParam) {
    wchar_t text[256] = { 0 };
    GetWindowTextW(hwnd, text, 256);

    if (g_hTargetParent == NULL && wcsstr(text, L"�}����@")) {
        g_hTargetParent = GetParent(hwnd);
        if (g_hTargetParent && GetPropW(g_hTargetParent, L"ContainerOldWndProc") == NULL) {
            WNDPROC pOldProc = (WNDPROC)SetWindowLongPtrW(g_hTargetParent, GWLP_WNDPROC, (LONG_PTR)ContainerWndProc);
            if (pOldProc) {
                SetPropW(g_hTargetParent, L"ContainerOldWndProc", (HANDLE)pOldProc);
            }
        }
    }

    static const std::vector<std::pair<std::wstring, std::wstring>> mappings = {
        {L"�E���۾�", L"�Ұʥ۾�"}, {L"�۰�KNPC", L"NPC���"}, {L"�۰ʾԤ�", L"�۰ʾ԰�"},
        {L"�ֳt�Ԥ�", L"�ֳt�԰�"}, {L"�Ԥ�]�w", L"�԰��]�w"}, {L"�Ԥ�]�m", L"�԰��]�m"},
        {L"�M��", L"�M��"}, {L"�X��|�Ʋz|����", L"�X��|�Ʋz|���"}
    };
    for (const auto& map : mappings) {
        if (wcsstr(text, map.first.c_str())) {
            SetWindowTextW(hwnd, map.second.c_str());
            GetWindowTextW(hwnd, text, 256);
            break;
        }
    }

    if (wcscmp(text, L"�Ұʥ۾�") == 0) {
        wchar_t className[256] = { 0 };
        GetClassNameW(hwnd, className, 256);
        if (wcscmp(className, L"ThunderRT6CommandButton") == 0 && GetPropW(hwnd, L"OldWndProc") == NULL) {
            WNDPROC pOldProc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)LaunchButtonWndProc);
            if (pOldProc) { SetPropW(hwnd, L"OldWndProc", (HANDLE)pOldProc); }
        }
    }

    if (wcscmp(text, L"�ֳt�樫") == 0) g_hFastWalk = hwnd;
    else if (wcscmp(text, L"����樫") == 0) g_hWallHack = hwnd;
    else if (wcscmp(text, L"�۰ʲq�g") == 0) g_hAutoRiddle = hwnd;

    return TRUE;
}

// �إߩҦ��ۭq���s
void CreateAllButtons() {
    HWND hParent = g_hTargetParent ? g_hTargetParent : g_assaInfo.HWND;
    if (!hParent) return;

    HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtrW(hParent, GWLP_HINSTANCE);
    if (g_hCustomFont == NULL) {
        g_hCustomFont = CreateFontW(-MulDiv(85, GetDeviceCaps(GetDC(hParent), LOGPIXELSY), 720), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft JhengHei UI");
    }

    const int start_x = 5, start_y = 47, btn_w = 100, btn_h = 25, gap_x = 6, gap_y = 6;

    struct ButtonInfo {
        int id;
        const wchar_t* label;
        POINT pos;
    };

    ButtonInfo buttons[] = {
        { ID_ENCOUNT_ON,      L"�}�l�J��",   {start_x, start_y} },
        { ID_ENCOUNT_OFF,     L"����J��",   {start_x + btn_w + gap_x, start_y} },
        { ID_ACCOMPANY,       L"�I�곥�~",   {start_x, start_y + btn_h + gap_y} },
        { ID_ADVENTURE_GUILD, L"�_�I���|",   {start_x + btn_w + gap_x, start_y + btn_h + gap_y} },
        { ID_PILE,            L"���|�D��",   {start_x, start_y + 2 * (btn_h + gap_y)} },
        { ID_OFFLINE_ON,      L"���u����",   {start_x + btn_w + gap_x, start_y + 2 * (btn_h + gap_y)} }
    };

    for (const auto& btn : buttons) {
        HWND hButton = CreateWindowExW(0, L"BUTTON", btn.label, WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            btn.pos.x, btn.pos.y, btn_w, btn_h,
            hParent, (HMENU)btn.id, hInstance, NULL);
        if (hButton && g_hCustomFont) {
            SendMessage(hButton, WM_SETFONT, (WPARAM)g_hCustomFont, TRUE);
        }
    }
}

// UI �ǳƨ禡
void PrepareUI() {
    g_assaInfo.HWND = GetCurrentProcessMainWindow();
    if (g_assaInfo.HWND) {
        if (GetPropW(g_assaInfo.HWND, L"ParentOldWndProc") == NULL) {
            WNDPROC pOldProc = (WNDPROC)SetWindowLongPtrW(g_assaInfo.HWND, GWLP_WNDPROC, (LONG_PTR)ParentWndProc);
            if (pOldProc) { SetPropW(g_assaInfo.HWND, L"ParentOldWndProc", (HANDLE)pOldProc); }
        }

        EnumChildWindows(g_assaInfo.HWND, FindAndHookControlsProc, NULL);

        if (g_hFastWalk && g_hAutoRiddle) {
            HWND hFastWalkParent = GetParent(g_hFastWalk);
            if (hFastWalkParent) ShowWindow(hFastWalkParent, SW_HIDE);

            // �i�ץ��j�ץ����B�����r���~
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