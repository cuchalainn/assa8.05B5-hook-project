// �ɮ�: UIManager.cpp
#include "pch.h"
#include "UIManager.h"
#include "Globals.h"
#include "helpers.h"

// �����ܼƩw�q
HFONT g_hCustomFont = NULL;
HFONT g_hInfoBoxFont = NULL;

// �����ϥΪ��R�A�ܼ�
static bool g_areButtonsCreated = false;
static HWND g_hFastWalk = NULL;
static HWND g_hWallHack = NULL;
static HWND g_hAutoRiddle = NULL;
static HWND g_hTargetParent = NULL; // �Ω�s��s���s�����e���y�`

// ���s ID �w�q
#define ID_BUTTON_1 9001
#define ID_BUTTON_2 9002
#define ID_BUTTON_3 9003
#define ID_BUTTON_4 9004
#define ID_BUTTON_5 9005
#define ID_BUTTON_6 9006
static HWND g_hDiceButtons[6] = { NULL };

// �禡�ŧi
void CreateAllButtons();
void autosay(int button_id);
LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

// �M�� "�Ұʥ۾�" ���s�]�p�������{��
LRESULT CALLBACK LaunchButtonWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WNDPROC pOldProc = (WNDPROC)GetPropW(hwnd, L"OldWndProc");
    if (!pOldProc) {
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    if (uMsg == WM_LBUTTONUP) {
        LRESULT result = CallWindowProcW(pOldProc, hwnd, uMsg, wParam, lParam);
        Sleep(300); // ���ݥ۾��ɥN�{���Ұ�
        // ��s�ؼе{����T
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
        // �p�G���s�|���إߡA�h�I�s�إߨ禡
        if (!g_areButtonsCreated) {
            CreateAllButtons();
            g_areButtonsCreated = true;
        }
        return result;
    }
    return CallWindowProcW(pOldProc, hwnd, uMsg, wParam, lParam);
}

// �����b�D�����W�������{�ǡA�Ω󱵦��ۭq���s���I������
LRESULT CALLBACK ParentWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WNDPROC pOldProc = (WNDPROC)GetPropW(hwnd, L"ParentOldWndProc");
    if (!pOldProc) {
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }
    if (uMsg == WM_COMMAND) {
        if (HIWORD(wParam) == BN_CLICKED) { // �P�_�O�_�����s�I���ƥ�
            int button_id = LOWORD(wParam);
            if (button_id >= ID_BUTTON_1 && button_id <= ID_BUTTON_6) {
                autosay(button_id); // �I�s�֤ߥ\��禡
                return 0; // �����w�B�z
            }
        }
    }
    return CallWindowProcW(pOldProc, hwnd, uMsg, wParam, lParam);
}

// ���s�\�઺�֤ߨ禡�A�t�d�ק�O���餤���R�O
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
    // �N�R�O�g�J�O����
    LPVOID stringAddress = (LPVOID)((BYTE*)hMod + 0xF53D1);
    DWORD oldProtect;
    BYTE clearBuffer[40] = { 0 };
    if (VirtualProtect(stringAddress, sizeof(clearBuffer), PAGE_EXECUTE_READWRITE, &oldProtect)) {
        memcpy(stringAddress, clearBuffer, sizeof(clearBuffer)); // ���M��
        if (!commandBytes.empty()) {
            memcpy(stringAddress, commandBytes.data(), commandBytes.size()); // �A�g�J
        }
        VirtualProtect(stringAddress, sizeof(clearBuffer), oldProtect, &oldProtect);
    }
}

// �M���l�����A�ק��r�ñ��_�S�w���s
BOOL CALLBACK FindAndHookControlsProc(HWND hwnd, LPARAM lParam) {
    wchar_t text[256] = { 0 };
    GetWindowTextW(hwnd, text, 256);

    // �M��ؼЮe��
    if (g_hTargetParent == NULL && wcsstr(text, L"�}����@")) {
        g_hTargetParent = GetParent(hwnd);
    }
    // �ק��R�A��r
    const std::vector<std::pair<std::wstring, std::wstring>> mappings = {
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
    // ���_�u�Ұʥ۾��v���s
    if (wcscmp(text, L"�Ұʥ۾�") == 0) {
        wchar_t className[256] = { 0 };
        GetClassNameW(hwnd, className, 256);
        if (wcscmp(className, L"ThunderRT6CommandButton") == 0) {
            if (GetPropW(hwnd, L"OldWndProc") == NULL) {
                WNDPROC pOldProc = (WNDPROC)SetWindowLongPtrW(hwnd, GWLP_WNDPROC, (LONG_PTR)LaunchButtonWndProc);
                if (pOldProc) { SetPropW(hwnd, L"OldWndProc", (HANDLE)pOldProc); }
            }
        }
    }
    // �x�s��L�ݭn�ާ@������y�`
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
    // �إ߫��s�r��
    if (g_hCustomFont == NULL) {
        g_hCustomFont = CreateFontW(-MulDiv(85, GetDeviceCaps(GetDC(hParent), LOGPIXELSY), 720), 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_SWISS, L"Microsoft JhengHei UI");
    }
    // �w�q���s�G��
    const int start_x = 5, start_y = 47, btn_w = 100, btn_h = 25, gap_x = 6, gap_y = 6;
    POINT positions[6] = { {start_x, start_y}, {start_x + btn_w + gap_x, start_y}, {start_x, start_y + btn_h + gap_y}, {start_x + btn_w + gap_x, start_y + btn_h + gap_y}, {start_x, start_y + 2 * (btn_h + gap_y)}, {start_x + btn_w + gap_x, start_y + 2 * (btn_h + gap_y)} };
    const int ids[6] = { ID_BUTTON_1, ID_BUTTON_2, ID_BUTTON_3, ID_BUTTON_4, ID_BUTTON_5, ID_BUTTON_6 };
    const wchar_t* labels[6] = { L"�}�l�J��", L"����J��", L"�I�곥�~", L"�_�I���|", L"���|�D��", L"���u����" };

    // �j��إ߫��s
    for (int i = 0; i < 6; ++i) {
        g_hDiceButtons[i] = CreateWindowExW(0, L"BUTTON", labels[i], WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, positions[i].x, positions[i].y, btn_w, btn_h, hParent, (HMENU)ids[i], hInstance, NULL);
        if (g_hDiceButtons[i]) {
            if (g_hCustomFont) { SendMessage(g_hDiceButtons[i], WM_SETFONT, (WPARAM)g_hCustomFont, TRUE); }
            SetWindowPos(g_hDiceButtons[i], HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
        }
    }
}

// DLL �`�J����檺 UI �ǳƨ禡
void PrepareUI() {
    g_assaInfo.HWND = GetCurrentProcessMainWindow();
    if (g_assaInfo.HWND) {
        // ���_�D�����H��������
        if (GetPropW(g_assaInfo.HWND, L"ParentOldWndProc") == NULL) {
            WNDPROC pOldProc = (WNDPROC)SetWindowLongPtrW(g_assaInfo.HWND, GWLP_WNDPROC, (LONG_PTR)ParentWndProc);
            if (pOldProc) { SetPropW(g_assaInfo.HWND, L"ParentOldWndProc", (HANDLE)pOldProc); }
        }
        // �M���÷ǳ� UI
        EnumChildWindows(g_assaInfo.HWND, FindAndHookControlsProc, NULL);
        // ���s�G���¦� UI
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