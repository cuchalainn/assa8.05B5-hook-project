// 檔案: Hooks.cpp
// 說明: 此檔案定義了所有的 Hook 函式以及安裝它們的邏輯。
//       "Hook" (掛鉤) 的本質是在程式執行到某個特定位置時，
//       強制它跳轉來執行我們自己寫的程式碼，完成後再跳回去繼續執行。

#include "pch.h"
#include "Hooks.h"
#include "Globals.h"
#include "helpers.h"
#include "RiddleDatabase.h"

// --- 命名空間與常數定義 ---
namespace Addr {
    // 這些是 Assa8.0B5.exe 程式中，我們想要 Hook 的位置相對於模組基底位址的偏移量。
    // 這些值是透過逆向工程工具 (如 x64dbg, IDA) 分析 Assa 程式得到的。

    // 主程式 (Assa) Hook 相關偏移
    constexpr DWORD CompareHookOffset = 0x71972;     // 自動發話比對點
    constexpr DWORD AddEspHookOffset = 0x71AD5;     // 自動發話訊息清空點
    constexpr DWORD EcxStringHookOffset = 0x7AA0B;     // 處理 "Button" 指令的點 (我們用來觸發猜謎)
    constexpr DWORD ParameterHookOffset = 0x77E80;     // 處理 "Let" 指令的點 (用來實現自訂變數)
    constexpr DWORD PrintHookOffset = 0x76269;     // 處理 "Print" 指令的點 (用來實現計時器)
    constexpr DWORD Button2HookOffset = 0x7A472;     // 處理 "Button2" (確定按鈕) 的點
    constexpr DWORD IfitemHookOffset = 0x88AE3;     // 處理 "Ifitem" 指令的點
    constexpr DWORD CommandChainHookOffset = 0x820B2;     // 處理 "Set" 指令的點 (用來設定外掛內部參數)
    constexpr DWORD CommandChainJumpTargetOffset = 0x83E66;   // Set指令成功後跳轉的位址
    constexpr DWORD StringCopyHookOffset = 0x7EBDB;     // 處理 "Set2" 指令的點 (用來替換字串)
    constexpr DWORD StringCopyTargetPtr = 0x1220;      // Set2指令原呼叫函式的指標位址
    constexpr DWORD AssignHookOffset = 0x76DC0;     // 處理 "Assign" 指令

    // 自動堆疊相關偏移
    constexpr DWORD AutoPileHookOffset = 0x6CC7E;     // 自動堆疊的觸發點
    constexpr DWORD AutoPileSwitchOffset = 0xF6668;     // 自動堆疊功能的開關位址
    constexpr DWORD AutoPileJumpTargetOffset = 0x6D399;     // 自動堆疊Hook完成後跳回的位址

    // 坐騎鎖定功能修復
    constexpr DWORD MountHookOffset = 0x686DD;     // 修復坐騎鎖定功能

    // 隊伍檢查功能
    constexpr DWORD TeamCheckHookOffset = 0x89B3A;     // 判斷隊伍狀態

    // 寵物檢查功能
    constexpr DWORD PetCheckHookOffset = 0x8A370;     // 寵物最大血量、四圍、尾數777檢查

    // 目標遊戲 (sa_8002a.exe) 記憶體偏移
    constexpr DWORD GameTimeOffset = 0x1AD760;    // 遊戲時間
    constexpr DWORD WindowStateOffset = 0x1696CC;    // 遊戲視窗狀態 (用於判斷NPC對話窗是否開啟)
    constexpr DWORD NpcIdOffset = 0xAFA34;     // NPC ID
    constexpr DWORD WinIdOffset = 0xAFA30;     // 視窗 ID
	constexpr DWORD AccountNameOffset = 0x4AC28B8;  // 帳號名稱 
	constexpr DWORD CharacterNameOffset = 0x422DF18;    // 角色名稱 
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
BYTE* gMountHookReturnAddr = nullptr;
BYTE* gTeamCheckHookReturnAddr = nullptr;
BYTE* gPetCheckHookReturnAddr = nullptr;
BYTE* gAssignHookReturnAddr = nullptr;

void* g_pfnVbaStrCopy = nullptr;
void* gAutoPileJumpTarget = nullptr;
void* gCommandChainJumpTarget = nullptr;

// =================================================================
// C++ 核心邏輯函式
// =================================================================
// --- 字串處理輔助函式 ---
static std::string WstringToString(const std::wstring& wstr);
static void Trim(std::string& s);

// 【新增】獨立出一個獲取帳號名稱的函式，方便共用
static std::wstring GetAccountName() {
    if (!g_saInfo.hProcess || !g_saInfo.base) {
        return L"UnknownAccount";
    }

    char big5Buffer[256] = { 0 };
    wchar_t unicodeBuffer[256] = { 0 };

    std::wstring accountName = L"UnknownAccount";

    LPCVOID accountAddr = (LPCVOID)((BYTE*)g_saInfo.base + Addr::AccountNameOffset);
    if (ReadProcessMemory(g_saInfo.hProcess, accountAddr, big5Buffer, sizeof(big5Buffer) - 1, NULL)) {
        MultiByteToWideChar(950, 0, big5Buffer, -1, unicodeBuffer, 256);
        accountName = unicodeBuffer;

        std::wstring suffix = L"www.longzoro.com";
        size_t pos = accountName.rfind(suffix);
        if (pos != std::wstring::npos) {
            accountName.erase(pos, suffix.length());
        }
    }
    return accountName;
}

static std::string GetCurrentCharacterKey() {
    std::wstring accountName = GetAccountName(); // 直接呼叫新函式
    std::wstring characterName = L"UnknownChar";

    if (g_saInfo.hProcess && g_saInfo.base) {
        char big5Buffer[256] = { 0 };
        wchar_t unicodeBuffer[256] = { 0 };
        LPCVOID charAddr = (LPCVOID)((BYTE*)g_saInfo.base + Addr::CharacterNameOffset);
        if (ReadProcessMemory(g_saInfo.hProcess, charAddr, big5Buffer, sizeof(big5Buffer) - 1, NULL)) {
            MultiByteToWideChar(950, 0, big5Buffer, -1, unicodeBuffer, 256);
            characterName = unicodeBuffer;
        }
    }

    std::wstring fullKeyW = accountName + L"_" + characterName;
    return WstringToString(fullKeyW);
}
// --- Assign 指令核心邏輯 ---
static bool ProcessAssignStringCommand(LPCWSTR finalStringAddress) {
    try {
        if (IsBadReadPtr(finalStringAddress, sizeof(wchar_t))) return false;

        std::wstring fullCommand(finalStringAddress);
        std::wstringstream wss(fullCommand);
        std::vector<std::wstring> parts;
        std::wstring part;

        while (wss >> part) parts.push_back(part);

        // --- save 和 load 邏輯 ---
        if (parts.size() == 2 && _wcsicmp(parts[0].c_str(), L"assign") == 0) {
            std::string characterKey = GetCurrentCharacterKey();
            const std::string filename = "ev_vars.txt";
            std::string sectionHeader = "[" + characterKey + "]";

            if (_wcsicmp(parts[1].c_str(), L"save") == 0) {
                std::vector<std::string> lines;
                std::ifstream inFile(filename);
                std::string line;
                bool section_found = false;

                if (inFile.is_open()) {
                    while (std::getline(inFile, line)) lines.push_back(line);
                    inFile.close();
                }

                std::ofstream outFile(filename, std::ios::trunc);
                if (!outFile.is_open()) return false;

                // 【修改】使用迴圈儲存所有 ev 變數
                for (size_t i = 0; i < lines.size(); ++i) {
                    if (lines[i] == sectionHeader) {
                        section_found = true;
                        outFile << lines[i] << std::endl;
                        for (int j = 0; j < 10; ++j) {
                            outFile << "ev" << (j + 1) << "=" << ev[j] << std::endl;
                        }
                        while (i + 1 < lines.size() && !lines[i + 1].empty() && lines[i + 1][0] != '[') {
                            i++;
                        }
                    }
                    else {
                        outFile << lines[i] << std::endl;
                    }
                }

                if (!section_found) {
                    outFile << sectionHeader << std::endl;
                    for (int j = 0; j < 10; ++j) {
                        outFile << "ev" << (j + 1) << "=" << ev[j] << std::endl;
                    }
                }
                outFile.close();
                return true;

            }
            else if (_wcsicmp(parts[1].c_str(), L"load") == 0) {
                std::ifstream inFile(filename);
                if (!inFile.is_open()) {
                    for (int i = 0; i < 10; ++i) ev[i] = 0.0; // 檔案不存在，重置所有變數
                    return true;
                }

                std::string line;
                bool in_correct_section = false;
                for (int i = 0; i < 10; ++i) ev[i] = 0.0; // 先預設重置

                while (std::getline(inFile, line)) {
                    Trim(line);
                    if (line == sectionHeader) {
                        in_correct_section = true;
                        continue;
                    }
                    if (line.empty() || line[0] == '[') {
                        if (in_correct_section) break; // 已讀完該區塊，可提早結束
                        in_correct_section = false;
                        continue;
                    }
                    if (in_correct_section) {
                        size_t delimiter_pos = line.find('=');
                        if (delimiter_pos != std::string::npos) {
                            std::string key = line.substr(0, delimiter_pos);
                            std::string value_str = line.substr(delimiter_pos + 1);
                            // 【修改】解析 ev1 到 ev10
                            if (key.rfind("ev", 0) == 0) {
                                try {
                                    int index = std::stoi(key.substr(2)) - 1;
                                    if (index >= 0 && index < 10) {
                                        ev[index] = std::stod(value_str);
                                    }
                                }
                                catch (...) { /* 忽略格式錯誤的行 */ }
                            }
                        }
                    }
                }
                inFile.close();
                return true;
            }
        }

        // --- assign evX,=,value 的數值運算邏輯 ---
        std::wstringstream wss_numeric(fullCommand);
        std::vector<std::wstring> num_parts;
        if (std::getline(wss_numeric, part, L' ')) num_parts.push_back(part);
        if (std::getline(wss_numeric, part, L' ')) {
            std::wstringstream wss_params(part);
            while (std::getline(wss_params, part, L',')) num_parts.push_back(part);
        }
        if (num_parts.size() != 4 || _wcsicmp(num_parts[0].c_str(), L"assign") != 0) return false;

        // 【修改】解析 ev 後面的數字，存取陣列
        double* targetVariable = nullptr;
        std::wstring varName = num_parts[1];
        if (varName.rfind(L"ev", 0) == 0) {
            try {
                int index = std::stoi(varName.substr(2)) - 1;
                if (index >= 0 && index < 10) {
                    targetVariable = &ev[index];
                }
                else {
                    return false; // ev 編號超出範圍
                }
            }
            catch (...) {
                return false; // ev 編號格式錯誤
            }
        }
        else {
            return false; // 不是 ev 變數
        }

        if (!targetVariable) return false;

        std::wstring valStr = num_parts[3];
        try {
            double value = _wtof(valStr.c_str());
            wchar_t op = num_parts[2][0];
            switch (op) {
            case L'=': *targetVariable = value; break;
            case L'+': *targetVariable += value; break;
            case L'-': *targetVariable -= value; break;
            case L'*': *targetVariable *= value; break;
            case L'/': if (value != 0) *targetVariable /= value; break;
            case L'%': if (value != 0) *targetVariable = fmod(*targetVariable, value); break;
            default: return false;
            }
            return true;
        }
        catch (...) {
            return false;
        }
    }
    catch (...) {
        return false;
    }
}

// --- 重新加入之前新增的輔助函式 ---
static std::string WstringToString(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    int size_needed = WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), NULL, 0, NULL, NULL);
    std::string strTo(size_needed, 0);
    WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &strTo[0], size_needed, NULL, NULL);
    return strTo;
}

static void Trim(std::string& s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
        }));
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
        }).base(), s.end());
}


// --- 寵物檢查核心邏輯 ---
static ULONGLONG ParseUnicodeNumber(const wchar_t* str, int start, int len) {
    ULONGLONG result = 0;
    for (int i = 0; i < len; ++i) {
        wchar_t c = str[start + i];
        if (c >= L'0' && c <= L'9') {
            result = result * 10 + static_cast<ULONGLONG>(c - L'0');
        }
        else {
            return 0;
        }
    }
    return result;
}

// 【關鍵修正 1】修改函式簽名，增加 LPCWSTR pStringParam 參數
static int ProcessPetChecks(int checkType, const PetStats* stats, LPCWSTR pStringParam) {
    if (checkType == 1) { // 尾數檢查 (此部分邏輯不變)
        if ((stats->HP % 10 == 7) && (stats->ATK % 10 == 7) &&
            (stats->DEF % 10 == 7) && (stats->DEX % 10 == 7)) {
            return 7777;
        }
        return 9999;
    }

    if (checkType == 2) { // 四圍檢查
        // 【關鍵修正 2】移除硬編碼位址，改用傳入的參數
        if (IsBadReadPtr(pStringParam, sizeof(wchar_t))) return 6666; // 增加指標有效性檢查

        const wchar_t* targetString = pStringParam;
        if (wcslen(targetString) != 16) return 5555;

        ULONGLONG targetLv = ParseUnicodeNumber(targetString, 0, 3);
        ULONGLONG targetHP = ParseUnicodeNumber(targetString, 3, 4);
        ULONGLONG targetATK = ParseUnicodeNumber(targetString, 7, 3);
        ULONGLONG targetDEF = ParseUnicodeNumber(targetString, 10, 3);
        ULONGLONG targetDEX = ParseUnicodeNumber(targetString, 13, 3);

        if (targetLv * targetHP * targetATK * targetDEF * targetDEX == 0) return 6666;

        if ((stats->LV == targetLv) && (stats->HP >= targetHP) && (stats->ATK >= targetATK) &&
            (stats->DEF >= targetDEF) && (stats->DEX >= targetDEX)) {
            return 7777;
        }
        return 4444;
    }
    return 0;
}

// 【關鍵修正 3】修改函式簽名，增加 LPCWSTR pStringParam 參數
static void WriteCheckResult(int result, LPCWSTR pStringParam) {
    // 【關鍵修正 4】移除硬編碼位址，改用傳入的參數
    if (IsBadReadPtr(pStringParam, sizeof(wchar_t))) {
        return;
    }
    wchar_t buffer[17];
    swprintf_s(buffer, L"%d", result);
    OverwriteStringInMemory(pStringParam, buffer);
}

// =================================================================
// Hook 安裝與組合語言邏輯
// =================================================================

// 【優化】將重複的 Hook 安裝邏輯提取出來，做成一個共用函式
static bool InstallJmpHook(LPCWSTR moduleName, DWORD offset, LPVOID newFunction, size_t hookSize, LPVOID* returnAddress) {
    // 【修正】優先使用全域句柄，如果 moduleName 是 NULL
    HMODULE hMod = g_assaInfo.base;
    if (!hMod) {
        // 如果全域句柄不存在 (理論上不應發生)，再嘗試用名稱獲取
        if (moduleName) {
            hMod = GetModuleHandleW(moduleName);
        }
        else {
            return false;
        }
    }
    if (!hMod) return false;

    BYTE* pTarget = (BYTE*)hMod + offset;
    *returnAddress = pTarget + hookSize;

    DWORD oldProtect;
    if (VirtualProtect(pTarget, hookSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        pTarget[0] = 0xE9;
        *(DWORD*)(pTarget + 1) = (DWORD)((BYTE*)newFunction - pTarget - 5);
        for (size_t i = 5; i < hookSize; ++i) {
            pTarget[i] = 0x90;
        }
        VirtualProtect(pTarget, hookSize, oldProtect, &oldProtect);
        return true;
    }
    return false;
}

// --- 寵物檢查 Hook (Naked Function) ---
__declspec(naked) void MyPetCheckHook() {
    __asm {
        cmp dword ptr[eax], 0x88406EFF
        je check_max_hp
        cmp dword ptr[eax], 0x65785C3E
        je call_cpp_checker
        cmp dword ptr[eax], 0x570D56DB
        je call_cpp_checker

		push 0x004219D0 //原始彙編碼
        jmp gPetCheckHookReturnAddr

        check_max_hp :
        push eax
            call esi
            test eax, eax
            jne fail_path
            mov esi, [ebp - 0x14]
            mov edx, ds: [0x004F5204]
            lea eax, [esi + esi * 8]
            shl eax, 0x03
            sub eax, esi
            lea ecx, [eax + eax * 4]
            mov eax, [edx + ecx * 8 + 8]
            mov[ebp - 0x14], eax
            jmp success_path

            call_cpp_checker :
        push eax
            call esi
            test eax, eax
            jne fail_path
            mov esi, [ebp - 0x14]
            mov edx, ds: [0x004F5204]
            lea eax, [esi + esi * 8]
            shl eax, 0x03
            sub eax, esi
            lea ecx, [eax + eax * 4]

            sub esp, 20
            mov edi, esp

            mov eax, [edx + ecx * 8 + 0x1C]
            mov[edi + 0], eax
            mov eax, [edx + ecx * 8 + 8]
            mov[edi + 4], eax
            mov eax, [edx + ecx * 8 + 0x20]
            mov[edi + 8], eax
            mov eax, [edx + ecx * 8 + 0x24]
            mov[edi + 12], eax
            mov eax, [edx + ecx * 8 + 0x28]
            mov[edi + 16], eax

            mov eax, [ebp - 0x90]
            cmp dword ptr[eax], 0x65785C3E
            je is_last_digit

            // --- 四圍檢查 (stats check) ---
            // 【關鍵修正 5】將動態位址 [ebp+0x870] 作為第 3 個參數壓入堆疊
            push dword ptr[ebp + 0x870]
            push edi
            push 2
            jmp do_call

            is_last_digit :
        // --- 尾數檢查 (last digit) ---
        // 【關鍵修正 6】將動態位址 [ebp+0x870] 作為第 3 個參數壓入堆疊
        push dword ptr[ebp + 0x870]
            push edi
            push 1

            do_call :
            call ProcessPetChecks
            add esp, 12 // 清理 3 個參數 (4 * 3 = 12 bytes)

            // 將結果寫回
            push eax // 保存 ProcessPetChecks 的返回值 (7777, etc.)

            // 【關鍵修正 7】將動態位址 [ebp+0x870] 作為第 2 個參數壓入堆疊
            push dword ptr[ebp + 0x870]
            push eax // 將結果碼作為第 1 個參數壓入堆疊
            call WriteCheckResult
            add esp, 8 // 清理 2 個參數

            pop eax // 恢復 ProcessPetChecks 的返回值

            add esp, 20
            mov eax, 7777
            mov[ebp - 0x14], eax

            success_path :
        push 0x0048A119
            ret
            fail_path :
        push 0x0048A3A2
            ret
    }
}


// --- 隊伍檢查 Hook ---
__declspec(naked) void MyTeamCheckHook() {
    __asm {
        cmp dword ptr ds : [eax] , 0x4F0D968A // 檢查是否為隊伍檢查的特定標記
        je team_check_logic
        // 不匹配則走原路
        push 0x004219D0
        jmp gTeamCheckHookReturnAddr

        team_check_logic :
        push eax
            call esi
            test eax, eax
            push esi
            mov ecx, 1                       // 隊長計為1
            mov esi, ds: [0x004F523C]          // 隊伍資料基址

            // 依序檢查4個隊員欄位是否為0，不為0則計數器+1
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
            jle end_check
            inc ecx

            end_check :
        pop esi
            mov[ebp - 0x14], ecx           // 將隊伍人數存入目標變數
            push 0x00489DC3                  // 跳轉到成功路徑
            ret
    }
}

// --- 修復坐騎鎖定功能 ---
__declspec(naked) void MyMountHook() {
    __asm {
        mov cl, byte ptr ds : [0x004F665B]
        cmp cl, byte ptr ds : [0x004F51EC]
        je jump_to_fix                  // 如果坐騎ID匹配，則跳轉到修復路徑
        jmp gMountHookReturnAddr        // 否則走原路

        jump_to_fix :
        push 0x004687CA                 // 跳轉到另一段程式碼，繞過錯誤
            ret
    }
}

// --- Let指令 Hook ---
static void ProcessParameterHook(DWORD eax_value) {
    if (IsBadReadPtr((void*)eax_value, sizeof(LPCWSTR))) return;
    LPCWSTR* pString = (LPCWSTR*)eax_value;
    if (IsBadReadPtr(*pString, sizeof(wchar_t))) return;

    std::wstring lookupKey(*pString);
    std::transform(lookupKey.begin(), lookupKey.end(), lookupKey.begin(), ::tolower);

    // --- 【修改】重構 actionMap，動態處理 #ev1~10 ---
    if (lookupKey.rfind(L"#ev", 0) == 0) {
        try {
            int index = std::stoi(lookupKey.substr(3)) - 1;
            if (index >= 0 && index < 10) {
                wchar_t buf[32];
                swprintf_s(buf, L"%g", ev[index]);
                OverwriteStringInMemory(*pString, buf);
            }
        }
        catch (...) { /* 忽略格式錯誤 */ }
        return; // 處理完畢，直接返回
    }

    static const std::unordered_map<std::wstring, std::function<void(LPCWSTR)>> actionMap = {
        {L"#系統毫秒時間戳00", [](LPCWSTR addr) {
            FILETIME ft_now; GetSystemTimeAsFileTime(&ft_now);
            ULARGE_INTEGER uli_now = { ft_now.dwLowDateTime, ft_now.dwHighDateTime };
            ULONGLONG finalValue = (uli_now.QuadPart / 10000) % 7776000000ULL;
            wchar_t buffer[64]; swprintf_s(buffer, L"%llu", finalValue);
            OverwriteStringInMemory(addr, buffer);
        }},
        {L"#系統秒數時間戳", [](LPCWSTR addr) {
            FILETIME ft_now; GetSystemTimeAsFileTime(&ft_now);
            ULARGE_INTEGER uli_now = { ft_now.dwLowDateTime, ft_now.dwHighDateTime };
            ULONGLONG finalValue = (uli_now.QuadPart / 10000000) % 7776000ULL;
            wchar_t buffer[64]; swprintf_s(buffer, L"%llu", finalValue);
            OverwriteStringInMemory(addr, buffer);
        }},
        {L"#xpos", [](LPCWSTR addr) {
            if (!g_assaInfo.base) return;
            DWORD xposValue = *(DWORD*)((BYTE*)g_assaInfo.base + 0xF5078);
            wchar_t buffer[64]; swprintf_s(buffer, L"%d", xposValue);
            OverwriteStringInMemory(addr, buffer);
        }},
        {L"#ypos", [](LPCWSTR addr) {
            if (!g_assaInfo.base) return;
            DWORD yposValue = *(DWORD*)((BYTE*)g_assaInfo.base + 0xF507C);
            wchar_t buffer[64]; swprintf_s(buffer, L"%d", yposValue);
            OverwriteStringInMemory(addr, buffer);
        }},
        {L"#遊戲時間", [](LPCWSTR addr) {
            if (!g_saInfo.hProcess || !g_saInfo.base) return;
            DWORD gameTime = 0;
            if (ReadProcessMemory(g_saInfo.hProcess, (LPCVOID)((BYTE*)g_saInfo.base + Addr::GameTimeOffset), &gameTime, sizeof(gameTime), NULL)) {
                wchar_t buffer[64]; swprintf_s(buffer, L"%u", gameTime);
                OverwriteStringInMemory(addr, buffer);
            }
        }},
        {L"#npcid", [](LPCWSTR addr) {
            if (!g_saInfo.hProcess || !g_saInfo.base) return;
            DWORD windowState = 0;
            if (ReadProcessMemory(g_saInfo.hProcess, (LPCVOID)((BYTE*)g_saInfo.base + Addr::WindowStateOffset), &windowState, sizeof(windowState), NULL) && windowState == 1) {
                DWORD finalId = 0;
                if (ReadProcessMemory(g_saInfo.hProcess, (LPCVOID)((BYTE*)g_saInfo.base + Addr::NpcIdOffset), &finalId, sizeof(finalId), NULL)) {
                    wchar_t buffer[64]; swprintf_s(buffer, L"%u", finalId);
                    OverwriteStringInMemory(addr, buffer);
                }
            }
 else { OverwriteStringInMemory(addr, L"視窗未開啟"); }
}},
{L"#winid", [](LPCWSTR addr) {
    if (!g_saInfo.hProcess || !g_saInfo.base) return;
    DWORD windowState = 0;
    if (ReadProcessMemory(g_saInfo.hProcess, (LPCVOID)((BYTE*)g_saInfo.base + Addr::WindowStateOffset), &windowState, sizeof(windowState), NULL) && windowState == 1) {
        DWORD finalId = 0;
        if (ReadProcessMemory(g_saInfo.hProcess, (LPCVOID)((BYTE*)g_saInfo.base + Addr::WinIdOffset), &finalId, sizeof(finalId), NULL)) {
            wchar_t buffer[64]; swprintf_s(buffer, L"%u", finalId);
            OverwriteStringInMemory(addr, buffer);
        }
    }
else { OverwriteStringInMemory(addr, L"視窗未開啟"); }
}},
{L"#亂數", [](LPCWSTR addr) {
    static std::mt19937 gen(std::random_device{}());
    static std::uniform_int_distribution<> distrib(0, 10);
    wchar_t buf[16]; swprintf_s(buf, L"%d", distrib(gen));
    OverwriteStringInMemory(addr, buf);
}},
// 【新增】#Account 指令
{L"#account", [](LPCWSTR addr) {
    std::wstring accountName = GetAccountName();
    OverwriteStringInMemory(addr, accountName);
}}
    };

    auto it = actionMap.find(lookupKey);
    if (it != actionMap.end()) {
        it->second(*pString);
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
            *pValue = 0x5B9A786E; // 修改特定值以觸發腳本行為
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

// --- Button指令 Hook (猜謎) ---
void ProcessEcxString(LPCWSTR stringAddress) {
    if (stringAddress && !IsBadReadPtr(stringAddress, sizeof(wchar_t))) {
        if (wcscmp(stringAddress, L"從心所行之路即是正路") == 0) {
            std::wstring finalOutput = QA(); // 呼叫猜謎資料庫
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
        swprintf_s(timeString, L"%04d/%02d/%02d %02d:%02d:%02d", lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond);
        OverwriteStringInMemory(finalStringAddress, timeString);
    }
    else if (wcscmp(finalStringAddress, L"#計時開始") == 0) {
        scriptTime = time(NULL);
        OverwriteStringInMemory(finalStringAddress, L"計時開始");
    }
    else if (wcscmp(finalStringAddress, L"#計時停止") == 0) {
        if (scriptTime != 0) {
            time_t endTime = time(NULL);
            if (scriptTime > 31536000) { // 判斷是否為一個有效的時間戳
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
            swprintf_s(buffer, L"經過 %d時 %d分 %d秒", h, m, s);
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
static bool ProcessCommandChain(LPCWSTR keyString, LPCWSTR valueString) {
    if (IsBadReadPtr(keyString, sizeof(wchar_t)) || IsBadReadPtr(valueString, sizeof(wchar_t))) {
        return false;
    }
    int valueAsInt;
    try {
        valueAsInt = std::stoi(valueString);
    }
    catch (...) {
        return false;
    }

    struct CustomCommandRule {
        std::wstring keyString; int minValue; int maxValue; DWORD CheckboxOffset; DWORD ValueOffset;
    };
    static const std::vector<CustomCommandRule> rules = {
        { L"戰鬥精靈", 0, 5, 0xF61C6, 0xF61C7 }, { L"戰鬥精靈人物", 0,100, 0xF61C6, 0xF61F5 },
        { L"戰鬥精靈寵物", 0,100, 0xF61C6, 0xF61F6 }, { L"戰鬥補血", 0, 1, 0xF61C8, 0xF61C8 },
        { L"戰寵補血", 0, 3, 0xF61C9, 0xF61CA }, { L"戰寵補血人物", 0,100, 0xF61C9, 0xF61F9 },
        { L"戰寵補血寵物", 0,100, 0xF61C9, 0xF61FA }, { L"復活精靈", 0, 5, 0xF61FE, 0xF6200 },
        { L"復活物品", 0, 1, 0xF61FF, 0xF61FF }, { L"平時精靈", 0,20, 0xF6229, 0xF622A },
        { L"平時精靈人物", 0,100, 0xF6229, 0xF622B }, { L"平時精靈寵物", 0,100, 0xF6229, 0xF622C },
        { L"平時補血", 0, 1, 0xF622D, 0xF622D }, { L"平時技能", 0,26, 0xF6232, 0xF6233 },
        { L"寵物七技", 0, 1, 0xF6662, 0xF6662 }, { L"戰鬥換裝", 0, 1, 0xF6661, 0xF6661 },
        { L"戰鬥詳情", 0, 1, 0xF6669, 0xF6669 }, { L"白名單", 0, 1, 0xF53CC, 0xF53CC },
        { L"黑名單", 0, 1, 0xF53CD, 0xF53CD }, { L"郵件白名單", 0, 1, 0xF53B8, 0xF53B8 },
        { L"自動剔除", 0, 1, 0xF53CE, 0xF53CE }, { L"隊員1", 0, 1, 0xF53B9, 0xF53B9 },
        { L"隊員2", 0, 1, 0xF53BA, 0xF53BA }, { L"隊員3", 0, 1, 0xF53BB, 0xF53BB },
        { L"隊員4", 0, 1, 0xF53BC, 0xF53BC }, { L"隊員5", 0, 1, 0xF53BD, 0xF53BD },
    };

    HMODULE hMod = g_assaInfo.base;
    if (!hMod) return false;

    for (const auto& rule : rules) {
        if (rule.keyString == keyString && valueAsInt >= rule.minValue && valueAsInt <= rule.maxValue) {
            BYTE* pCheckbox = (BYTE*)hMod + rule.CheckboxOffset;
            *pCheckbox = (valueAsInt > 0) ? 1 : 0;

            if (valueAsInt > 0 && rule.maxValue > 1 && rule.CheckboxOffset != rule.ValueOffset) {
                BYTE* pValue = (BYTE*)hMod + rule.ValueOffset;

                // 【關鍵修正】根據 maxValue 是否小於 50 來決定是否要減 1
                if (rule.maxValue < 50) {
                    // 對於小範圍的下拉選單類型設定 (如精靈、技能)，數值減 1 以對應 0-based 索引
                    *pValue = valueAsInt - 1;
                }
                else {
                    // 對於大範圍的百分比類型設定 (如血量)，直接使用數值
                    *pValue = valueAsInt;
                }
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
        {L"戰鬥補氣", L"戰斗補氣"}, {L"走動遇敵", L"功能取消"}, {L"走動步數", L"功能取消"},
        {L"快速遇敵", L"功能取消"}, {L"快速延遲", L"功能取消"}, {L"自動猜謎", L"功能取消"}
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

// --- Assign 指令 Hook ---
__declspec(naked) void MyAssignHook() {
    __asm {
        mov edx, [ebp - 0x01A0]
        cmp[edx], 0x00530041
        jne original_path_assign

        pushad
        lea edx, [ebp - 0x1AF8]
        push edx
        call ProcessAssignStringCommand
        add esp, 4
        test al, al
        popad
        jz original_path_assign
        push 0x00483E5F
        ret

        original_path_assign :
            mov edx, [ebp - 0x01A0]
            jmp gAssignHookReturnAddr
    }
}

// --- 自動堆疊 (Auto Pile) Hook ---
static void ProcessAutoPile() {
    // 【修正】改為使用全域變數 g_assaInfo.base
    HMODULE hMod = g_assaInfo.base;
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
        L"/watch", L"/encount on", L"/encount off", L"/pile", L"/pile on",
        L"/pile off", L"前往冒險者之島"
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
    // 【修正】改為使用全域變數 g_assaInfo.base
    HMODULE hMod = g_assaInfo.base;
    if (!hMod) return;

    // 安裝常規 JMP Hook
    InstallJmpHook(L"Assa8.0B5.exe", Addr::CompareHookOffset, MyCompareHook, 6, (LPVOID*)&gCompareHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::AddEspHookOffset, MyAddEspHook, 5, (LPVOID*)&gAddEspHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::EcxStringHookOffset, MyEcxStringHook, 5, (LPVOID*)&gEcxStringHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::ParameterHookOffset, MyParameterHook, 5, (LPVOID*)&gParameterHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::PrintHookOffset, MyPrintHook, 5, (LPVOID*)&gPrintHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::Button2HookOffset, MyButton2Hook, 6, (LPVOID*)&gButton2HookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::IfitemHookOffset, MyIfitemHook, 6, (LPVOID*)&gIfitemHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::MountHookOffset, MyMountHook, 6, (LPVOID*)&gMountHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::TeamCheckHookOffset, MyTeamCheckHook, 5, (LPVOID*)&gTeamCheckHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::PetCheckHookOffset, MyPetCheckHook, 5, (LPVOID*)&gPetCheckHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::AssignHookOffset, MyAssignHook, 6, (LPVOID*)&gAssignHookReturnAddr);

    // --- 特殊 Hook 的安裝 (需要設定跳轉目標位址) ---
    gCommandChainJumpTarget = (void*)((BYTE*)hMod + Addr::CommandChainJumpTargetOffset);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::CommandChainHookOffset, MyCommandChainHook, 6, (LPVOID*)&gCommandChainHookReturnAddr);

    gAutoPileJumpTarget = (void*)((BYTE*)hMod + Addr::AutoPileJumpTargetOffset);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::AutoPileHookOffset, MyAutoPileHook, 5, (LPVOID*)&gAutoPileJumpTarget);

    DWORD* pFuncPtrAddr = (DWORD*)((BYTE*)hMod + Addr::StringCopyTargetPtr);
    g_pfnVbaStrCopy = (void*)*pFuncPtrAddr;
    InstallJmpHook(L"Assa8.0B5.exe", Addr::StringCopyHookOffset, MyStringCopyHook, 6, (LPVOID*)&gStringCopyHookReturnAddr);
}