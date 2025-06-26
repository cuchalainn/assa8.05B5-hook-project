// 檔案: Hooks.cpp
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
#include <functional> //【優化】為了使用 std::function


// --- 常數定義 ---
namespace Addr {
    // 主程式 Hook 相關偏移
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
    //主程式 Hook 自動堆疊 相關偏移
    constexpr DWORD AutoPileHookOffset = 0x6CC7E;
    constexpr DWORD AutoPileSwitchOffset = 0xF6668;
    constexpr DWORD AutoPileJumpTargetOffset = 0x6D399;
    // 目標程式 Hook 相關偏移
    constexpr DWORD GameTimeOffset = 0x1AD760;
    constexpr DWORD WindowStateOffset = 0x1696CC;
    constexpr DWORD NpcIdOffset = 0xAFA34;
    constexpr DWORD WinIdOffset = 0xAFA30;
}

// --- Hook 返回位址全域變數 ---
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

// --- GDI Hook 相關程式碼 ---
typedef BOOL(WINAPI* PFN_TEXTOUTW)(HDC, int, int, LPCWSTR, int);
PFN_TEXTOUTW pfnOriginalTextOutW = NULL;
typedef BOOL(WINAPI* PFN_EXTTEXTOUTW)(HDC, int, int, UINT, const RECT*, LPCWSTR, UINT, const INT*);
PFN_EXTTEXTOUTW pfnOriginalExtTextOutW = NULL;
typedef int(WINAPI* PFN_DRAWTEXTW)(HDC, LPCWSTR, int, LPRECT, UINT);
PFN_DRAWTEXTW pfnOriginalDrawTextW = NULL;


// 用於儲存自訂set指令所需資訊的結構
struct CustomCommandInfo {
    DWORD CheckboxOffset;
    DWORD ValueOffset;
};
struct CustomCommandRule {
    std::wstring keyString;
    int minValue; // 範圍的最小值 (包含)
    int maxValue; // 範圍的最大值 (包含)
    CustomCommandInfo action;
};

// 輔助函式用於判斷字串是否相符
int ShouldDoCustomJump() {
    LPCWSTR commandString = (LPCWSTR)0x4F53D1;
    if (IsBadReadPtr(commandString, sizeof(wchar_t))) {
        return 0; // 指標無效，返回 0 (false)
    }
    const wchar_t* commandsToMatch[] = {
        L"/status", L"/accompany", L"/eo", L"/offline on", L"/offline off",
        L"/watch", L"/encount on", L"/encount off", L"/pile", L"前往冒險者之島"
    };
    for (const auto& cmd : commandsToMatch) {
        if (wcscmp(commandString, cmd) == 0) {
            return 1; // 找到相符指令，明確返回 1 (true)
        }
    }
    return 0; // 沒找到，明確返回 0 (false)
}

// --- Let指令 Hook ---
void ProcessParameterHook(DWORD eax_value) {
    if (IsBadReadPtr((void*)eax_value, sizeof(LPCWSTR))) return;
    LPCWSTR* pString = (LPCWSTR*)eax_value;
    if (IsBadReadPtr(*pString, sizeof(wchar_t))) return;

    LPCWSTR finalStringAddress = *pString;
    wchar_t buffer[64];

    //【優化】使用 map 將字串對應到處理邏輯，替代冗長的 if-else。
    // 使用 std::function 儲存不同的處理函式。
    static std::unordered_map<std::wstring, std::function<void(LPCWSTR)>> actionMap = {
        {L"#系統時間秒數差", [](LPCWSTR addr) {
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
        {L"#遊戲時間", [](LPCWSTR addr) {
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
 else { OverwriteStringInMemory(addr, L"視窗未開啟"); }
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
else { OverwriteStringInMemory(addr, L"視窗未開啟"); }
}},
{L"#EV1", [](LPCWSTR addr) { wchar_t buf[16]; swprintf(buf, 16, L"%d", ev1); OverwriteStringInMemory(addr, buf); }},
{L"#EV2", [](LPCWSTR addr) { wchar_t buf[16]; swprintf(buf, 16, L"%d", ev2); OverwriteStringInMemory(addr, buf); }},
{L"#EV3", [](LPCWSTR addr) { wchar_t buf[16]; swprintf(buf, 16, L"%d", ev3); OverwriteStringInMemory(addr, buf); }},
{L"#亂數", [](LPCWSTR addr) {
    static std::mt19937 gen(std::random_device{}());
        static std::uniform_int_distribution<> distrib(0, 10);
    wchar_t buf[16];
    swprintf(buf, 16, L"%d", distrib(gen));
    OverwriteStringInMemory(addr, buf);
}}
    };
    auto it = actionMap.find(finalStringAddress);
    if (it != actionMap.end()) {
        it->second(finalStringAddress); // 執行對應的 lambda 函式
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

// --- Button確定指令 Hook ---
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

// --- Button指令 Hook ---
void ProcessEcxString(LPCWSTR stringAddress) {
    if (stringAddress == nullptr || IsBadReadPtr(stringAddress, sizeof(wchar_t))) {
        return;
    }
    if (wcscmp(stringAddress, L"從心所行之路即是正路") == 0) {
        // 【修改】呼叫新的 QA() 函式
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

// ---Print指令 Hook ---
void ProcessPrintHook(DWORD edx_value) {
    if (IsBadReadPtr((void*)edx_value, sizeof(LPCWSTR))) return;
    LPCWSTR* pString = (LPCWSTR*)edx_value;
    if (IsBadReadPtr(*pString, sizeof(wchar_t))) return;
    LPCWSTR finalStringAddress = *pString;
    if (wcscmp(finalStringAddress, L"#0000/00/00 00:00:00") == 0) {
        // 取得當前本機時間
        SYSTEMTIME lt;
        GetLocalTime(&lt);
        wchar_t timeString[64];
        swprintf(timeString, 64, L"%04d/%02d/%02d %02d:%02d:%02d",
            lt.wYear, lt.wMonth, lt.wDay,
            lt.wHour, lt.wMinute, lt.wSecond);
        OverwriteStringInMemory(finalStringAddress, timeString);
    }
    else if (wcscmp(finalStringAddress, L"#計時開始") == 0) {
        scriptTime = time(NULL); // 記錄當前時間戳 (秒)
        OverwriteStringInMemory(finalStringAddress, L"計時計時");
    }
    else if (wcscmp(finalStringAddress, L"#計時停止") == 0) {
        if (scriptTime == 0) {
            OverwriteStringInMemory(finalStringAddress, L"無計時紀錄");
        }
        else {
            time_t endTime = time(NULL);
            const time_t one_year_in_seconds = 31536000;
            // 判斷 scriptTime 存的是時間點還是已經計算過的時間差
            if (scriptTime > one_year_in_seconds) {
                scriptTime = endTime - scriptTime; // 計算並儲存時間差
            }
            OverwriteStringInMemory(finalStringAddress, L"計時停止");
        }
    }
    else if (wcscmp(finalStringAddress, L"#時間差00:00:00") == 0) {
        if (scriptTime == 0) {
            OverwriteStringInMemory(finalStringAddress, L"無計時紀錄");
        }
        else {
            time_t secondsToFormat = 0;
            const time_t one_year_in_seconds = 31536000;

            if (scriptTime > one_year_in_seconds) {
                // 計時未停止，動態計算當前時間與開始時間的差額
                secondsToFormat = time(NULL) - scriptTime;
            }
            else {
                // 計時已停止，直接使用儲存的時間差
                secondsToFormat = scriptTime;
            }

            // 格式化為 時:分:秒
            long h = (long)(secondsToFormat / 3600);
            long m = (long)((secondsToFormat % 3600) / 60);
            long s = (long)(secondsToFormat % 60);

            wchar_t buffer[128];
            swprintf(buffer, 128, L"經過%d時%d分%d秒", h, m, s);
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

//  ---Ifitem指令 Hook ---
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

//  ---set指令 Hook ---
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
        //戰鬥設置v
        { L"戰鬥精靈", 0, 5, { 0xF61C6, 0xF61C7 } },
        { L"戰鬥精靈人物", 0,101, { 0xF61C6, 0xF61F5 } },
        { L"戰鬥精靈寵物", 0,101, { 0xF61C6, 0xF61F6 } },
        { L"戰鬥補血", 0, 1, { 0xF61C8, 0xF61C8 } },
        { L"戰寵補血", 0, 3, { 0xF61C9, 0xF61CA } },
        { L"戰寵補血人物", 0,101, { 0xF61C9, 0xF61F9 } },
        { L"戰寵補血寵物", 0,101, { 0xF61C9, 0xF61FA } },
        { L"復活精靈", 0, 5, { 0xF61FE, 0xF6200 } },
        { L"復活物品", 0, 1, { 0xF61FF, 0xF61FF } },
        //一般設置v
        { L"平時精靈", 0,20, { 0xF6229, 0xF622A } },
        { L"平時精靈人物", 0,101, { 0xF6229, 0xF622B } },
        { L"平時精靈寵物", 0,101, { 0xF6229, 0xF622C } },
        { L"平時補血", 0, 1, { 0xF622D, 0xF622D } },
        { L"平時技能", 0,26, { 0xF6232, 0xF6233 } },
        { L"寵物七技", 0, 1, { 0xF6662, 0xF6662 } },
        { L"戰鬥換裝", 0, 1, { 0xF6661, 0xF6661 } },
        { L"戰鬥詳情", 0, 1, { 0xF6669, 0xF6669 } },
        //資料設定
        { L"白名單", 0, 1, { 0xF53CC, 0xF53CC } },
        { L"黑名單", 0, 1, { 0xF53CD, 0xF53CD } },
        { L"郵件白名單", 0, 1, { 0xF53B8, 0xF53B8 } },
        { L"自動剔除", 0, 1, { 0xF53CE, 0xF53CE } },
        { L"隊員1", 0, 1, { 0xF53B9, 0xF53B9 } },
        { L"隊員2", 0, 1, { 0xF53BA, 0xF53BA } },
        { L"隊員3", 0, 1, { 0xF53BB, 0xF53BB } },
        { L"隊員4", 0, 1, { 0xF53BC, 0xF53BC } },
        { L"隊員5", 0, 1, { 0xF53BD, 0xF53BD } },
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
                    // 判斷此規則是否為帶參數的指令 (最大值 > 1)
                    if (rule.maxValue > 1) {
                        if (info.CheckboxOffset != info.ValueOffset) {
                            BYTE* pValue = (BYTE*)hMod + info.ValueOffset;
                            *pValue = valueAsInt - 1;
                        }
                    }
                }
                return true; // 處理完畢
            }
        }
    }
    return false; // 沒找到任何符合的規則
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
    gCommandChainJumpTarget = (void*)((BYTE*)hMod + Addr::CommandChainJumpTargetOffset); // 【修改】
    DWORD oldProtect;
    VirtualProtect(pTarget, 6, PAGE_EXECUTE_READWRITE, &oldProtect);
    pTarget[0] = 0xE9;
    DWORD offset = (DWORD)((BYTE*)&MyCommandChainHook - pTarget - 5);
    *(DWORD*)(pTarget + 1) = offset;
    pTarget[5] = 0x90;
    VirtualProtect(pTarget, 6, oldProtect, &oldProtect);
}

//  ---set2指令 Hook ---
void ProcessStringCopyHook(LPCWSTR stringAddress) {
    if (IsBadReadPtr(stringAddress, sizeof(wchar_t))) return;
    }
    // 如果不符合任何條件，函式直接結束，不執行任何操作
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


// --- 自動堆疊 (Auto Pile) Hook ---
void ProcessAutoPile() {
    HMODULE hMod = GetModuleHandleW(L"Assa8.0B5.exe");
    if (!hMod) return;
    BYTE* switchAddress = (BYTE*)hMod + Addr::AutoPileSwitchOffset;
    BYTE switchValue = *switchAddress;
    if (switchValue == 1) {
        pileCounter++;
        // 如果計數器大於10
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


// --- 自動發話 Hook ---
//比對:呼喚野獸、堆疊、開始遇敵
__declspec(naked) void MyCompareHook() {
    __asm {
        pushad
        call ShouldDoCustomJump // 呼叫 C++ 輔助函式，結果在 EAX (0 或 1)
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

// --- 自動發話訊息清空 Hook ---
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

// --- 公開的總安裝函式 ---
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