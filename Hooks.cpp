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
#include <string>      // 【新增】用於處理字串物件
#include <algorithm>   // 【新增】用於字串轉換
#include <cctype>      // 【新增】用於字元大小寫轉換
#include <cmath> // 【新增】用於 fmod 函式 (浮點數取餘)
#include <vector> // 【新增】用於字串分割
#include <sstream> // 【新增】用於字串分割
#include <fstream> // 【新增】用於檔案讀寫 (fstream)

// =================================================================
// Assign 指令核心邏輯 (修正版 - 支援 assign save/load)
// =================================================================
bool ProcessAssignStringCommand() {
    try {
        // 1. 從固定位址讀取指令字串
        LPCWSTR finalStringAddress = (LPCWSTR)0x0019DF9C;
        if (IsBadReadPtr(finalStringAddress, sizeof(wchar_t))) {
            return false;
        }
        std::wstring fullCommand(finalStringAddress);

        // 2. 優先嘗試用空格分割，檢查是否為 save/load 指令
        std::wstringstream wss(fullCommand);
        std::vector<std::wstring> space_parts;
        std::wstring part;
        while (wss >> part) { // 按空格分割所有部分
            space_parts.push_back(part);
        }

        // 檢查是否為 "assign save" 或 "assign load"
        if (space_parts.size() == 2) {
            std::wstring cmd = space_parts[0];
            std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);

            if (cmd == L"assign") {
                std::wstring action = space_parts[1];
                std::transform(action.begin(), action.end(), action.begin(), ::tolower);
                const char* filename = "ev_vars.txt";

                if (action == L"save") {
                    std::ofstream outFile(filename);
                    if (outFile.is_open()) {
                        outFile << ev1 << std::endl;
                        outFile << ev2 << std::endl;
                        outFile << ev3 << std::endl;
                        outFile.close();
                        //MessageBoxW(NULL, L"所有 ev 變數已成功保存到 ev_vars.txt", L"Assign SAVE", MB_OK | MB_ICONINFORMATION);
                        return true;
                    }
                }
                else if (action == L"load") {
                    std::ifstream inFile(filename);
                    if (inFile.is_open()) {
                        inFile >> ev1 >> ev2 >> ev3;
                        inFile.close();
                        wchar_t msg[256];
                        //swprintf_s(msg, 256, L"已從 ev_vars.txt 成功讀取變數:\n- ev1: %g\n- ev2: %g\n- ev3: %g", ev1, ev2, ev3);
                        //MessageBoxW(NULL, msg, L"Assign LOAD", MB_OK | MB_ICONINFORMATION);
                        return true;
                    }
                }
            }
        }


        // 3. 如果不是 save/load，則嘗試解析原有的數值運算指令
        // 格式: assign evX,=,100
        std::wstringstream wss_numeric(fullCommand);
        std::vector<std::wstring> num_parts;
        if (std::getline(wss_numeric, part, L' ')) {
            num_parts.push_back(part);
        }
        if (std::getline(wss_numeric, part, L' ')) {
            std::wstringstream wss_params(part);
            while (std::getline(wss_params, part, L',')) {
                num_parts.push_back(part);
            }
        }

        if (num_parts.size() != 4) {
            return false;
        }

        std::wstring cmd = num_parts[0];
        std::transform(cmd.begin(), cmd.end(), cmd.begin(), ::tolower);
        if (cmd != L"assign") {
            return false;
        }

        // ... (後續的數值運算邏輯完全不變) ...
        std::wstring varName = num_parts[1];
        std::transform(varName.begin(), varName.end(), varName.begin(), ::tolower);
        double* targetVariable = nullptr;
        if (varName == L"ev1")      targetVariable = &ev1;
        else if (varName == L"ev2") targetVariable = &ev2;
        else if (varName == L"ev3") targetVariable = &ev3;
        else return false;

        std::wstring valStr = num_parts[3];
        bool is_valid_number = !valStr.empty() && [&valStr]() {
            bool dot_found = false;
            for (size_t i = 0; i < valStr.length(); ++i) {
                wchar_t c = valStr[i];
                if (iswdigit(c)) continue;
                if (c == L'.' && !dot_found) { dot_found = true; continue; }
                if (c == L'-' && i == 0) continue;
                return false;
            }
            return true;
            }();

        if (!is_valid_number) {
            return false;
        }

        std::wstring opStr = num_parts[2];
        if (opStr.length() != 1) return false;
        wchar_t op = opStr[0];
        double value = _wtof(valStr.c_str());

        switch (op) {
        case L'=': *targetVariable = value; break;
        case L'+': *targetVariable += value; break;
        case L'-': *targetVariable -= value; break;
        case L'*': *targetVariable *= value; break;
        case L'/': if (value != 0) *targetVariable /= value; break;
        case L'%': if (value != 0) *targetVariable = fmod(*targetVariable, value); break;
        default: return false;
        }

        wchar_t successMsg[256];
        //swprintf_s(successMsg, 256, L"變數 %s 的值已更新為: %g", varName.c_str(), *targetVariable);
        //MessageBoxW(NULL, successMsg, L"Assign 指令成功", MB_OK | MB_ICONINFORMATION);

        return true;
    }
    catch (...) {
        return false;
    }
}

// =================================================================
// 寵物檢查核心邏輯 (C++ 實現)
// =================================================================

// 將一個最多16個字元的Unicode數字字串，從指定位置開始轉換為64位元整數
ULONGLONG ParseUnicodeNumber(const wchar_t* str, int start, int len) {
    ULONGLONG result = 0;
    for (int i = 0; i < len; ++i) {
        wchar_t c = str[start + i];
        if (c >= L'0' && c <= L'9') {
            result = result * 10 + (c - L'0');
        }
        else {
            // 如果遇到非數字字元，可以視為無效
            return 0;
        }
    }
    return result;
}


// 處理寵物 "尾數" 和 "四圍" 檢查的主要函式
// checkType: 1=尾數檢查, 2=四圍檢查
// petStats: 包含寵物當前 LV, HP, ATK, DEF, DEX 的結構指標
// returns: 成功則返回 7777，失敗則返回對應的錯誤碼 (6666 或 9999)
int ProcessPetChecks(int checkType, const PetStats* stats) {
    if (checkType == 1) { // --- 尾數檢查 ---
        if ((stats->HP % 10 == 7) &&
            (stats->ATK % 10 == 7) &&
            (stats->DEF % 10 == 7) &&
            (stats->DEX % 10 == 7)) {
            return 7777; // 全部符合，成功
        }
        return 9999; // 至少一個不符合，失敗
    }

    if (checkType == 2) { // --- 四圍檢查 ---
        LPCWSTR* pOuterString = (LPCWSTR*)0x0019FA58;
        LPCWSTR targetString = *pOuterString;
        // 檢查字串長度是否為16
        if (wcslen(targetString) != 16) {
            return 5555;
        }
        // 從字串解析出目標數值
        ULONGLONG targetLv = ParseUnicodeNumber(targetString, 0, 3);
        ULONGLONG targetHP = ParseUnicodeNumber(targetString, 3, 4);
        ULONGLONG targetATK = ParseUnicodeNumber(targetString, 7, 3);
        ULONGLONG targetDEF = ParseUnicodeNumber(targetString, 10, 3);
        ULONGLONG targetDEX = ParseUnicodeNumber(targetString, 13, 3);
        // 檢查是否有任何一個數值解析為0，若有則視為無效並返回
        if (targetLv * targetHP * targetATK * targetDEF * targetDEX == 0) {
            return 6666;
        }
        // 進行比較
        if ((stats->LV == targetLv) &&
            (stats->HP >= targetHP) &&
            (stats->ATK >= targetATK) &&
            (stats->DEF >= targetDEF) &&
            (stats->DEX >= targetDEX)) {
            return 7777; // 全部通過
        }
        return 4444; // 至少一項不通過
    }

    return 0; // 未知的檢查類型
}

// 將結果數字寫回目標字串記憶體
void WriteCheckResult(int result) {
    LPCWSTR* pOuterString = (LPCWSTR*)0x0019FA58;
    if (IsBadReadPtr(pOuterString, sizeof(LPCWSTR)) || IsBadReadPtr(*pOuterString, sizeof(wchar_t))) {
        return;
    }
    wchar_t buffer[17]; // 16個字元 + 結束符
    swprintf_s(buffer, L"%d", result);
    OverwriteStringInMemory(*pOuterString, buffer);
}

// --- 命名空間與常數定義 ---
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
    // 【新增】Assign 指令 Hook 偏移量
    constexpr DWORD AssignHookOffset = 0x76DC0;
    // 自動堆疊相關偏移
    constexpr DWORD AutoPileHookOffset = 0x6CC7E;        // 自動堆疊的觸發點
    constexpr DWORD AutoPileSwitchOffset = 0xF6668;     // 自動堆疊功能的開關位址
    constexpr DWORD AutoPileJumpTargetOffset = 0x6D399; // 自動堆疊Hook完成後跳回的位址

    // 坐騎鎖定功能修復
    constexpr DWORD MountHookOffset = 0x686DD;          // 修復坐騎鎖定功能

    // 隊伍檢查功能
    constexpr DWORD TeamCheckHookOffset = 0x89B3A;      // 判斷隊伍狀態

    // 【新增】寵物檢查功能
    constexpr DWORD PetCheckHookOffset = 0x8A370;       // 寵物最大血量、四圍、尾數777檢查

    // 目標遊戲 (sa_8002a.exe) 記憶體偏移
    constexpr DWORD GameTimeOffset = 0x1AD760;          // 遊戲時間
    constexpr DWORD WindowStateOffset = 0x1696CC;       // 遊戲視窗狀態 (用於判斷NPC對話窗是否開啟)
    constexpr DWORD NpcIdOffset = 0xAFA34;              // NPC ID
    constexpr DWORD WinIdOffset = 0xAFA30;              // 視窗 ID
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
BYTE* gMountHookReturnAddr = nullptr;
BYTE* gTeamCheckHookReturnAddr = nullptr;
// 【新增】寵物檢查Hook的返回位址
BYTE* gPetCheckHookReturnAddr = nullptr;
// 【新增】Assign 指令 Hook 返回位址
BYTE* gAssignHookReturnAddr = nullptr;


// 【優化】將重複的 Hook 安裝邏輯提取出來，做成一個共用函式
static bool InstallJmpHook(LPCWSTR moduleName, DWORD offset, LPVOID newFunction, size_t hookSize, LPVOID* returnAddress) {
    HMODULE hMod = GetModuleHandleW(moduleName);
    if (!hMod) {
        return false;
    }

    BYTE* pTarget = (BYTE*)hMod + offset;
    *returnAddress = pTarget + hookSize;

    DWORD oldProtect;
    if (VirtualProtect(pTarget, hookSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        pTarget[0] = 0xE9; // JMP 指令
        DWORD jmpOffset = (DWORD)((BYTE*)newFunction - pTarget - 5);
        *(DWORD*)(pTarget + 1) = jmpOffset;

        for (size_t i = 5; i < hookSize; ++i) {
            pTarget[i] = 0x90;
        }

        VirtualProtect(pTarget, hookSize, oldProtect, &oldProtect);
        return true;
    }
    return false;
}

// --- 寵物最大血量、四圍、尾數777檢查 (重構版) ---
__declspec(naked) void MyPetCheckHook() {
    __asm {
        // eax 是由 Hook 點之前的 mov eax,[ebp-0x90] 所設定
        // 根據 eax 指向的值，判斷執行哪個分支
        // 檢查是否為 "滿血"
        cmp dword ptr[eax], 0x88406EFF
        je check_max_hp_asm
        // 檢查是否為 "尾數"
        cmp dword ptr[eax], 0x65785C3E
        je call_cpp_checker
        // 檢查是否為 "四圍"
        cmp dword ptr[eax], 0x570D56DB
        je call_cpp_checker
        // 如果都不是，則執行原始指令並返回
        original_path :
        push 0x004219D0
            jmp gPetCheckHookReturnAddr

        check_max_hp_asm :// --- 滿血邏輯 (純組合語言) ---
            push eax
            call esi
            test eax, eax
            jne fail_jump // 如果 call 的結果不為 0，跳轉到失敗路徑
            mov esi, [ebp - 0x14]
            mov edx, ds: [0x004F5204] //寵物數值基址
            lea eax, [esi + esi * 8]
            shl eax, 0x03
            sub eax, esi
            lea ecx, [eax + eax * 4]
            mov eax, [edx + ecx * 8 + 8] // 讀取寵物滿血值
            mov [ebp - 0x14],eax  // 將結果存入目標變數
            jmp success_path_asm

        call_cpp_checker :// --- 尾數 & 四圍邏輯 (呼叫 C++) ---
            push eax
            call esi
            test eax, eax
            jne fail_jump // 如果 call 的結果不為 0，跳轉
            mov esi, [ebp - 0x14]
            mov edx, ds: [0x004F5204] //寵物數值基址
            lea eax, [esi + esi * 8]
            shl eax, 0x03
            sub eax, esi
            lea ecx, [eax + eax * 4]
            // 準備結構體和參數以呼叫 C++ 函式
            sub esp, 20 // 在堆疊上為 PetStats 結構體分配空間
            mov edi, esp              // edi 指向堆疊上的 PetStats
            // 填充結構體
            mov eax, [edx + ecx * 8 + 0x1C] // 等級
            mov[edi + 0], eax
            mov eax, [edx + ecx * 8 + 8]    // 血
            mov[edi + 4], eax
            mov eax, [edx + ecx * 8 + 0x20] // 攻
            mov[edi + 8], eax
            mov eax, [edx + ecx * 8 + 0x24] // 防
            mov[edi + 12], eax
            mov eax, [edx + ecx * 8 + 0x28] // 敏
            mov[edi + 16], eax
            // 判斷是尾數還是四圍檢查
            mov eax, [ebp - 0x90] // 重新載入原始的檢查類型指標
            cmp dword ptr[eax], 0x65785C3E
            je is_last_digit

            is_stats_check :
        push edi          // 參數2: PetStats 結構指標
            push 2            // 參數1: checkType = 2 (四圍)
            jmp do_call

            is_last_digit :
        push edi          // 參數2: PetStats 結構指標
            push 1            // 參數1: checkType = 1 (尾數)

            do_call :
            call ProcessPetChecks // 呼叫 C++ 函式
            add esp, 8             // 清理2個參數的堆疊

            // C++ 函式返回後，EAX 中是結果 (7777, 6666, 9999)
            push eax               // 保存結果
            call WriteCheckResult  // 呼叫 C++ 函式將結果寫入字串
            pop eax                // 恢復結果到 EAX
            add esp, 20 // 釋放為 PetStats 分配的堆疊空間[eax]
            mov eax,7777
            mov [ebp - 0x14], eax  // 將結果存入目標變數
		success_path_asm :
            push 0x0048A119        // 跳轉到成功處理流程
            ret
        fail_jump :
            push 0x0048A3A2
            ret
    }
}


// --- 隊伍檢查 Hook ---
__declspec(naked) void MyTeamCheckHook() {
    __asm {
        cmp dword ptr ds : [eax] , 0x4F0D968A
        je team_check_logic
        push 0x004219D0
        jmp gTeamCheckHookReturnAddr
        team_check_logic :
            push eax
            call esi
            test eax, eax
            push esi
            mov ecx, 1
            mov esi, ds: [0x004F523C]
            cmp dword ptr ds : [esi + 0x30] , 0
            jle next1
            inc ecx
        next1 :
            cmp dword ptr ds : [esi + 0x60] , 0
            jle next2
            inc ecx
        next2 :
            cmp dword ptr ds : [esi + 0x90] , 0
            jle next3
            inc ecx
        next3 :
            cmp dword ptr ds : [esi + 0xC0] , 0
            jle end_comparisons
            inc ecx
        end_comparisons :
            pop esi
            mov[ebp - 0x14], ecx
            push 0x00489DC3
            ret
    }
}


// --- 修復坐騎鎖定功能 ---
__declspec(naked) void MyMountHook() {
    __asm {
        mov cl, byte ptr ds : [0x004F665B]
        cmp cl, byte ptr ds : [0x004F51EC]
        je jump_if_equal
        jmp gMountHookReturnAddr
        jump_if_equal :
        push 0x004687CA
        ret
    }
}

// --- Let指令 Hook ---
void ProcessParameterHook(DWORD eax_value) {
    if (IsBadReadPtr((void*)eax_value, sizeof(LPCWSTR))) return;
    LPCWSTR* pString = (LPCWSTR*)eax_value;
    if (IsBadReadPtr(*pString, sizeof(wchar_t))) return;
    LPCWSTR finalStringAddress = *pString;
    // --- 大小寫不敏感處理 ---
    // 1. 將傳入的字串轉換為全小寫的 std::wstring，用於後續比對
    std::wstring lookupKey(finalStringAddress);
    std::transform(lookupKey.begin(), lookupKey.end(), lookupKey.begin(),
        [](wchar_t c) { return std::tolower(c); });
    // ---
    static const std::unordered_map<std::wstring, std::function<void(LPCWSTR)>> actionMap = {
        {L"#系統毫秒時間戳00", [](LPCWSTR addr) {
            SYSTEMTIME st_now;
            GetSystemTime(&st_now); // 取得目前 UTC 時間
            FILETIME ft_now;
            SystemTimeToFileTime(&st_now, &ft_now); // 轉換為 FILETIME 格式
            ULARGE_INTEGER uli_now;
            uli_now.LowPart = ft_now.dwLowDateTime;
            uli_now.HighPart = ft_now.dwHighDateTime;
            // uli_now.QuadPart 是從 1601/1/1 至今的 100奈秒 (nanosecond) 間隔數
            // 1. 將其轉換為毫秒 (millisecond) -> 除以 10000
            ULONGLONG totalMilliseconds = uli_now.QuadPart / 10000;
            // 2. 對 7,776,000,000 取餘數
            ULONGLONG finalValue = totalMilliseconds % 7776000000ULL;
            wchar_t buffer[64];
            // 使用 %llu 來格式化 ULONGLONG (unsigned long long)
            swprintf_s(buffer, 64, L"%llu", finalValue);
            OverwriteStringInMemory(addr, buffer);
        }},
        {L"#系統秒數時間戳", [](LPCWSTR addr) {
            SYSTEMTIME st_now;
            GetSystemTime(&st_now); // 取得目前 UTC 時間
            FILETIME ft_now;
            SystemTimeToFileTime(&st_now, &ft_now); // 轉換為 FILETIME 格式
            ULARGE_INTEGER uli_now;
            uli_now.LowPart = ft_now.dwLowDateTime;
            uli_now.HighPart = ft_now.dwHighDateTime;
            // uli_now.QuadPart 是從 1601/1/1 至今的 100奈秒 (nanosecond) 間隔數
            // 1. 將其轉換為秒 -> 除以 10000000
            ULONGLONG totalMilliseconds = uli_now.QuadPart / 10000000;
            // 2. 對 7,776,000 取餘數
            ULONGLONG finalValue = totalMilliseconds % 7776000ULL;
            wchar_t buffer[64];
            // 使用 %llu 來格式化 ULONGLONG (unsigned long long)
            swprintf_s(buffer, 64, L"%llu", finalValue);
            OverwriteStringInMemory(addr, buffer);
        }},
        {L"#xpos", [](LPCWSTR addr) {
            // 【修正】改為使用全域變數 g_assaInfo.base
            if (!g_assaInfo.base) return;

            DWORD xposValue = 0;
            LPCVOID dynamicAddress = (LPCVOID)((BYTE*)g_assaInfo.base + 0xF5078);

            xposValue = *(DWORD*)dynamicAddress;

            wchar_t buffer[64];
            swprintf_s(buffer, 64, L"%d", xposValue);
            OverwriteStringInMemory(addr, buffer);
        }},
        {L"#ypos", [](LPCWSTR addr) {
            // 【修正】改為使用全域變數 g_assaInfo.base
            if (!g_assaInfo.base) return;

            DWORD yposValue = 0;
            LPCVOID dynamicAddress = (LPCVOID)((BYTE*)g_assaInfo.base + 0xF507C);

            yposValue = *(DWORD*)dynamicAddress;

            wchar_t buffer[64];
            swprintf_s(buffer, 64, L"%d", yposValue);
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
        {L"#npcid", [](LPCWSTR addr) {
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
{L"#winid", [](LPCWSTR addr) {
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
// 【修正】將 %d 改為 %g 以正確讀取 double 類型的值
        { L"#ev1", [](LPCWSTR addr) { wchar_t buf[32]; swprintf_s(buf, 32, L"%g", ev1); OverwriteStringInMemory(addr, buf); } },
        { L"#ev2", [](LPCWSTR addr) { wchar_t buf[32]; swprintf_s(buf, 32, L"%g", ev2); OverwriteStringInMemory(addr, buf); } },
        { L"#ev3", [](LPCWSTR addr) { wchar_t buf[32]; swprintf_s(buf, 32, L"%g", ev3); OverwriteStringInMemory(addr, buf); } },

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
__declspec(naked) void MyParameterHook() {
    __asm {
        lea eax, [ebp - 0x38]
        pushad
        push eax
        call ProcessParameterHook
        pop eax
        popad
        push ecx
        push eax
        jmp gParameterHookReturnAddr
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
void ProcessEcxString(LPCWSTR stringAddress) {
    if (stringAddress && !IsBadReadPtr(stringAddress, sizeof(wchar_t))) {
        if (wcscmp(stringAddress, L"從心所行之路即是正路") == 0) {
            std::wstring finalOutput = QA();
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
            if (scriptTime > 31536000) {
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
        { L"戰鬥精靈", 0, 5, 0xF61C6, 0xF61C7 }, { L"戰鬥精靈人物", 0,101, 0xF61C6, 0xF61F5 },
        { L"戰鬥精靈寵物", 0,101, 0xF61C6, 0xF61F6 }, { L"戰鬥補血", 0, 1, 0xF61C8, 0xF61C8 },
        { L"戰寵補血", 0, 3, 0xF61C9, 0xF61CA }, { L"戰寵補血人物", 0,101, 0xF61C9, 0xF61F9 },
        { L"戰寵補血寵物", 0,101, 0xF61C9, 0xF61FA }, { L"復活精靈", 0, 5, 0xF61FE, 0xF6200 },
        { L"復活物品", 0, 1, 0xF61FF, 0xF61FF }, { L"平時精靈", 0,20, 0xF6229, 0xF622A },
        { L"平時精靈人物", 0,101, 0xF6229, 0xF622B }, { L"平時精靈寵物", 0,101, 0xF6229, 0xF622C },
        { L"平時補血", 0, 1, 0xF622D, 0xF622D }, { L"平時技能", 0,26, 0xF6232, 0xF6233 },
        { L"寵物七技", 0, 1, 0xF6662, 0xF6662 }, { L"戰鬥換裝", 0, 1, 0xF6661, 0xF6661 },
        { L"戰鬥詳情", 0, 1, 0xF6669, 0xF6669 }, { L"白名單", 0, 1, 0xF53CC, 0xF53CC },
        { L"黑名單", 0, 1, 0xF53CD, 0xF53CD }, { L"郵件白名單", 0, 1, 0xF53B8, 0xF53B8 },
        { L"自動剔除", 0, 1, 0xF53CE, 0xF53CE }, { L"隊員1", 0, 1, 0xF53B9, 0xF53B9 },
        { L"隊員2", 0, 1, 0xF53BA, 0xF53BA }, { L"隊員3", 0, 1, 0xF53BB, 0xF53BB },
        { L"隊員4", 0, 1, 0xF53BC, 0xF53BC }, { L"隊員5", 0, 1, 0xF53BD, 0xF53BD },
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

// --- Assign 指令 Hook (修正版) ---
__declspec(naked) void MyAssignHook() {
    __asm {
        mov edx , [ebp - 0x01A0]
        cmp[edx] , 0x00530041
        jne original_path

        // 保存執行環境
        pushad
        // 呼叫 C++ 函式
        mov eax, offset ProcessAssignStringCommand
        call eax
        // 【修正】在 popad 之前，將 EAX 中的返回值暫存到堆疊中
        // (pushad 會壓入8個暫存器，共32 bytes，所以 esp+32 是 pushad 之前的位置)
        mov[esp + 32 - 4], eax // 將 eax 存到原 esp 的位置，安全可靠
        // 恢復所有通用暫存器 (此時 EAX 被舊值覆蓋)
        popad
        // 【修正】從堆疊中將我們暫存的返回值，重新取回到 EAX
        mov eax, [esp - 4]
        // 現在可以安全地比較返回值了
        cmp al, 1
        // 如果成功，跳轉到指定的成功路徑
        je assign_success

        original_path :
            // 如果失敗，執行原始被覆蓋的指令
            mov edx, [ebp - 0x01A0]
            // 然後跳轉回原始程式碼的下一行
            jmp gAssignHookReturnAddr

        assign_success :
            // 使用 push + ret 的方式跳轉
            push 0x00483E5F
            ret
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
    else {
        pileCounter = 0;
    }
}
__declspec(naked) void MyAutoPileHook() {
    __asm {
        pushad
        call ProcessAutoPile
        popad
        mov al, 0
        jmp gAutoPileJumpTarget
    }
}

// --- 自動發話(Compare) Hook ---
int ShouldDoCustomJump() {
    LPCWSTR commandString = (LPCWSTR)0x4F53D1;
    if (IsBadReadPtr(commandString, sizeof(wchar_t))) return 0;
    static const std::vector<std::wstring> commandsToMatch = {
        L"/status", L"/accompany", L"/eo", L"/offline on", L"/offline off",
        L"/watch", L"/encount on", L"/encount off",
        L"/pile", L"/pile on", L"/pile off", L"前往冒險者之島"
    };
    for (const auto& cmd : commandsToMatch) {
        if (wcscmp(commandString, cmd.c_str()) == 0) {
            return 1;
        }
    }
    return 0;
}
__declspec(naked) void MyCompareHook() {
    __asm {
        pushad
        call ShouldDoCustomJump
        cmp eax, 1
        popad
        je do_custom_jump
        mov dl, byte ptr ds : [0x4F5442]
        jmp gCompareHookReturnAddr
        do_custom_jump :
        push 0x004719C4
            ret
    }
}

// --- 自動發話訊息清空 Hook ---
__declspec(naked) void MyAddEspHook() {
    __asm {
        mov eax, 0x00200020
        lea edi, ds: [0x4F53D1]
        mov ecx, 16
        rep stosd
        lea eax, [ebp - 0x7C]
        push edx
        push eax
        jmp gAddEspHookReturnAddr
    }
}

// --- 公開的總安裝函式 ---
void InstallAllHooks() {
    InstallJmpHook(L"Assa8.0B5.exe", Addr::CompareHookOffset, MyCompareHook, 6, (LPVOID*)&gCompareHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::AddEspHookOffset, MyAddEspHook, 5, (LPVOID*)&gAddEspHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::EcxStringHookOffset, MyEcxStringHook, 5, (LPVOID*)&gEcxStringHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::ParameterHookOffset, MyParameterHook, 5, (LPVOID*)&gParameterHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::PrintHookOffset, MyPrintHook, 5, (LPVOID*)&gPrintHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::Button2HookOffset, MyButton2Hook, 6, (LPVOID*)&gButton2HookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::IfitemHookOffset, MyIfitemHook, 6, (LPVOID*)&gIfitemHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::MountHookOffset, MyMountHook, 6, (LPVOID*)&gMountHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::TeamCheckHookOffset, MyTeamCheckHook, 5, (LPVOID*)&gTeamCheckHookReturnAddr);
    // 【新增】安裝寵物檢查Hook，被覆蓋的原始指令長度為 5 bytes
    InstallJmpHook(L"Assa8.0B5.exe", Addr::PetCheckHookOffset, MyPetCheckHook, 5, (LPVOID*)&gPetCheckHookReturnAddr);
    // 【新增】安裝 Assign 指令 Hook，被覆蓋的原始指令長度為 6 bytes
    InstallJmpHook(L"Assa8.0B5.exe", Addr::AssignHookOffset, MyAssignHook, 6, (LPVOID*)&gAssignHookReturnAddr);

    // --- 特殊 Hook 的安裝 ---
    HMODULE hMod = GetModuleHandleW(L"Assa8.0B5.exe");
    if (hMod) {
        gCommandChainJumpTarget = (void*)((BYTE*)hMod + Addr::CommandChainJumpTargetOffset);
        InstallJmpHook(L"Assa8.0B5.exe", Addr::CommandChainHookOffset, MyCommandChainHook, 6, (LPVOID*)&gCommandChainHookReturnAddr);

        gAutoPileJumpTarget = (void*)((BYTE*)hMod + Addr::AutoPileJumpTargetOffset);
        InstallJmpHook(L"Assa8.0B5.exe", Addr::AutoPileHookOffset, MyAutoPileHook, 5, (LPVOID*)&gAutoPileJumpTarget);

        DWORD* pFuncPtrAddr = (DWORD*)((BYTE*)hMod + Addr::StringCopyTargetPtr);
        g_pfnVbaStrCopy = (void*)*pFuncPtrAddr;
        InstallJmpHook(L"Assa8.0B5.exe", Addr::StringCopyHookOffset, MyStringCopyHook, 6, (LPVOID*)&gStringCopyHookReturnAddr);
    }
}