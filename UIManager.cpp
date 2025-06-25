// �ɮ�: UIManager.cpp
#include "pch.h"
#include "UIManager.h"
#include "Globals.h"
#include "helpers.h" // �i���I�j�ޥ� helpers.h
#include "RiddleDatabase.h"

// --- �i�s�W�j�Ω��x�s�ڭ̭n�ާ@������y�`�������R�A�ܼ� ---
// 'static' ����r���o���ܼƶȦb�� .cpp �ɮפ��i��
static HWND g_hFastWalk = NULL;
static HWND g_hFastWalkParent = NULL;
static HWND g_hWallHack = NULL;
static HWND g_hAutoRiddle = NULL;
static HWND g_hAutoRiddleParent = NULL;
static HWND g_hDataDisplay = NULL;
static HWND g_hDataDisplayParent = NULL;

#define ID_BUTTON_MY_NEW_BUTTON 9004

// �W�ߥ\��禡��@
void encountON() {
    WriteCommandToMemory({ 0x2F,0x00,0x65,0x00,0x6E,0x00,0x63,0x00,0x6F,0x00,0x75,0x00,0x6E,0x00,0x74,0x00,0x20,0x00,0x6F,0x00,0x6E,0x00,0x00,0x00,0x20,0x00,0x20 });
}
void accompany() {
    WriteCommandToMemory({ 0x2F,0x00,0x61,0x00,0x63,0x00,0x63,0x00,0x6F,0x00,0x6D,0x00,0x70,0x00,0x61,0x00,0x6E,0x00,0x79,0x00,0x00,0x00,0x00,0x00,0x20,0x00 });
}

// �D�n�����B�z�{��
LRESULT CALLBACK UniversalCustomWndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    WNDPROC pOldProc = (WNDPROC)GetPropW(hwnd, L"OldWndProc");
    if (!pOldProc) {
        return DefWindowProcW(hwnd, uMsg, wParam, lParam);
    }

    if (uMsg == WM_LBUTTONUP) {
        wchar_t controlText[256] = { 0 };
        GetWindowTextW(hwnd, controlText, 256);

        if (wcscmp(controlText, L"�Ұʥ۾�") == 0) {
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
        // �i�s�W�j�B�z�ڭ̦ۭq���s���I���ƥ�
        else if (GetDlgCtrlID(hwnd) == ID_BUTTON_MY_NEW_BUTTON) {
            MessageBoxW(hwnd, L"�ۭq���s�I�����\�I", L"����", MB_OK | MB_ICONINFORMATION);
            // �z�i�H�b���B�I�s����\��A�Ҧp accompany();
            return 0;
        }
    }

    return CallWindowProcW(pOldProc, hwnd, uMsg, wParam, lParam);
}

// UI��l�ƨ禡
BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam) {
    wchar_t text[256] = { 0 };
    GetWindowTextW(hwnd, text, 256);
    if (wcscmp(text, L"�ֳt�樫") == 0) {
        g_hFastWalk = hwnd;
    }
    else if (wcscmp(text, L"����樫") == 0) {
        g_hWallHack = hwnd;
    }
    else if (wcscmp(text, L"�۰ʲq�g") == 0) { 
        g_hAutoRiddle = hwnd;
    }

    const std::vector<std::pair<std::wstring, std::wstring>> mappings = {
        {L"�E���۾�", L"�Ұʥ۾�"}, {L"�۰�KNPC", L"NPC���"}, {L"�۰ʾԤ�", L"�۰ʾ԰�"},
        {L"�ֳt�Ԥ�", L"�ֳt�԰�"}, {L"�Ԥ�]�w", L"�԰��]�w"}, {L"�Ԥ�]�m", L"�԰��]�m"},
        {L"�M��", L"�M��"},{L"�X��|�Ʋz|����", L"�X��|�Ʋz|���"}
    };
    for (const auto& map : mappings) {
        if (wcsstr(text, map.first.c_str())) {
            SetWindowTextW(hwnd, map.second.c_str());
            break;
        }
    }
    if (wcsstr(text, L"�E���۾�")) {
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
        // 1. ��l�ƥ���y�`�ܼ�
        g_hFastWalk = NULL;
        g_hWallHack = NULL;
        g_hAutoRiddle = NULL;
        //�I�s EnumChildProc �i��M���A��R�W���� g_h... �ܼ�
        EnumChildWindows(Assa_HWND, EnumChildProc, NULL);
        // --- �إߤ@�ӻP�C����ͥ~�[�@�P���s���s ---
        HWND hNewButton = CreateWindowExW(
            0,
            L"ThunderRT6CommandButton", // �i�ק�j�ϥαz�qSpy++��쪺���T���O�W��
            L"���ի��s",                 // ���s���D
            WS_CHILD | WS_VISIBLE | WS_TABSTOP, // �i�ק�j�ϥαqSpy++�[��쪺���T�˦�
            10, 10,                     // X, Y �y��
            120, 30,                    // �e, ��
            Assa_HWND,                  // �������O�D�{������
            (HMENU)ID_BUTTON_MY_NEW_BUTTON, // �N�ڭ̩w�q��ID�ᤩ����
            GetModuleHandle(NULL),
            NULL
        );

        // ���o�ӷs���s���W�ڭ̪��T���B�z�{�ǡA�Ϩ��ƥ\��
        if (hNewButton) {
            WNDPROC pOldProc = (WNDPROC)SetWindowLongPtrW(hNewButton, GWLP_WNDPROC, (LONG_PTR)UniversalCustomWndProc);
            if (pOldProc) {
                SetPropW(hNewButton, L"OldWndProc", (HANDLE)pOldProc);
            }
        }

        //�M��������A�ھڰ���G����Ҧ�UI�ק�
        if (g_hFastWalk && g_hAutoRiddle) { // �T�O�֤ߥؼФw���

            //���áu�ֳt�樫�v�����e��
            HWND hFastWalkParent = GetParent(g_hFastWalk);
            if (hFastWalkParent) ShowWindow(hFastWalkParent, SW_HIDE);

            //���áu�۰ʲq�g�v���s����
            ShowWindow(g_hAutoRiddle, SW_HIDE);

            // ���o�u�۰ʲq�g�v�����e���@�����ʪ��ؼЮe��
            HWND hNewParent = GetParent(g_hAutoRiddle);
            if (hNewParent) {

                //���ʡu�ֳt�樫�v
                SetParent(g_hFastWalk, hNewParent);
                ShowWindow(g_hFastWalk, SW_SHOW); // �T�O���i��
                MoveWindow(g_hFastWalk, 108, 22, 102, 20, TRUE); // �i�ۦ�վ�e��

                //���ʡu����樫�v
                if (g_hWallHack) {
                    SetParent(g_hWallHack, hNewParent);
                    ShowWindow(g_hWallHack, SW_SHOW);
                    MoveWindow(g_hWallHack, 5, 69, 102, 20, TRUE);
                }
            }
        }
    }
}