// �ɮ�: Hooks.cpp
// ����: ���ɮשw�q�F�Ҧ��� Hook �禡�H�Φw�˥��̪��޿�C
//       "Hook" (���_) ������O�b�{�������Y�ӯS�w��m�ɡA
//       �j�����Ӱ���ڭ̦ۤv�g���{���X�A������A���^�h�~�����C

#include "pch.h"
#include "Hooks.h"
#include "Globals.h"
#include "helpers.h"
#include "RiddleDatabase.h" // �ޥβq����Ʈw
#include <functional>
#include <random>

// --- �R�W�Ŷ��P�`�Ʃw�q ---
// �i�W�٫�ĳ�jAddr �i�H�Ҽ{��W�� Offsets �� GameOffsets�A����T��ܳo�ǬO�O���鰾���q�C
namespace Addr {
    // �o�ǬO Assa8.0B5.exe �{�����A�ڭ̷Q�n Hook ����m�۹��Ҳհ򩳦�}�������q�C
    // �o�ǭȬO�z�L�f�V�u�{�u�� (�p x64dbg, IDA) ���R Assa �{���o�쪺�C
    //
    // �D�{�� (Assa) Hook ��������
    constexpr DWORD CompareHookOffset = 0x71972;        // �۰ʵo�ܤ���I
    constexpr DWORD AddEspHookOffset = 0x71AD5;         // �۰ʵo�ܰT���M���I
    constexpr DWORD EcxStringHookOffset = 0x7AA0B;      // �B�z "Button" ���O���I (�ڭ̥Ψ�Ĳ�o�q��)
    constexpr DWORD ParameterHookOffset = 0x77E80;      // �B�z "Let" ���O���I (�Ψӹ�{�ۭq�ܼ�)
    constexpr DWORD PrintHookOffset = 0x76269;          // �B�z "Print" ���O���I (�Ψӹ�{�p�ɾ�)
    constexpr DWORD Button2HookOffset = 0x7A472;        // �B�z "Button2" (�T�w���s) ���I
    constexpr DWORD IfitemHookOffset = 0x88AE3;         // �B�z "Ifitem" ���O���I
    constexpr DWORD CommandChainHookOffset = 0x820B2;    // �B�z "Set" ���O���I (�Ψӳ]�w�~�������Ѽ�)
    constexpr DWORD CommandChainJumpTargetOffset = 0x83E66; // Set���O���\����઺��}
    constexpr DWORD StringCopyHookOffset = 0x7EBDB;     // �B�z "Set2" ���O���I (�ΨӴ����r��)
    constexpr DWORD StringCopyTargetPtr = 0x1220;       // Set2���O��I�s�禡�����Ц�}

    // �۰ʰ��|��������
    constexpr DWORD AutoPileHookOffset = 0x6CC7E;        // �۰ʰ��|��Ĳ�o�I
    constexpr DWORD AutoPileSwitchOffset = 0xF6668;     // �۰ʰ��|�\�઺�}����}
    constexpr DWORD AutoPileJumpTargetOffset = 0x6D399; // �۰ʰ��|Hook��������^����}

    // �ؼйC�� (sa_8002a.exe) �O���鰾��
    constexpr DWORD GameTimeOffset = 0x1AD760;          // �C���ɶ�
    constexpr DWORD WindowStateOffset = 0x1696CC;       // �C���������A (�Ω�P�_NPC��ܵ��O�_�}��)
    constexpr DWORD NpcIdOffset = 0xAFA34;              // NPC ID
    constexpr DWORD WinIdOffset = 0xAFA30;              // ���� ID
}

// --- Hook ��^��}�����ܼ� ---
// �o���ܼƥΨ��x�s�C�� Hook �I��l�{���X���U�@�����O��}�C
// �b�ڭ̪� Hook �禡���槹����A�ݭn�����o�Ǧ�}�A����{�����~�򥿱`����C
// �i�W�٫�ĳ�jgCompareHookReturnAddr �i�אּ g_pCompareHookReturn (p�N��pointer)
BYTE* gCompareHookReturnAddr = nullptr;
BYTE* gAddEspHookReturnAddr = nullptr;
BYTE* gEcxStringHookReturnAddr = nullptr;
BYTE* gParameterHookReturnAddr = nullptr;
BYTE* gPrintHookReturnAddr = nullptr;
BYTE* gButton2HookReturnAddr = nullptr;
BYTE* gIfitemHookReturnAddr = nullptr;
BYTE* gCommandChainHookReturnAddr = nullptr;
BYTE* gStringCopyHookReturnAddr = nullptr;
void* g_pfnVbaStrCopy = nullptr; // �x�sSet2��l�I�s���禡����
void* gAutoPileJumpTarget = nullptr;
void* gCommandChainJumpTarget = nullptr;

// �i�u�ơj�N���ƪ� Hook �w���޿责���X�ӡA�����@�Ӧ@�Ψ禡
static bool InstallJmpHook(LPCWSTR moduleName, DWORD offset, LPVOID newFunction, size_t hookSize, LPVOID* returnAddress) {
    HMODULE hMod = GetModuleHandleW(moduleName);
    if (!hMod) {
        // �p�G����Ҳե��ѡA�i�H�Ҽ{�O�����~
        return false;
    }

    BYTE* pTarget = (BYTE*)hMod + offset;
    *returnAddress = pTarget + hookSize;

    DWORD oldProtect;
    if (VirtualProtect(pTarget, hookSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        pTarget[0] = 0xE9; // JMP ���O
        DWORD jmpOffset = (DWORD)((BYTE*)newFunction - pTarget - 5);
        *(DWORD*)(pTarget + 1) = jmpOffset;

        // �� NOP (0x90) ��R�h�l���Ŷ��A�T�O���O�����
        for (size_t i = 5; i < hookSize; ++i) {
            pTarget[i] = 0x90;
        }

        VirtualProtect(pTarget, hookSize, oldProtect, &oldProtect);
        return true;
    }
    return false;
}


// --- Let���O Hook ---
// �禡: ProcessParameterHook
// �\��: �B�z Assa �� "Let" ���O�A���ڭ̥i�H�w�q�ۤv���ܼ� (�p #�C���ɶ�)�C
// ����: �� Assa �}������� `Let` �ɡA�o�Ө禡�|�QĲ�o�C���ˬd�ܼƦW�O�_���ڭ̹w�w�q��
//       �S��r��A�p�G�O�A�N�q�C���O����Ψ�L�ӷ�Ū���������ȡA�üg�^�� Assa�C
// �ե�: �� MyParameterHook (Naked Function) �I�s�C
void ProcessParameterHook(DWORD eax_value) {
    // ... (���B�ٲ�������@�A�\�ର�r����P��������ާ@)
    if (IsBadReadPtr((void*)eax_value, sizeof(LPCWSTR))) return;
    LPCWSTR* pString = (LPCWSTR*)eax_value;
    if (IsBadReadPtr(*pString, sizeof(wchar_t))) return;

    LPCWSTR finalStringAddress = *pString;

    static const std::unordered_map<std::wstring, std::function<void(LPCWSTR)>> actionMap = {
        {L"#�t�ήɶ���Ʈt", [](LPCWSTR addr) {
            SYSTEMTIME st_now, st_year_start = { 0 };
            GetSystemTime(&st_now);
            st_year_start.wYear = st_now.wYear; st_year_start.wMonth = 1; st_year_start.wDay = 1;
            FILETIME ft_now, ft_year_start;
            SystemTimeToFileTime(&st_now, &ft_now); SystemTimeToFileTime(&st_year_start, &ft_year_start);
            ULARGE_INTEGER uli_now = {ft_now.dwLowDateTime, ft_now.dwHighDateTime};
            ULARGE_INTEGER uli_year_start = {ft_year_start.dwLowDateTime, ft_year_start.dwHighDateTime};
            ULONGLONG totalSeconds = (uli_now.QuadPart - uli_year_start.QuadPart) / 10000000;
            wchar_t buffer[64];
            swprintf(buffer, 64, L"%llu", totalSeconds);
            OverwriteStringInMemory(addr, buffer);
        }},
        {L"#�C���ɶ�", [](LPCWSTR addr) {
            if (!g_saInfo.hProcess || !g_saInfo.base) return;
            DWORD gameTime = 0;
            LPCVOID dynamicAddress = (LPCVOID)((BYTE*)g_saInfo.base + Addr::GameTimeOffset);
            if (ReadProcessMemory(g_saInfo.hProcess, dynamicAddress, &gameTime, sizeof(gameTime), NULL)) {
                wchar_t buffer[64];
                swprintf(buffer, 64, L"%u", gameTime);
                OverwriteStringInMemory(addr, buffer);
            }
        }},
        {L"#NPCID", [](LPCWSTR addr) {
            if (!g_saInfo.hProcess || !g_saInfo.base) return;
            DWORD windowState = 0;
            LPCVOID dynamicStateAddr = (LPCVOID)((BYTE*)g_saInfo.base + Addr::WindowStateOffset);
            if (ReadProcessMemory(g_saInfo.hProcess, dynamicStateAddr, &windowState, sizeof(windowState), NULL) && windowState == 1) {
                DWORD finalId = 0;
                LPCVOID dynamicIdAddr = (LPCVOID)((BYTE*)g_saInfo.base + Addr::NpcIdOffset);
                if (ReadProcessMemory(g_saInfo.hProcess, dynamicIdAddr, &finalId, sizeof(finalId), NULL)) {
                    wchar_t buffer[64];
                    swprintf(buffer, 64, L"%u", finalId);
                    OverwriteStringInMemory(addr, buffer);
                }
            }
 else { OverwriteStringInMemory(addr, L"�������}��"); }
}},
{L"#WINID", [](LPCWSTR addr) {
    if (!g_saInfo.hProcess || !g_saInfo.base) return;
    DWORD windowState = 0;
    LPCVOID dynamicStateAddr = (LPCVOID)((BYTE*)g_saInfo.base + Addr::WindowStateOffset);
    if (ReadProcessMemory(g_saInfo.hProcess, dynamicStateAddr, &windowState, sizeof(windowState), NULL) && windowState == 1) {
        DWORD finalId = 0;
        LPCVOID dynamicIdAddr = (LPCVOID)((BYTE*)g_saInfo.base + Addr::WinIdOffset);
        if (ReadProcessMemory(g_saInfo.hProcess, dynamicIdAddr, &finalId, sizeof(finalId), NULL)) {
            wchar_t buffer[64];
            swprintf(buffer, 64, L"%u", finalId);
            OverwriteStringInMemory(addr, buffer);
        }
    }
else { OverwriteStringInMemory(addr, L"�������}��"); }
}},
{L"#EV1", [](LPCWSTR addr) { wchar_t buf[16]; swprintf(buf, 16, L"%d", ev1); OverwriteStringInMemory(addr, buf); }},
{L"#EV2", [](LPCWSTR addr) { wchar_t buf[16]; swprintf(buf, 16, L"%d", ev2); OverwriteStringInMemory(addr, buf); }},
{L"#EV3", [](LPCWSTR addr) { wchar_t buf[16]; swprintf(buf, 16, L"%d", ev3); OverwriteStringInMemory(addr, buf); }},
{L"#�ü�", [](LPCWSTR addr) {
    static std::mt19937 gen(std::random_device{}());
    static std::uniform_int_distribution<> distrib(0, 10);
    wchar_t buf[16];
    swprintf(buf, 16, L"%d", distrib(gen));
    OverwriteStringInMemory(addr, buf);
}}
    };

    auto it = actionMap.find(finalStringAddress);
    if (it != actionMap.end()) {
        it->second(finalStringAddress);
    }
}
// �禡: MyParameterHook
// �\��: �ΨӸ���� ProcessParameterHook ���զX�y�� (Assembly) �{���X�C
// ����: �o�ب禡�Q�٬� "Naked Function"�A�N��sĶ�����|�������ͥ����B�~�����|(stack)�ާ@�{���X�C
//       �ڭ̻ݭn��ʫO�s�Ҧ��Ȧs�� (pushad/popad)�A�I�s C++ �禡�A�M�����^��l�{���X�C
__declspec(naked) void MyParameterHook() {
    __asm {
        lea eax, [ebp - 0x38]   // ���o�ѼƦb���|�W����}
        pushad                  // �O�s�Ҧ��q�μȦs��
        push eax                // �N�ѼƦ�}���J���|�A�@��C++�禡���Ѽ�
        call ProcessParameterHook // �I�s�ڭ̪��B�z�禡
        pop eax                 // ���Ű��|
        popad                   // ��_�Ҧ��Ȧs��
        push ecx                // �H�U�O��l�Q�ڭ��л\���{���X
        push eax
        jmp gParameterHookReturnAddr // ����^ Assa ����l����y�{
    }
}

// --- Button�T�w���O Hook ---
void ProcessButton2Hook(DWORD edx_value) {
    DWORD* pValue = (DWORD*)edx_value;
    if (!IsBadReadPtr(pValue, sizeof(DWORD)) && *pValue == 0x5B9A78BA) {
        DWORD oldProtect;
        if (VirtualProtect(pValue, sizeof(DWORD), PAGE_READWRITE, &oldProtect)) {
            *pValue = 0x5B9A786E;
            VirtualProtect(pValue, sizeof(DWORD), oldProtect, &oldProtect);
        }
    }
}
__declspec(naked) void MyButton2Hook() {
    __asm {
        mov edx, [ebp - 0x1B4]
        pushad
        push edx
        call ProcessButton2Hook
        pop edx
        popad
        push edx
        jmp gButton2HookReturnAddr
    }
}

// --- Button���O Hook (���D) ---
// �禡: ProcessEcxString
// �\��: �B�z Assa �� "Button" ���O�A�d�I�S�w�r���Ĳ�o�۰ʲq���C
// �ե�: �� MyEcxStringHook �I�s�C
void ProcessEcxString(LPCWSTR stringAddress) {
    if (stringAddress && !IsBadReadPtr(stringAddress, sizeof(wchar_t))) {
        if (wcscmp(stringAddress, L"�q�ߩҦ椧���Y�O����") == 0) {
            // �p�G�r��۲šA�N�I�s RiddleDatabase.cpp ���� QA �禡��������סC
            std::wstring finalOutput = QA();
            // �N����쪺���׼g�^�O����A�л\����Ӫ��r��C
            OverwriteStringInMemory(stringAddress, finalOutput);
        }
    }
}
__declspec(naked) void MyEcxStringHook() {
    __asm {
        mov ecx, [ebp - 0x28]
        pushad
        push ecx
        call ProcessEcxString
        pop ecx
        popad
        push 0x01
        jmp gEcxStringHookReturnAddr
    }
}


// ---Print���O Hook (�p��)---
void ProcessPrintHook(DWORD edx_value) {
    if (IsBadReadPtr((void*)edx_value, sizeof(LPCWSTR))) return;
    LPCWSTR* pString = (LPCWSTR*)edx_value;
    if (IsBadReadPtr(*pString, sizeof(wchar_t))) return;

    LPCWSTR finalStringAddress = *pString;

    if (wcscmp(finalStringAddress, L"#0000/00/00 00:00:00") == 0) {
        SYSTEMTIME lt;
        GetLocalTime(&lt);
        wchar_t timeString[64];
        swprintf(timeString, 64, L"%04d/%02d/%02d %02d:%02d:%02d", lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond);
        OverwriteStringInMemory(finalStringAddress, timeString);
    }
    else if (wcscmp(finalStringAddress, L"#�p�ɶ}�l") == 0) {
        scriptTime = time(NULL);
        OverwriteStringInMemory(finalStringAddress, L"�p�ɶ}�l");
    }
    else if (wcscmp(finalStringAddress, L"#�p�ɰ���") == 0) {
        if (scriptTime != 0) {
            time_t endTime = time(NULL);
            // �u���b scriptTime �O�@�Ӯɶ��W�ɤ~�p��t��
            if (scriptTime > 31536000) { // �קK���ƭp��
                scriptTime = endTime - scriptTime;
            }
        }
        OverwriteStringInMemory(finalStringAddress, L"�p�ɰ���");
    }
    else if (wcscmp(finalStringAddress, L"#�ɶ��t00:00:00") == 0) {
        if (scriptTime == 0) {
            OverwriteStringInMemory(finalStringAddress, L"�L�p�ɬ���");
        }
        else {
            time_t secondsToFormat = (scriptTime > 31536000) ? (time(NULL) - scriptTime) : scriptTime;
            long h = (long)(secondsToFormat / 3600);
            long m = (long)((secondsToFormat % 3600) / 60);
            long s = (long)(secondsToFormat % 60);
            wchar_t buffer[128];
            swprintf(buffer, 128, L"�g�L %d�� %d�� %d��", h, m, s);
            OverwriteStringInMemory(finalStringAddress, buffer);
        }
    }
}
__declspec(naked) void MyPrintHook() {
    __asm {
        lea edx, [ebp - 0x28]
        push ecx
        push edx
        pushad
        push edx
        call ProcessPrintHook
        pop edx
        popad
        jmp gPrintHookReturnAddr
    }
}

//  ---Ifitem���O Hook ---
__declspec(naked) void MyIfitemHook() {
    __asm {
        push ecx
        mov eax, [esp + 0x44]
        sub eax, 0x108
        mov ecx, [eax]
            mov eax, [ebp - 0x1C]
                add eax, ecx
                pop ecx
                jmp gIfitemHookReturnAddr
    }
}

//  ---set���O Hook ---
bool ProcessCommandChain(LPCWSTR keyString, LPCWSTR valueString) {
    if (IsBadReadPtr(keyString, sizeof(wchar_t)) || IsBadReadPtr(valueString, sizeof(wchar_t))) {
        return false;
    }

    int valueAsInt;
    try {
        valueAsInt = std::stoi(valueString);
    }
    catch (const std::exception&) {
        return false;
    }

    struct CustomCommandRule {
        std::wstring keyString;
        int minValue;
        int maxValue;
        DWORD CheckboxOffset;
        DWORD ValueOffset;
    };

    static const std::vector<CustomCommandRule> rules = {
        //�԰��]�m
        { L"�԰����F", 0, 5, 0xF61C6, 0xF61C7 },
        { L"�԰����F�H��", 0,101, 0xF61C6, 0xF61F5 },
        { L"�԰����F�d��", 0,101, 0xF61C6, 0xF61F6 },
        { L"�԰��ɦ�", 0, 1, 0xF61C8, 0xF61C8 },
        { L"���d�ɦ�", 0, 3, 0xF61C9, 0xF61CA },
        { L"���d�ɦ�H��", 0,101, 0xF61C9, 0xF61F9 },
        { L"���d�ɦ��d��", 0,101, 0xF61C9, 0xF61FA },
        { L"�_�����F", 0, 5, 0xF61FE, 0xF6200 },
        { L"�_�����~", 0, 1, 0xF61FF, 0xF61FF },
        //�@��]�m
        { L"���ɺ��F", 0,20, 0xF6229, 0xF622A },
        { L"���ɺ��F�H��", 0,101, 0xF6229, 0xF622B },
        { L"���ɺ��F�d��", 0,101, 0xF6229, 0xF622C },
        { L"���ɸɦ�", 0, 1, 0xF622D, 0xF622D },
        { L"���ɧޯ�", 0,26, 0xF6232, 0xF6233 },
        { L"�d���C��", 0, 1, 0xF6662, 0xF6662 },
        { L"�԰�����", 0, 1, 0xF6661, 0xF6661 },
        { L"�԰��Ա�", 0, 1, 0xF6669, 0xF6669 },
        //��Ƴ]�w
        { L"�զW��", 0, 1, 0xF53CC, 0xF53CC },
        { L"�¦W��", 0, 1, 0xF53CD, 0xF53CD },
        { L"�l��զW��", 0, 1, 0xF53B8, 0xF53B8 },
        { L"�۰ʭ簣", 0, 1, 0xF53CE, 0xF53CE },
        { L"����1", 0, 1, 0xF53B9, 0xF53B9 },
        { L"����2", 0, 1, 0xF53BA, 0xF53BA },
        { L"����3", 0, 1, 0xF53BB, 0xF53BB },
        { L"����4", 0, 1, 0xF53BC, 0xF53BC },
        { L"����5", 0, 1, 0xF53BD, 0xF53BD },
    };

    HMODULE hMod = GetModuleHandleW(L"Assa8.0B5.exe");
    if (!hMod) return false;

    for (const auto& rule : rules) {
        if (rule.keyString == keyString && valueAsInt >= rule.minValue && valueAsInt <= rule.maxValue) {
            BYTE* pCheckbox = (BYTE*)hMod + rule.CheckboxOffset;
            *pCheckbox = (valueAsInt > 0) ? 1 : 0;

            if (valueAsInt > 0 && rule.maxValue > 1 && rule.CheckboxOffset != rule.ValueOffset) {
                BYTE* pValue = (BYTE*)hMod + rule.ValueOffset;
                *pValue = valueAsInt - 1;
            }
            return true;
        }
    }
    return false;
}
__declspec(naked) void MyCommandChainHook() {
    __asm {
        pushad
        mov eax, [ebp - 0x30]
        mov ecx, [ebp - 0x1BC]
        push eax
        push ecx
        call ProcessCommandChain
        add esp, 8
        cmp al, 1
        popad
        je handle_custom_command
        mov ecx, [ebp - 0x1BC]
        jmp gCommandChainHookReturnAddr
        handle_custom_command :
        jmp dword ptr[gCommandChainJumpTarget]
    }
}

// ---set2���O (�r�����) Hook ---
void ProcessStringCopyHook(LPCWSTR stringAddress) {
    if (IsBadReadPtr(stringAddress, sizeof(wchar_t))) return;
    static const std::unordered_map<std::wstring, std::wstring> replacements = {
        {L"�M��", L"�M��"}, {L"�۰ʹq�����", L"�۰�KNPC"}, {L"�۰ʾ԰�", L"�۰ʾԤ�"},
        {L"�ֳt�԰�", L"�ֳt�Ԥ�"}, {L"�n�J�D��", L"�n���D��"}, {L"�n�J�ƾ�", L"�n���ƾ�"},
        {L"�n�J�H��", L"�n���H��"}, {L"�۰ʵn�J", L"�۰ʵn��"}, {L"�}������", L"�}������"},
        {L"�԰��ɮ�", L"�Ԥ�ɮ�"},
        // �\�����
        {L"���ʹJ��", L"�\�����"}, {L"���ʨB��", L"�\�����"}, {L"�ֳt�J��", L"�\�����"},
        {L"�ֳt����", L"�\�����"}, {L"�۰ʲq��", L"�\�����"}
    };
    auto it = replacements.find(stringAddress);
    if (it != replacements.end()) {
        OverwriteStringInMemory(stringAddress, it->second);
    }
}
__declspec(naked) void MyStringCopyHook() {
    __asm {
        pushad
        push edx
        call ProcessStringCopyHook
        pop edx
        popad
        call dword ptr[g_pfnVbaStrCopy]
            jmp gStringCopyHookReturnAddr
    }
}

// --- �۰ʰ��| (Auto Pile) Hook ---
void ProcessAutoPile() {
    HMODULE hMod = GetModuleHandleW(L"Assa8.0B5.exe");
    if (!hMod) return;
    BYTE* switchAddress = (BYTE*)hMod + Addr::AutoPileSwitchOffset;
    if (*switchAddress == 1) {
        pileCounter++;
        if (pileCounter > 10) {
            const std::vector<BYTE> pileCommand = { 0x2F, 0x00, 0x70, 0x00, 0x69, 0x00, 0x6C, 0x00, 0x65, 0x00, 0x00, 0x00 };
            WriteCommandToMemory(pileCommand);
            pileCounter = 0;
        }
    }
}
__declspec(naked) void MyAutoPileHook() {
    __asm {
        pushad
        call ProcessAutoPile
        popad
        jmp gAutoPileJumpTarget
    }
}

// -- - �۰ʵo��(Compare) Hook-- -
// �禡: ShouldDoCustomJump
// �\��: �P�_�ثe Assa �ǳƵo�e���T���O�_���ڭ̻ݭn�d�I���S����O�C
// ����: �o�ǫ��O�p�G�����o�e�|�S���ĪG�A�ݭn�ڭ��d�I�U�Өè��S���B�z�y�{�C
// �ե�: �� MyCompareHook �I�s�C
// ��^: 1 ��ܻݭn�d�I�A0 ��ܤ��ݭn�C
int ShouldDoCustomJump() {
    LPCWSTR commandString = (LPCWSTR)0x4F53D1; // �o�O Assa �����s��ݵo�e�T�����O�����}
    if (IsBadReadPtr(commandString, sizeof(wchar_t))) return 0;

    static const std::vector<std::wstring> commandsToMatch = {
        L"/status", L"/accompany", L"/eo", L"/offline on", L"/offline off",
        L"/watch", L"/encount on", L"/encount off", L"/pile", L"�e���_�I�̤��q"
    };

    for (const auto& cmd : commandsToMatch) {
        if (wcscmp(commandString, cmd.c_str()) == 0) {
            return 1; // ���ǰt�����O
        }
    }
    return 0;
}
__declspec(naked) void MyCompareHook() {
    __asm {
        pushad
        call ShouldDoCustomJump // �I�sC++�禡�A��^�ȷ|�b EAX �Ȧs����
        cmp eax, 1              // �����^�ȬO�_�� 1
        popad
        je do_custom_jump       // �p�G�O 1 (je = jump if equal)�A�N����ڭ̪��S��B�z
        mov dl, byte ptr ds : [0x4F5442] // �_�h�A�����l�{���X
        jmp gCompareHookReturnAddr      // �M����^��y�{
        do_custom_jump :
        push 0x004719C4         // �o�O�S��B�z�������}�A�� Assa �{�����O�w���\�o�e
            ret                     // ��^
    }
}

// --- �۰ʵo�ܰT���M�� Hook ---
__declspec(naked) void MyAddEspHook() {
    __asm {
        mov eax, 0x00200020
        // �ϥ� rep stosd �ӧֳt�M�ŰO����
        lea edi, ds: [0x4F53D1]
        mov ecx, 16 // 16 * 4 = 64 bytes
        rep stosd

        lea eax, [ebp - 0x7C]
        push edx
        push eax
        jmp gAddEspHookReturnAddr
    }
}

// --- ���}���`�w�˨禡 ---
// �禡: InstallAllHooks
// �\��: �I�s�Ҧ��W�ߪ� Hook �w�˨禡�A�@���ʧ����Ҧ��\�઺�����C
// �ե�: �� dllmain.cpp �b DLL_PROCESS_ATTACH �ƥ󤤩I�s�C
void InstallAllHooks() {
    // �ϥηs�����U�禡�Ӧw�˩Ҧ� Hook
    InstallJmpHook(L"Assa8.0B5.exe", Addr::CompareHookOffset, MyCompareHook, 6, (LPVOID*)&gCompareHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::AddEspHookOffset, MyAddEspHook, 5, (LPVOID*)&gAddEspHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::EcxStringHookOffset, MyEcxStringHook, 5, (LPVOID*)&gEcxStringHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::ParameterHookOffset, MyParameterHook, 5, (LPVOID*)&gParameterHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::PrintHookOffset, MyPrintHook, 5, (LPVOID*)&gPrintHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::Button2HookOffset, MyButton2Hook, 6, (LPVOID*)&gButton2HookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::IfitemHookOffset, MyIfitemHook, 6, (LPVOID*)&gIfitemHookReturnAddr);

    // --- �S�� Hook ���w�� ---
    HMODULE hMod = GetModuleHandleW(L"Assa8.0B5.exe");
    if (hMod) {
        // ���� Hook �ݭn�b�w�˫e�B�~�]�w����ؼЦ�}�Ψ��o�禡����
        gCommandChainJumpTarget = (void*)((BYTE*)hMod + Addr::CommandChainJumpTargetOffset);
        InstallJmpHook(L"Assa8.0B5.exe", Addr::CommandChainHookOffset, MyCommandChainHook, 6, (LPVOID*)&gCommandChainHookReturnAddr);

        gAutoPileJumpTarget = (void*)((BYTE*)hMod + Addr::AutoPileJumpTargetOffset);
        InstallJmpHook(L"Assa8.0B5.exe", Addr::AutoPileHookOffset, MyAutoPileHook, 5, (LPVOID*)&gAutoPileJumpTarget);

        DWORD* pFuncPtrAddr = (DWORD*)((BYTE*)hMod + Addr::StringCopyTargetPtr);
        g_pfnVbaStrCopy = (void*)*pFuncPtrAddr;
        InstallJmpHook(L"Assa8.0B5.exe", Addr::StringCopyHookOffset, MyStringCopyHook, 6, (LPVOID*)&gStringCopyHookReturnAddr);
    }
}