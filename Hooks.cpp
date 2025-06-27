// 檔案: Hooks.cpp
// 說明: 此檔案定義了所有的 Hook 函式以及安裝它們的邏輯。
//       "Hook" (掛鉤) 的本質是在程式執行到某個特定位置時，
//       強制它跳轉來執行我們自己寫的程式碼，完成後再跳回去繼續執行。

#include "pch.h"
#include "Hooks.h"
#include "Globals.h"
#include "helpers.h"
#include "RiddleDatabase.h" // 引用猜謎資料庫
#include <functional>
#include <random>

// --- 命名空間與常數定義 ---
// 【名稱建議】Addr 可以考慮改名為 Offsets 或 GameOffsets，更明確表示這些是記憶體偏移量。
namespace Addr {
    // 這些是 Assa8.0B5.exe 程式中，我們想要 Hook 的位置相對於模組基底位址的偏移量。
    // 這些值是透過逆向工程工具 (如 x64dbg, IDA) 分析 Assa 程式得到的。
    //
    // 主程式 (Assa) Hook 相關偏移
    constexpr DWORD CompareHookOffset = 0x71972;        // 自動發話比對點
    constexpr DWORD AddEspHookOffset = 0x71AD5;         // 自動發話訊息清空點
    constexpr DWORD EcxStringHookOffset = 0x7AA0B;      // 處理 "Button" 指令的點 (我們用來觸發猜謎)
    constexpr DWORD ParameterHookOffset = 0x77E80;      // 處理 "Let" 指令的點 (用來實現自訂變數)
    constexpr DWORD PrintHookOffset = 0x76269;          // 處理 "Print" 指令的點 (用來實現計時器)
    constexpr DWORD Button2HookOffset = 0x7A472;        // 處理 "Button2" (確定按鈕) 的點
    constexpr DWORD IfitemHookOffset = 0x88AE3;         // 處理 "Ifitem" 指令的點
    constexpr DWORD CommandChainHookOffset = 0x820B2;    // 處理 "Set" 指令的點 (用來設定外掛內部參數)
    constexpr DWORD CommandChainJumpTargetOffset = 0x83E66; // Set指令成功後跳轉的位址
    constexpr DWORD StringCopyHookOffset = 0x7EBDB;     // 處理 "Set2" 指令的點 (用來替換字串)
    constexpr DWORD StringCopyTargetPtr = 0x1220;       // Set2指令原呼叫函式的指標位址

    // 自動堆疊相關偏移
    constexpr DWORD AutoPileHookOffset = 0x6CC7E;        // 自動堆疊的觸發點
    constexpr DWORD AutoPileSwitchOffset = 0xF6668;     // 自動堆疊功能的開關位址
    constexpr DWORD AutoPileJumpTargetOffset = 0x6D399; // 自動堆疊Hook完成後跳回的位址

    // 目標遊戲 (sa_8002a.exe) 記憶體偏移
    constexpr DWORD GameTimeOffset = 0x1AD760;          // 遊戲時間
    constexpr DWORD WindowStateOffset = 0x1696CC;       // 遊戲視窗狀態 (用於判斷NPC對話窗是否開啟)
    constexpr DWORD NpcIdOffset = 0xAFA34;              // NPC ID
    constexpr DWORD WinIdOffset = 0xAFA30;              // 視窗 ID
}

// --- Hook 返回位址全域變數 ---
// 這些變數用來儲存每個 Hook 點原始程式碼的下一條指令位址。
// 在我們的 Hook 函式執行完畢後，需要跳轉到這些位址，讓原程式能繼續正常執行。
// 【名稱建議】gCompareHookReturnAddr 可改為 g_pCompareHookReturn (p代表pointer)
BYTE* gCompareHookReturnAddr = nullptr;
BYTE* gAddEspHookReturnAddr = nullptr;
BYTE* gEcxStringHookReturnAddr = nullptr;
BYTE* gParameterHookReturnAddr = nullptr;
BYTE* gPrintHookReturnAddr = nullptr;
BYTE* gButton2HookReturnAddr = nullptr;
BYTE* gIfitemHookReturnAddr = nullptr;
BYTE* gCommandChainHookReturnAddr = nullptr;
BYTE* gStringCopyHookReturnAddr = nullptr;
void* g_pfnVbaStrCopy = nullptr; // 儲存Set2原始呼叫的函式指標
void* gAutoPileJumpTarget = nullptr;
void* gCommandChainJumpTarget = nullptr;

// 【優化】將重複的 Hook 安裝邏輯提取出來，做成一個共用函式
static bool InstallJmpHook(LPCWSTR moduleName, DWORD offset, LPVOID newFunction, size_t hookSize, LPVOID* returnAddress) {
    HMODULE hMod = GetModuleHandleW(moduleName);
    if (!hMod) {
        // 如果獲取模組失敗，可以考慮記錄錯誤
        return false;
    }

    BYTE* pTarget = (BYTE*)hMod + offset;
    *returnAddress = pTarget + hookSize;

    DWORD oldProtect;
    if (VirtualProtect(pTarget, hookSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        pTarget[0] = 0xE9; // JMP 指令
        DWORD jmpOffset = (DWORD)((BYTE*)newFunction - pTarget - 5);
        *(DWORD*)(pTarget + 1) = jmpOffset;

        // 用 NOP (0x90) 填充多餘的空間，確保指令完整性
        for (size_t i = 5; i < hookSize; ++i) {
            pTarget[i] = 0x90;
        }

        VirtualProtect(pTarget, hookSize, oldProtect, &oldProtect);
        return true;
    }
    return false;
}


// --- Let指令 Hook ---
// 函式: ProcessParameterHook
// 功能: 處理 Assa 的 "Let" 指令，讓我們可以定義自己的變數 (如 #遊戲時間)。
// 說明: 當 Assa 腳本執行到 `Let` 時，這個函式會被觸發。它檢查變數名是否為我們預定義的
//       特殊字串，如果是，就從遊戲記憶體或其他來源讀取對應的值，並寫回給 Assa。
// 調用: 由 MyParameterHook (Naked Function) 呼叫。
void ProcessParameterHook(DWORD eax_value) {
    // ... (此處省略內部實作，功能為字串比對與執行對應操作)
    if (IsBadReadPtr((void*)eax_value, sizeof(LPCWSTR))) return;
    LPCWSTR* pString = (LPCWSTR*)eax_value;
    if (IsBadReadPtr(*pString, sizeof(wchar_t))) return;

    LPCWSTR finalStringAddress = *pString;

    static const std::unordered_map<std::wstring, std::function<void(LPCWSTR)>> actionMap = {
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
 else { OverwriteStringInMemory(addr, L"視窗未開啟"); }
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
        it->second(finalStringAddress);
    }
}
// 函式: MyParameterHook
// 功能: 用來跳轉到 ProcessParameterHook 的組合語言 (Assembly) 程式碼。
// 說明: 這種函式被稱為 "Naked Function"，代表編譯器不會為它產生任何額外的堆疊(stack)操作程式碼。
//       我們需要手動保存所有暫存器 (pushad/popad)，呼叫 C++ 函式，然後跳轉回原始程式碼。
__declspec(naked) void MyParameterHook() {
    __asm {
        lea eax, [ebp - 0x38]   // 取得參數在堆疊上的位址
        pushad                  // 保存所有通用暫存器
        push eax                // 將參數位址壓入堆疊，作為C++函式的參數
        call ProcessParameterHook // 呼叫我們的處理函式
        pop eax                 // 平衡堆疊
        popad                   // 恢復所有暫存器
        push ecx                // 以下是原始被我們覆蓋的程式碼
        push eax
        jmp gParameterHookReturnAddr // 跳轉回 Assa 的原始執行流程
    }
}

// --- Button確定指令 Hook ---
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

// --- Button指令 Hook (謎題) ---
// 函式: ProcessEcxString
// 功能: 處理 Assa 的 "Button" 指令，攔截特定字串來觸發自動猜謎。
// 調用: 由 MyEcxStringHook 呼叫。
void ProcessEcxString(LPCWSTR stringAddress) {
    if (stringAddress && !IsBadReadPtr(stringAddress, sizeof(wchar_t))) {
        if (wcscmp(stringAddress, L"從心所行之路即是正路") == 0) {
            // 如果字串相符，就呼叫 RiddleDatabase.cpp 中的 QA 函式來獲取答案。
            std::wstring finalOutput = QA();
            // 將獲取到的答案寫回記憶體，覆蓋掉原來的字串。
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


// ---Print指令 Hook (計時)---
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
    else if (wcscmp(finalStringAddress, L"#計時開始") == 0) {
        scriptTime = time(NULL);
        OverwriteStringInMemory(finalStringAddress, L"計時開始");
    }
    else if (wcscmp(finalStringAddress, L"#計時停止") == 0) {
        if (scriptTime != 0) {
            time_t endTime = time(NULL);
            // 只有在 scriptTime 是一個時間戳時才計算差值
            if (scriptTime > 31536000) { // 避免重複計算
                scriptTime = endTime - scriptTime;
            }
        }
        OverwriteStringInMemory(finalStringAddress, L"計時停止");
    }
    else if (wcscmp(finalStringAddress, L"#時間差00:00:00") == 0) {
        if (scriptTime == 0) {
            OverwriteStringInMemory(finalStringAddress, L"無計時紀錄");
        }
        else {
            time_t secondsToFormat = (scriptTime > 31536000) ? (time(NULL) - scriptTime) : scriptTime;
            long h = (long)(secondsToFormat / 3600);
            long m = (long)((secondsToFormat % 3600) / 60);
            long s = (long)(secondsToFormat % 60);
            wchar_t buffer[128];
            swprintf(buffer, 128, L"經過 %d時 %d分 %d秒", h, m, s);
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

    struct CustomCommandRule {
        std::wstring keyString;
        int minValue;
        int maxValue;
        DWORD CheckboxOffset;
        DWORD ValueOffset;
    };

    static const std::vector<CustomCommandRule> rules = {
        //戰鬥設置
        { L"戰鬥精靈", 0, 5, 0xF61C6, 0xF61C7 },
        { L"戰鬥精靈人物", 0,101, 0xF61C6, 0xF61F5 },
        { L"戰鬥精靈寵物", 0,101, 0xF61C6, 0xF61F6 },
        { L"戰鬥補血", 0, 1, 0xF61C8, 0xF61C8 },
        { L"戰寵補血", 0, 3, 0xF61C9, 0xF61CA },
        { L"戰寵補血人物", 0,101, 0xF61C9, 0xF61F9 },
        { L"戰寵補血寵物", 0,101, 0xF61C9, 0xF61FA },
        { L"復活精靈", 0, 5, 0xF61FE, 0xF6200 },
        { L"復活物品", 0, 1, 0xF61FF, 0xF61FF },
        //一般設置
        { L"平時精靈", 0,20, 0xF6229, 0xF622A },
        { L"平時精靈人物", 0,101, 0xF6229, 0xF622B },
        { L"平時精靈寵物", 0,101, 0xF6229, 0xF622C },
        { L"平時補血", 0, 1, 0xF622D, 0xF622D },
        { L"平時技能", 0,26, 0xF6232, 0xF6233 },
        { L"寵物七技", 0, 1, 0xF6662, 0xF6662 },
        { L"戰鬥換裝", 0, 1, 0xF6661, 0xF6661 },
        { L"戰鬥詳情", 0, 1, 0xF6669, 0xF6669 },
        //資料設定
        { L"白名單", 0, 1, 0xF53CC, 0xF53CC },
        { L"黑名單", 0, 1, 0xF53CD, 0xF53CD },
        { L"郵件白名單", 0, 1, 0xF53B8, 0xF53B8 },
        { L"自動剔除", 0, 1, 0xF53CE, 0xF53CE },
        { L"隊員1", 0, 1, 0xF53B9, 0xF53B9 },
        { L"隊員2", 0, 1, 0xF53BA, 0xF53BA },
        { L"隊員3", 0, 1, 0xF53BB, 0xF53BB },
        { L"隊員4", 0, 1, 0xF53BC, 0xF53BC },
        { L"隊員5", 0, 1, 0xF53BD, 0xF53BD },
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

// ---set2指令 (字串替換) Hook ---
void ProcessStringCopyHook(LPCWSTR stringAddress) {
    if (IsBadReadPtr(stringAddress, sizeof(wchar_t))) return;
    static const std::unordered_map<std::wstring, std::wstring> replacements = {
        {L"決鬥", L"決斗"}, {L"自動電腦對戰", L"自動KNPC"}, {L"自動戰鬥", L"自動戰斗"},
        {L"快速戰鬥", L"快速戰斗"}, {L"登入主機", L"登陸主機"}, {L"登入副機", L"登陸副機"},
        {L"登入人物", L"登陸人物"}, {L"自動登入", L"自動登陸"}, {L"腳本延遲", L"腳本延時"},
        {L"戰鬥補氣", L"戰斗補氣"},
        // 功能取消
        {L"走動遇敵", L"功能取消"}, {L"走動步數", L"功能取消"}, {L"快速遇敵", L"功能取消"},
        {L"快速延遲", L"功能取消"}, {L"自動猜謎", L"功能取消"}
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

// --- 自動堆疊 (Auto Pile) Hook ---
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

// -- - 自動發話(Compare) Hook-- -
// 函式: ShouldDoCustomJump
// 功能: 判斷目前 Assa 準備發送的訊息是否為我們需要攔截的特殊指令。
// 說明: 這些指令如果直接發送會沒有效果，需要我們攔截下來並走特殊的處理流程。
// 調用: 由 MyCompareHook 呼叫。
// 返回: 1 表示需要攔截，0 表示不需要。
int ShouldDoCustomJump() {
    LPCWSTR commandString = (LPCWSTR)0x4F53D1; // 這是 Assa 內部存放待發送訊息的記憶體位址
    if (IsBadReadPtr(commandString, sizeof(wchar_t))) return 0;

    static const std::vector<std::wstring> commandsToMatch = {
        L"/status", L"/accompany", L"/eo", L"/offline on", L"/offline off",
        L"/watch", L"/encount on", L"/encount off", L"/pile", L"前往冒險者之島"
    };

    for (const auto& cmd : commandsToMatch) {
        if (wcscmp(commandString, cmd.c_str()) == 0) {
            return 1; // 找到匹配的指令
        }
    }
    return 0;
}
__declspec(naked) void MyCompareHook() {
    __asm {
        pushad
        call ShouldDoCustomJump // 呼叫C++函式，返回值會在 EAX 暫存器中
        cmp eax, 1              // 比較返回值是否為 1
        popad
        je do_custom_jump       // 如果是 1 (je = jump if equal)，就跳到我們的特殊處理
        mov dl, byte ptr ds : [0x4F5442] // 否則，執行原始程式碼
        jmp gCompareHookReturnAddr      // 然後跳回原流程
        do_custom_jump :
        push 0x004719C4         // 這是特殊處理的跳轉位址，讓 Assa 認為指令已成功發送
            ret                     // 返回
    }
}

// --- 自動發話訊息清空 Hook ---
__declspec(naked) void MyAddEspHook() {
    __asm {
        mov eax, 0x00200020
        // 使用 rep stosd 來快速清空記憶體
        lea edi, ds: [0x4F53D1]
        mov ecx, 16 // 16 * 4 = 64 bytes
        rep stosd

        lea eax, [ebp - 0x7C]
        push edx
        push eax
        jmp gAddEspHookReturnAddr
    }
}

// --- 公開的總安裝函式 ---
// 函式: InstallAllHooks
// 功能: 呼叫所有獨立的 Hook 安裝函式，一次性完成所有功能的掛載。
// 調用: 由 dllmain.cpp 在 DLL_PROCESS_ATTACH 事件中呼叫。
void InstallAllHooks() {
    // 使用新的輔助函式來安裝所有 Hook
    InstallJmpHook(L"Assa8.0B5.exe", Addr::CompareHookOffset, MyCompareHook, 6, (LPVOID*)&gCompareHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::AddEspHookOffset, MyAddEspHook, 5, (LPVOID*)&gAddEspHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::EcxStringHookOffset, MyEcxStringHook, 5, (LPVOID*)&gEcxStringHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::ParameterHookOffset, MyParameterHook, 5, (LPVOID*)&gParameterHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::PrintHookOffset, MyPrintHook, 5, (LPVOID*)&gPrintHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::Button2HookOffset, MyButton2Hook, 6, (LPVOID*)&gButton2HookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::IfitemHookOffset, MyIfitemHook, 6, (LPVOID*)&gIfitemHookReturnAddr);

    // --- 特殊 Hook 的安裝 ---
    HMODULE hMod = GetModuleHandleW(L"Assa8.0B5.exe");
    if (hMod) {
        // 部分 Hook 需要在安裝前額外設定跳轉目標位址或取得函式指標
        gCommandChainJumpTarget = (void*)((BYTE*)hMod + Addr::CommandChainJumpTargetOffset);
        InstallJmpHook(L"Assa8.0B5.exe", Addr::CommandChainHookOffset, MyCommandChainHook, 6, (LPVOID*)&gCommandChainHookReturnAddr);

        gAutoPileJumpTarget = (void*)((BYTE*)hMod + Addr::AutoPileJumpTargetOffset);
        InstallJmpHook(L"Assa8.0B5.exe", Addr::AutoPileHookOffset, MyAutoPileHook, 5, (LPVOID*)&gAutoPileJumpTarget);

        DWORD* pFuncPtrAddr = (DWORD*)((BYTE*)hMod + Addr::StringCopyTargetPtr);
        g_pfnVbaStrCopy = (void*)*pFuncPtrAddr;
        InstallJmpHook(L"Assa8.0B5.exe", Addr::StringCopyHookOffset, MyStringCopyHook, 6, (LPVOID*)&gStringCopyHookReturnAddr);
    }
}