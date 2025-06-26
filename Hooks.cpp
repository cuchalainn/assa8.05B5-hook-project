// �ɮ�: Hooks.cpp
#include "pch.h"
#include "Hooks.h"
#include "Globals.h"
#include "helpers.h"
#include "RiddleDatabase.h"
#include "UIManager.h"
#include "MinHook.h"
#include <cstdio>
#include <string>
#include <random>
#include <time.h>
#include <algorithm>
#include <stdexcept>
#include <functional> //�i�u�ơj���F�ϥ� std::function


// --- �`�Ʃw�q ---
namespace Addr {
    // �D�{�� Hook ��������
    constexpr DWORD CompareHookOffset = 0x71972;
    constexpr DWORD AddEspHookOffset = 0x71AD5;
    constexpr DWORD EcxStringHookOffset = 0x7AA0B;
    constexpr DWORD ParameterHookOffset = 0x77E80;
    constexpr DWORD PrintHookOffset = 0x76269;
    constexpr DWORD Button2HookOffset = 0x7A472;
    constexpr DWORD IfitemHookOffset = 0x88AE3;
    constexpr DWORD CommandChainHookOffset = 0x820B2;
    constexpr DWORD CommandChainJumpTargetOffset = 0x83E66;
    constexpr DWORD StringCopyHookOffset = 0x7EBDB;
    constexpr DWORD StringCopyTargetPtr = 0x1220;
    //�D�{�� Hook �۰ʰ��| ��������
    constexpr DWORD AutoPileHookOffset = 0x6CC7E;
    constexpr DWORD AutoPileSwitchOffset = 0xF6668;
    constexpr DWORD AutoPileJumpTargetOffset = 0x6D399;
    // �ؼе{�� Hook ��������
    constexpr DWORD GameTimeOffset = 0x1AD760;
    constexpr DWORD WindowStateOffset = 0x1696CC;
    constexpr DWORD NpcIdOffset = 0xAFA34;
    constexpr DWORD WinIdOffset = 0xAFA30;
}

// --- Hook ��^��}�����ܼ� ---
BYTE* gCompareHookReturnAddr = nullptr;
BYTE* gAddEspHookReturnAddr = nullptr;
BYTE* gEcxStringHookReturnAddr = nullptr;
BYTE* gParameterHookReturnAddr = nullptr;
BYTE* gPrintHookReturnAddr = nullptr;
BYTE* gButton2HookReturnAddr = nullptr;
BYTE* gIfitemHookReturnAddr = nullptr;
BYTE* gCommandChainHookReturnAddr = nullptr;
BYTE* gStringCopyHookReturnAddr = nullptr;
void* g_pfnVbaStrCopy = nullptr;
void* gAutoPileJumpTarget = nullptr;
void* gCommandChainJumpTarget = nullptr;

// --- GDI Hook �����{���X ---
typedef BOOL(WINAPI* PFN_TEXTOUTW)(HDC, int, int, LPCWSTR, int);
PFN_TEXTOUTW pfnOriginalTextOutW = NULL;
typedef BOOL(WINAPI* PFN_EXTTEXTOUTW)(HDC, int, int, UINT, const RECT*, LPCWSTR, UINT, const INT*);
PFN_EXTTEXTOUTW pfnOriginalExtTextOutW = NULL;
typedef int(WINAPI* PFN_DRAWTEXTW)(HDC, LPCWSTR, int, LPRECT, UINT);
PFN_DRAWTEXTW pfnOriginalDrawTextW = NULL;


// �Ω��x�s�ۭqset���O�һݸ�T�����c
struct CustomCommandInfo {
    DWORD CheckboxOffset;
    DWORD ValueOffset;
};
struct CustomCommandRule {
    std::wstring keyString;
    int minValue; // �d�򪺳̤p�� (�]�t)
    int maxValue; // �d�򪺳̤j�� (�]�t)
    CustomCommandInfo action;
};

// ���U�禡�Ω�P�_�r��O�_�۲�
int ShouldDoCustomJump() {
    LPCWSTR commandString = (LPCWSTR)0x4F53D1;
    if (IsBadReadPtr(commandString, sizeof(wchar_t))) {
        return 0; // ���еL�ġA��^ 0 (false)
    }
    const wchar_t* commandsToMatch[] = {
        L"/status", L"/accompany", L"/eo", L"/offline on", L"/offline off",
        L"/watch", L"/encount on", L"/encount off", L"/pile", L"�e���_�I�̤��q"
    };
    for (const auto& cmd : commandsToMatch) {
        if (wcscmp(commandString, cmd) == 0) {
            return 1; // ���۲ū��O�A���T��^ 1 (true)
        }
    }
    return 0; // �S���A���T��^ 0 (false)
}

// --- Let���O Hook ---
void ProcessParameterHook(DWORD eax_value) {
    if (IsBadReadPtr((void*)eax_value, sizeof(LPCWSTR))) return;
    LPCWSTR* pString = (LPCWSTR*)eax_value;
    if (IsBadReadPtr(*pString, sizeof(wchar_t))) return;

    LPCWSTR finalStringAddress = *pString;
    wchar_t buffer[64];

    //�i�u�ơj�ϥ� map �N�r�������B�z�޿�A���N������ if-else�C
    // �ϥ� std::function �x�s���P���B�z�禡�C
    static std::unordered_map<std::wstring, std::function<void(LPCWSTR)>> actionMap = {
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
            if (!g_saInfo.hProcess) return;
            DWORD gameTime = 0;
            LPCVOID dynamicAddress = (LPCVOID)((BYTE*)g_saInfo.base + Addr::GameTimeOffset);
            if (ReadProcessMemory(g_saInfo.hProcess, dynamicAddress, &gameTime, sizeof(gameTime), NULL)) {
                wchar_t buffer[64];
                swprintf(buffer, 64, L"%u", gameTime);
                OverwriteStringInMemory(addr, buffer);
            }
        }},
        {L"#NPCID", [](LPCWSTR addr) {
            if (!g_saInfo.hProcess) return;
            DWORD windowState = 0;
            LPCVOID dynamicStateAddr = (LPCVOID)((BYTE*)g_saInfo.base + Addr::WindowStateOffset);
            ReadProcessMemory(g_saInfo.hProcess, dynamicStateAddr, &windowState, sizeof(windowState), NULL);
            if (windowState == 1) {
                DWORD finalId = 0;
                LPCVOID dynamicIdAddr = (LPCVOID)((BYTE*)g_saInfo.base + Addr::NpcIdOffset);
                ReadProcessMemory(g_saInfo.hProcess, dynamicIdAddr, &finalId, sizeof(finalId), NULL);
                wchar_t buffer[64];
                swprintf(buffer, 64, L"%u", finalId);
                OverwriteStringInMemory(addr, buffer);
        }
 else { OverwriteStringInMemory(addr, L"�������}��"); }
}},
{L"#WINID", [](LPCWSTR addr) {
    if (!g_saInfo.hProcess) return;
            DWORD windowState = 0;
    LPCVOID dynamicStateAddr = (LPCVOID)((BYTE*)g_saInfo.base + Addr::WindowStateOffset);
    ReadProcessMemory(g_saInfo.hProcess, dynamicStateAddr, &windowState, sizeof(windowState), NULL);
            if (windowState == 1) {
                DWORD finalId = 0;
        LPCVOID dynamicIdAddr = (LPCVOID)((BYTE*)g_saInfo.base + Addr::WinIdOffset);
        ReadProcessMemory(g_saInfo.hProcess, dynamicIdAddr, &finalId, sizeof(finalId), NULL);
        wchar_t buffer[64];
                swprintf(buffer, 64, L"%u", finalId);
        OverwriteStringInMemory(addr, buffer);
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
        it->second(finalStringAddress); // ��������� lambda �禡
    }
}
__declspec(naked) void MyParameterHook() {
    __asm {
        lea eax, [ebp - 0x38]
        pushad
        push eax
        call ProcessParameterHook
        add esp, 4
        popad
        push ecx
        push eax
        jmp gParameterHookReturnAddr
    }
}
void InstallParameterHook() {
    HMODULE hMod = GetModuleHandleW(L"Assa8.0B5.exe"); if (!hMod) return;
    BYTE* pTarget = (BYTE*)hMod + Addr::ParameterHookOffset;
    gParameterHookReturnAddr = pTarget + 5;
    DWORD oldProtect;
    VirtualProtect(pTarget, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
    pTarget[0] = 0xE9;
    DWORD offset = (DWORD)((BYTE*)&MyParameterHook - pTarget - 5);
    *(DWORD*)(pTarget + 1) = offset;
    VirtualProtect(pTarget, 5, oldProtect, &oldProtect);
}

// --- Button�T�w���O Hook ---
void ProcessButton2Hook(DWORD edx_value) {
    DWORD* pValue = (DWORD*)edx_value;
    if (IsBadReadPtr(pValue, sizeof(DWORD))) {
        return;
    }
    DWORD currentValue = *pValue;
    if (currentValue == 0x5B9A78BA) {
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
        add esp, 4
        popad
        push edx
        jmp gButton2HookReturnAddr
    }
}
void InstallButton2Hook() {
    HMODULE hMod = GetModuleHandleW(L"Assa8.0B5.exe");
    if (!hMod) return;
    BYTE* pTarget = (BYTE*)hMod + Addr::Button2HookOffset;
    gButton2HookReturnAddr = pTarget + 6;
    DWORD oldProtect;
    VirtualProtect(pTarget, 6, PAGE_EXECUTE_READWRITE, &oldProtect);
    pTarget[0] = 0xE9;
    DWORD offset = (DWORD)((BYTE*)&MyButton2Hook - pTarget - 5);
    *(DWORD*)(pTarget + 1) = offset;
    pTarget[5] = 0x90;
    VirtualProtect(pTarget, 6, oldProtect, &oldProtect);
}

// --- Button���O Hook ---
void ProcessEcxString(LPCWSTR stringAddress) {
    if (stringAddress == nullptr || IsBadReadPtr(stringAddress, sizeof(wchar_t))) {
        return;
    }
    if (wcscmp(stringAddress, L"�q�ߩҦ椧���Y�O����") == 0) {
        // �i�ק�j�I�s�s�� QA() �禡
        std::wstring finalOutput = QA();
        OverwriteStringInMemory(stringAddress, finalOutput);
    }
}
__declspec(naked) void MyEcxStringHook() {
    __asm {
        mov ecx, [ebp - 0x28]
        pushad
        push ecx
        call ProcessEcxString
        add esp, 4
        popad
        push 0x01
        jmp gEcxStringHookReturnAddr
    }
}
void InstallEcxStringHook() {
    HMODULE hMod = GetModuleHandleW(L"Assa8.0B5.exe"); if (!hMod) return;
    BYTE* pTarget = (BYTE*)hMod + Addr::EcxStringHookOffset;
    gEcxStringHookReturnAddr = pTarget + 5;
    DWORD oldProtect;
    VirtualProtect(pTarget, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
    pTarget[0] = 0xE9;
    DWORD offset = (DWORD)((BYTE*)&MyEcxStringHook - pTarget - 5);
    *(DWORD*)(pTarget + 1) = offset;
    VirtualProtect(pTarget, 5, oldProtect, &oldProtect);
}

// ---Print���O Hook ---
void ProcessPrintHook(DWORD edx_value) {
    if (IsBadReadPtr((void*)edx_value, sizeof(LPCWSTR))) return;
    LPCWSTR* pString = (LPCWSTR*)edx_value;
    if (IsBadReadPtr(*pString, sizeof(wchar_t))) return;
    LPCWSTR finalStringAddress = *pString;
    if (wcscmp(finalStringAddress, L"#0000/00/00 00:00:00") == 0) {
        // ���o��e�����ɶ�
        SYSTEMTIME lt;
        GetLocalTime(&lt);
        wchar_t timeString[64];
        swprintf(timeString, 64, L"%04d/%02d/%02d %02d:%02d:%02d",
            lt.wYear, lt.wMonth, lt.wDay,
            lt.wHour, lt.wMinute, lt.wSecond);
        OverwriteStringInMemory(finalStringAddress, timeString);
    }
    else if (wcscmp(finalStringAddress, L"#�p�ɶ}�l") == 0) {
        scriptTime = time(NULL); // �O����e�ɶ��W (��)
        OverwriteStringInMemory(finalStringAddress, L"�p�ɭp��");
    }
    else if (wcscmp(finalStringAddress, L"#�p�ɰ���") == 0) {
        if (scriptTime == 0) {
            OverwriteStringInMemory(finalStringAddress, L"�L�p�ɬ���");
        }
        else {
            time_t endTime = time(NULL);
            const time_t one_year_in_seconds = 31536000;
            // �P�_ scriptTime �s���O�ɶ��I�٬O�w�g�p��L���ɶ��t
            if (scriptTime > one_year_in_seconds) {
                scriptTime = endTime - scriptTime; // �p����x�s�ɶ��t
            }
            OverwriteStringInMemory(finalStringAddress, L"�p�ɰ���");
        }
    }
    else if (wcscmp(finalStringAddress, L"#�ɶ��t00:00:00") == 0) {
        if (scriptTime == 0) {
            OverwriteStringInMemory(finalStringAddress, L"�L�p�ɬ���");
        }
        else {
            time_t secondsToFormat = 0;
            const time_t one_year_in_seconds = 31536000;

            if (scriptTime > one_year_in_seconds) {
                // �p�ɥ�����A�ʺA�p���e�ɶ��P�}�l�ɶ����t�B
                secondsToFormat = time(NULL) - scriptTime;
            }
            else {
                // �p�ɤw����A�����ϥ��x�s���ɶ��t
                secondsToFormat = scriptTime;
            }

            // �榡�Ƭ� ��:��:��
            long h = (long)(secondsToFormat / 3600);
            long m = (long)((secondsToFormat % 3600) / 60);
            long s = (long)(secondsToFormat % 60);

            wchar_t buffer[128];
            swprintf(buffer, 128, L"�g�L%d��%d��%d��", h, m, s);
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
        add esp, 4
        popad
        jmp gPrintHookReturnAddr
    }
}
void InstallPrintHook() {
    HMODULE hMod = GetModuleHandleW(L"Assa8.0B5.exe");
    if (!hMod) return;
    BYTE* pTarget = (BYTE*)hMod + Addr::PrintHookOffset;
    gPrintHookReturnAddr = pTarget + 5;
    DWORD oldProtect;
    VirtualProtect(pTarget, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
    pTarget[0] = 0xE9; // JMP
    DWORD offset = (DWORD)((BYTE*)&MyPrintHook - pTarget - 5);
    *(DWORD*)(pTarget + 1) = offset;
    VirtualProtect(pTarget, 5, oldProtect, &oldProtect);
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
void InstallIfitemHook() {
    HMODULE hMod = GetModuleHandleW(L"Assa8.0B5.exe");
    if (!hMod) return;
    BYTE* pTarget = (BYTE*)hMod + Addr::IfitemHookOffset;
    gIfitemHookReturnAddr = pTarget + 6;
    DWORD oldProtect;
    VirtualProtect(pTarget, 6, PAGE_EXECUTE_READWRITE, &oldProtect);
    pTarget[0] = 0xE9;
    DWORD offset = (DWORD)((BYTE*)&MyIfitemHook - pTarget - 5);
    *(DWORD*)(pTarget + 1) = offset;
    pTarget[5] = 0x90;
    VirtualProtect(pTarget, 6, oldProtect, &oldProtect);
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
    static const std::vector<CustomCommandRule> rules = {
        //�԰��]�mv
        { L"�԰����F", 0, 5, { 0xF61C6, 0xF61C7 } },
        { L"�԰����F�H��", 0,101, { 0xF61C6, 0xF61F5 } },
        { L"�԰����F�d��", 0,101, { 0xF61C6, 0xF61F6 } },
        { L"�԰��ɦ�", 0, 1, { 0xF61C8, 0xF61C8 } },
        { L"���d�ɦ�", 0, 3, { 0xF61C9, 0xF61CA } },
        { L"���d�ɦ�H��", 0,101, { 0xF61C9, 0xF61F9 } },
        { L"���d�ɦ��d��", 0,101, { 0xF61C9, 0xF61FA } },
        { L"�_�����F", 0, 5, { 0xF61FE, 0xF6200 } },
        { L"�_�����~", 0, 1, { 0xF61FF, 0xF61FF } },
        //�@��]�mv
        { L"���ɺ��F", 0,20, { 0xF6229, 0xF622A } },
        { L"���ɺ��F�H��", 0,101, { 0xF6229, 0xF622B } },
        { L"���ɺ��F�d��", 0,101, { 0xF6229, 0xF622C } },
        { L"���ɸɦ�", 0, 1, { 0xF622D, 0xF622D } },
        { L"���ɧޯ�", 0,26, { 0xF6232, 0xF6233 } },
        { L"�d���C��", 0, 1, { 0xF6662, 0xF6662 } },
        { L"�԰�����", 0, 1, { 0xF6661, 0xF6661 } },
        { L"�԰��Ա�", 0, 1, { 0xF6669, 0xF6669 } },
        //��Ƴ]�w
        { L"�զW��", 0, 1, { 0xF53CC, 0xF53CC } },
        { L"�¦W��", 0, 1, { 0xF53CD, 0xF53CD } },
        { L"�l��զW��", 0, 1, { 0xF53B8, 0xF53B8 } },
        { L"�۰ʭ簣", 0, 1, { 0xF53CE, 0xF53CE } },
        { L"����1", 0, 1, { 0xF53B9, 0xF53B9 } },
        { L"����2", 0, 1, { 0xF53BA, 0xF53BA } },
        { L"����3", 0, 1, { 0xF53BB, 0xF53BB } },
        { L"����4", 0, 1, { 0xF53BC, 0xF53BC } },
        { L"����5", 0, 1, { 0xF53BD, 0xF53BD } },
    };
    for (const auto& rule : rules) {
        if (rule.keyString == keyString) {
            if (valueAsInt >= rule.minValue && valueAsInt <= rule.maxValue) {
                HMODULE hMod = GetModuleHandleW(L"Assa8.0B5.exe");
                if (!hMod) return false;
                const CustomCommandInfo& info = rule.action;
                BYTE* pCheckbox = (BYTE*)hMod + info.CheckboxOffset;
                if (valueAsInt == 0) {
                    *pCheckbox = 0;
                }
                else {
                    *pCheckbox = 1;
                    // �P�_���W�h�O�_���a�Ѽƪ����O (�̤j�� > 1)
                    if (rule.maxValue > 1) {
                        if (info.CheckboxOffset != info.ValueOffset) {
                            BYTE* pValue = (BYTE*)hMod + info.ValueOffset;
                            *pValue = valueAsInt - 1;
                        }
                    }
                }
                return true; // �B�z����
            }
        }
    }
    return false; // �S������ŦX���W�h
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
void InstallCommandChainHook() {
    HMODULE hMod = GetModuleHandleW(L"Assa8.0B5.exe");
    if (!hMod) return;
    BYTE* pTarget = (BYTE*)hMod + Addr::CommandChainHookOffset;
    gCommandChainHookReturnAddr = pTarget + 6;
    gCommandChainJumpTarget = (void*)((BYTE*)hMod + Addr::CommandChainJumpTargetOffset); // �i�ק�j
    DWORD oldProtect;
    VirtualProtect(pTarget, 6, PAGE_EXECUTE_READWRITE, &oldProtect);
    pTarget[0] = 0xE9;
    DWORD offset = (DWORD)((BYTE*)&MyCommandChainHook - pTarget - 5);
    *(DWORD*)(pTarget + 1) = offset;
    pTarget[5] = 0x90;
    VirtualProtect(pTarget, 6, oldProtect, &oldProtect);
}

//  ---set2���O Hook ---
void ProcessStringCopyHook(LPCWSTR stringAddress) {
    if (IsBadReadPtr(stringAddress, sizeof(wchar_t))) return;
    }
    // �p�G���ŦX�������A�禡���������A���������ާ@
}
__declspec(naked) void MyStringCopyHook() {
    __asm {
        pushad
        push edx
        call ProcessStringCopyHook
        add esp, 4
        popad
        call dword ptr[g_pfnVbaStrCopy]
            jmp gStringCopyHookReturnAddr
    }
}
void InstallStringCopyHook() {
    HMODULE hMod = GetModuleHandleW(L"Assa8.0B5.exe");
    if (!hMod) return;
    BYTE* pTarget = (BYTE*)hMod + Addr::StringCopyHookOffset;
    gStringCopyHookReturnAddr = pTarget + 6;
    DWORD* pFuncPtrAddr = (DWORD*)((BYTE*)hMod + Addr::StringCopyTargetPtr);
    g_pfnVbaStrCopy = (void*)*pFuncPtrAddr;
    DWORD oldProtect;
    VirtualProtect(pTarget, 6, PAGE_EXECUTE_READWRITE, &oldProtect);
    pTarget[0] = 0xE9;
    DWORD offset = (DWORD)((BYTE*)&MyStringCopyHook - pTarget - 5);
    *(DWORD*)(pTarget + 1) = offset;
    pTarget[5] = 0x90;
    VirtualProtect(pTarget, 6, oldProtect, &oldProtect);
}


// --- �۰ʰ��| (Auto Pile) Hook ---
void ProcessAutoPile() {
    HMODULE hMod = GetModuleHandleW(L"Assa8.0B5.exe");
    if (!hMod) return;
    BYTE* switchAddress = (BYTE*)hMod + Addr::AutoPileSwitchOffset;
    BYTE switchValue = *switchAddress;
    if (switchValue == 1) {
        pileCounter++;
        // �p�G�p�ƾ��j��10
        if (pileCounter > 10) {
            const std::vector<BYTE> pileCommand = {
                0x2F, 0x00, 0x70, 0x00, 0x69, 0x00, 0x6C, 0x00, 0x65, 0x00, 0x00, 0x00
            };
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
void InstallAutoPileHook() {
    HMODULE hMod = GetModuleHandleW(L"Assa8.0B5.exe");
    if (!hMod) return;
    BYTE* pTarget = (BYTE*)hMod + Addr::AutoPileHookOffset;
    gAutoPileJumpTarget = (void*)((BYTE*)hMod + Addr::AutoPileJumpTargetOffset);
    DWORD oldProtect;
    VirtualProtect(pTarget, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
    pTarget[0] = 0xE9; // JMP
    DWORD offset = (DWORD)((BYTE*)&MyAutoPileHook - pTarget - 5);
    *(DWORD*)(pTarget + 1) = offset;
    VirtualProtect(pTarget, 5, oldProtect, &oldProtect);
}


// --- �۰ʵo�� Hook ---
//���:�I�곥�~�B���|�B�}�l�J��
__declspec(naked) void MyCompareHook() {
    __asm {
        pushad
        call ShouldDoCustomJump // �I�s C++ ���U�禡�A���G�b EAX (0 �� 1)
        cmp eax, 1
        je do_custom_jump
        jmp execute_original_code
        execute_original_code :
        popad
            mov dl, byte ptr ds : [0x4F5442]
            jmp gCompareHookReturnAddr
            do_custom_jump :
        popad
            push 0x004719C4
            ret
    }
}
void InstallCompareHook() {
    HMODULE hMod = GetModuleHandleW(L"Assa8.0B5.exe"); if (!hMod) return;
    BYTE* pTarget = (BYTE*)hMod + Addr::CompareHookOffset;
    gCompareHookReturnAddr = pTarget + 6;
    DWORD oldProtect;
    VirtualProtect(pTarget, 6, PAGE_EXECUTE_READWRITE, &oldProtect);
    pTarget[0] = 0xE9;
    DWORD offset = (DWORD)((BYTE*)&MyCompareHook - pTarget - 5);
    *(DWORD*)(pTarget + 1) = offset;
    pTarget[5] = 0x90;
    VirtualProtect(pTarget, 6, oldProtect, &oldProtect);
}

// --- �۰ʵo�ܰT���M�� Hook ---
__declspec(naked) void MyAddEspHook() {
    __asm {
        mov eax, 0x00200020
        mov dword ptr ds : [0x4F53D1] , eax
        mov dword ptr ds : [0x4F53D5] , eax
        mov dword ptr ds : [0x4F53D9] , eax
        mov dword ptr ds : [0x4F53DD] , eax
        mov dword ptr ds : [0x4F53E1] , eax
        mov dword ptr ds : [0x4F53E5] , eax
        mov dword ptr ds : [0x4F53E9] , eax
        mov dword ptr ds : [0x4F53ED] , eax
        mov dword ptr ds : [0x4F53F1] , eax
        mov dword ptr ds : [0x4F53F5] , eax
        mov dword ptr ds : [0x4F53F9] , eax
        mov dword ptr ds : [0x4F53FD] , eax
        mov dword ptr ds : [0x4F5401] , eax
        mov dword ptr ds : [0x4F5405] , eax
        mov dword ptr ds : [0x4F5409] , eax
        mov dword ptr ds : [0x4F540D] , eax
        lea eax, [ebp - 0x7C]
        push edx
        push eax
        jmp gAddEspHookReturnAddr
    }
}
void InstallAddEspHook() {
    HMODULE hMod = GetModuleHandleW(L"Assa8.0B5.exe"); if (!hMod) return;
    BYTE* pTarget = (BYTE*)hMod + Addr::AddEspHookOffset;
    gAddEspHookReturnAddr = pTarget + 5;
    DWORD oldProtect;
    VirtualProtect(pTarget, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
    pTarget[0] = 0xE9;
    DWORD offset = (DWORD)((BYTE*)&MyAddEspHook - pTarget - 5);
    *(DWORD*)(pTarget + 1) = offset;
    VirtualProtect(pTarget, 5, oldProtect, &oldProtect);
}

// --- ���}���`�w�˨禡 ---
void InstallAllHooks() {
    InstallCompareHook();
    InstallAddEspHook();
    InstallEcxStringHook();
    InstallParameterHook();
    InstallAutoPileHook();
    InstallPrintHook();
    InstallButton2Hook();
    InstallIfitemHook();
    InstallCommandChainHook();
    InstallStringCopyHook();
}