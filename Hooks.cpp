// �ɮ�: Hooks.cpp
// ����: ���ɮשw�q�F�Ҧ��� Hook �禡�H�Φw�˥��̪��޿�C
//       "Hook" (���_) ������O�b�{�������Y�ӯS�w��m�ɡA
//       �j�����Ӱ���ڭ̦ۤv�g���{���X�A������A���^�h�~�����C

#include "pch.h"
#include "Hooks.h"
#include "Globals.h"
#include "helpers.h"
#include "RiddleDatabase.h"

// --- �R�W�Ŷ��P�`�Ʃw�q ---
namespace Addr {
    // �o�ǬO Assa8.0B5.exe �{�����A�ڭ̷Q�n Hook ����m�۹��Ҳհ򩳦�}�������q�C
    // �o�ǭȬO�z�L�f�V�u�{�u�� (�p x64dbg, IDA) ���R Assa �{���o�쪺�C

    // �D�{�� (Assa) Hook ��������
    constexpr DWORD CompareHookOffset = 0x71972;     // �۰ʵo�ܤ���I
    constexpr DWORD AddEspHookOffset = 0x71AD5;     // �۰ʵo�ܰT���M���I
    constexpr DWORD EcxStringHookOffset = 0x7AA0B;     // �B�z "Button" ���O���I (�ڭ̥Ψ�Ĳ�o�q��)
    constexpr DWORD ParameterHookOffset = 0x77E80;     // �B�z "Let" ���O���I (�Ψӹ�{�ۭq�ܼ�)
    constexpr DWORD PrintHookOffset = 0x76269;     // �B�z "Print" ���O���I (�Ψӹ�{�p�ɾ�)
    constexpr DWORD Button2HookOffset = 0x7A472;     // �B�z "Button2" (�T�w���s) ���I
    constexpr DWORD IfitemHookOffset = 0x88AE3;     // �B�z "Ifitem" ���O���I
    constexpr DWORD CommandChainHookOffset = 0x820B2;     // �B�z "Set" ���O���I (�Ψӳ]�w�~�������Ѽ�)
    constexpr DWORD CommandChainJumpTargetOffset = 0x83E66;   // Set���O���\����઺��}
    constexpr DWORD StringCopyHookOffset = 0x7EBDB;     // �B�z "Set2" ���O���I (�ΨӴ����r��)
    constexpr DWORD StringCopyTargetPtr = 0x1220;      // Set2���O��I�s�禡�����Ц�}
    constexpr DWORD AssignHookOffset = 0x76DC0;     // �B�z "Assign" ���O

    // �۰ʰ��|��������
    constexpr DWORD AutoPileHookOffset = 0x6CC7E;     // �۰ʰ��|��Ĳ�o�I
    constexpr DWORD AutoPileSwitchOffset = 0xF6668;     // �۰ʰ��|�\�઺�}����}
    constexpr DWORD AutoPileJumpTargetOffset = 0x6D399;     // �۰ʰ��|Hook��������^����}

    // ���M��w�\��״_
    constexpr DWORD MountHookOffset = 0x686DD;     // �״_���M��w�\��

    // �����ˬd�\��
    constexpr DWORD TeamCheckHookOffset = 0x89B3A;     // �P�_����A

    // �d���ˬd�\��
    constexpr DWORD PetCheckHookOffset = 0x8A370;     // �d���̤j��q�B�|��B����777�ˬd

    // �ؼйC�� (sa_8002a.exe) �O���鰾��
    constexpr DWORD GameTimeOffset = 0x1AD760;    // �C���ɶ�
    constexpr DWORD WindowStateOffset = 0x1696CC;    // �C���������A (�Ω�P�_NPC��ܵ��O�_�}��)
    constexpr DWORD NpcIdOffset = 0xAFA34;     // NPC ID
    constexpr DWORD WinIdOffset = 0xAFA30;     // ���� ID
	constexpr DWORD AccountNameOffset = 0x4AC28B8;  // �b���W�� 
	constexpr DWORD CharacterNameOffset = 0x422DF18;    // ����W�� 
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
BYTE* gMountHookReturnAddr = nullptr;
BYTE* gTeamCheckHookReturnAddr = nullptr;
BYTE* gPetCheckHookReturnAddr = nullptr;
BYTE* gAssignHookReturnAddr = nullptr;

void* g_pfnVbaStrCopy = nullptr;
void* gAutoPileJumpTarget = nullptr;
void* gCommandChainJumpTarget = nullptr;

// =================================================================
// C++ �֤��޿�禡
// =================================================================
// --- �r��B�z���U�禡 ---
static std::string WstringToString(const std::wstring& wstr);
static void Trim(std::string& s);

// �i�s�W�j�W�ߥX�@������b���W�٪��禡�A��K�@��
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
    std::wstring accountName = GetAccountName(); // �����I�s�s�禡
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
// --- Assign ���O�֤��޿� ---
static bool ProcessAssignStringCommand(LPCWSTR finalStringAddress) {
    try {
        if (IsBadReadPtr(finalStringAddress, sizeof(wchar_t))) return false;

        std::wstring fullCommand(finalStringAddress);
        std::wstringstream wss(fullCommand);
        std::vector<std::wstring> parts;
        std::wstring part;

        while (wss >> part) parts.push_back(part);

        // --- save �M load �޿� ---
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

                // �i�ק�j�ϥΰj���x�s�Ҧ� ev �ܼ�
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
                    for (int i = 0; i < 10; ++i) ev[i] = 0.0; // �ɮפ��s�b�A���m�Ҧ��ܼ�
                    return true;
                }

                std::string line;
                bool in_correct_section = false;
                for (int i = 0; i < 10; ++i) ev[i] = 0.0; // ���w�]���m

                while (std::getline(inFile, line)) {
                    Trim(line);
                    if (line == sectionHeader) {
                        in_correct_section = true;
                        continue;
                    }
                    if (line.empty() || line[0] == '[') {
                        if (in_correct_section) break; // �wŪ���Ӱ϶��A�i��������
                        in_correct_section = false;
                        continue;
                    }
                    if (in_correct_section) {
                        size_t delimiter_pos = line.find('=');
                        if (delimiter_pos != std::string::npos) {
                            std::string key = line.substr(0, delimiter_pos);
                            std::string value_str = line.substr(delimiter_pos + 1);
                            // �i�ק�j�ѪR ev1 �� ev10
                            if (key.rfind("ev", 0) == 0) {
                                try {
                                    int index = std::stoi(key.substr(2)) - 1;
                                    if (index >= 0 && index < 10) {
                                        ev[index] = std::stod(value_str);
                                    }
                                }
                                catch (...) { /* �����榡���~���� */ }
                            }
                        }
                    }
                }
                inFile.close();
                return true;
            }
        }

        // --- assign evX,=,value ���ƭȹB���޿� ---
        std::wstringstream wss_numeric(fullCommand);
        std::vector<std::wstring> num_parts;
        if (std::getline(wss_numeric, part, L' ')) num_parts.push_back(part);
        if (std::getline(wss_numeric, part, L' ')) {
            std::wstringstream wss_params(part);
            while (std::getline(wss_params, part, L',')) num_parts.push_back(part);
        }
        if (num_parts.size() != 4 || _wcsicmp(num_parts[0].c_str(), L"assign") != 0) return false;

        // �i�ק�j�ѪR ev �᭱���Ʀr�A�s���}�C
        double* targetVariable = nullptr;
        std::wstring varName = num_parts[1];
        if (varName.rfind(L"ev", 0) == 0) {
            try {
                int index = std::stoi(varName.substr(2)) - 1;
                if (index >= 0 && index < 10) {
                    targetVariable = &ev[index];
                }
                else {
                    return false; // ev �s���W�X�d��
                }
            }
            catch (...) {
                return false; // ev �s���榡���~
            }
        }
        else {
            return false; // ���O ev �ܼ�
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

// --- ���s�[�J���e�s�W�����U�禡 ---
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


// --- �d���ˬd�֤��޿� ---
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

// �i����ץ� 1�j�ק�禡ñ�W�A�W�[ LPCWSTR pStringParam �Ѽ�
static int ProcessPetChecks(int checkType, const PetStats* stats, LPCWSTR pStringParam) {
    if (checkType == 1) { // �����ˬd (�������޿褣��)
        if ((stats->HP % 10 == 7) && (stats->ATK % 10 == 7) &&
            (stats->DEF % 10 == 7) && (stats->DEX % 10 == 7)) {
            return 7777;
        }
        return 9999;
    }

    if (checkType == 2) { // �|���ˬd
        // �i����ץ� 2�j�����w�s�X��}�A��ζǤJ���Ѽ�
        if (IsBadReadPtr(pStringParam, sizeof(wchar_t))) return 6666; // �W�[���Ц��ĩ��ˬd

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

// �i����ץ� 3�j�ק�禡ñ�W�A�W�[ LPCWSTR pStringParam �Ѽ�
static void WriteCheckResult(int result, LPCWSTR pStringParam) {
    // �i����ץ� 4�j�����w�s�X��}�A��ζǤJ���Ѽ�
    if (IsBadReadPtr(pStringParam, sizeof(wchar_t))) {
        return;
    }
    wchar_t buffer[17];
    swprintf_s(buffer, L"%d", result);
    OverwriteStringInMemory(pStringParam, buffer);
}

// =================================================================
// Hook �w�˻P�զX�y���޿�
// =================================================================

// �i�u�ơj�N���ƪ� Hook �w���޿责���X�ӡA�����@�Ӧ@�Ψ禡
static bool InstallJmpHook(LPCWSTR moduleName, DWORD offset, LPVOID newFunction, size_t hookSize, LPVOID* returnAddress) {
    // �i�ץ��j�u���ϥΥ���y�`�A�p�G moduleName �O NULL
    HMODULE hMod = g_assaInfo.base;
    if (!hMod) {
        // �p�G����y�`���s�b (�z�פW�����o��)�A�A���եΦW�����
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

// --- �d���ˬd Hook (Naked Function) ---
__declspec(naked) void MyPetCheckHook() {
    __asm {
        cmp dword ptr[eax], 0x88406EFF
        je check_max_hp
        cmp dword ptr[eax], 0x65785C3E
        je call_cpp_checker
        cmp dword ptr[eax], 0x570D56DB
        je call_cpp_checker

		push 0x004219D0 //��l�J�s�X
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

            // --- �|���ˬd (stats check) ---
            // �i����ץ� 5�j�N�ʺA��} [ebp+0x870] �@���� 3 �ӰѼ����J���|
            push dword ptr[ebp + 0x870]
            push edi
            push 2
            jmp do_call

            is_last_digit :
        // --- �����ˬd (last digit) ---
        // �i����ץ� 6�j�N�ʺA��} [ebp+0x870] �@���� 3 �ӰѼ����J���|
        push dword ptr[ebp + 0x870]
            push edi
            push 1

            do_call :
            call ProcessPetChecks
            add esp, 12 // �M�z 3 �ӰѼ� (4 * 3 = 12 bytes)

            // �N���G�g�^
            push eax // �O�s ProcessPetChecks ����^�� (7777, etc.)

            // �i����ץ� 7�j�N�ʺA��} [ebp+0x870] �@���� 2 �ӰѼ����J���|
            push dword ptr[ebp + 0x870]
            push eax // �N���G�X�@���� 1 �ӰѼ����J���|
            call WriteCheckResult
            add esp, 8 // �M�z 2 �ӰѼ�

            pop eax // ��_ ProcessPetChecks ����^��

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


// --- �����ˬd Hook ---
__declspec(naked) void MyTeamCheckHook() {
    __asm {
        cmp dword ptr ds : [eax] , 0x4F0D968A // �ˬd�O�_�������ˬd���S�w�аO
        je team_check_logic
        // ���ǰt�h�����
        push 0x004219D0
        jmp gTeamCheckHookReturnAddr

        team_check_logic :
        push eax
            call esi
            test eax, eax
            push esi
            mov ecx, 1                       // �����p��1
            mov esi, ds: [0x004F523C]          // �����ư�}

            // �̧��ˬd4�Ӷ������O�_��0�A����0�h�p�ƾ�+1
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
            mov[ebp - 0x14], ecx           // �N����H�Ʀs�J�ؼ��ܼ�
            push 0x00489DC3                  // ����즨�\���|
            ret
    }
}

// --- �״_���M��w�\�� ---
__declspec(naked) void MyMountHook() {
    __asm {
        mov cl, byte ptr ds : [0x004F665B]
        cmp cl, byte ptr ds : [0x004F51EC]
        je jump_to_fix                  // �p�G���MID�ǰt�A�h�����״_���|
        jmp gMountHookReturnAddr        // �_�h�����

        jump_to_fix :
        push 0x004687CA                 // �����t�@�q�{���X�A¶�L���~
            ret
    }
}

// --- Let���O Hook ---
static void ProcessParameterHook(DWORD eax_value) {
    if (IsBadReadPtr((void*)eax_value, sizeof(LPCWSTR))) return;
    LPCWSTR* pString = (LPCWSTR*)eax_value;
    if (IsBadReadPtr(*pString, sizeof(wchar_t))) return;

    std::wstring lookupKey(*pString);
    std::transform(lookupKey.begin(), lookupKey.end(), lookupKey.begin(), ::tolower);

    // --- �i�ק�j���c actionMap�A�ʺA�B�z #ev1~10 ---
    if (lookupKey.rfind(L"#ev", 0) == 0) {
        try {
            int index = std::stoi(lookupKey.substr(3)) - 1;
            if (index >= 0 && index < 10) {
                wchar_t buf[32];
                swprintf_s(buf, L"%g", ev[index]);
                OverwriteStringInMemory(*pString, buf);
            }
        }
        catch (...) { /* �����榡���~ */ }
        return; // �B�z�����A������^
    }

    static const std::unordered_map<std::wstring, std::function<void(LPCWSTR)>> actionMap = {
        {L"#�t�β@��ɶ��W00", [](LPCWSTR addr) {
            FILETIME ft_now; GetSystemTimeAsFileTime(&ft_now);
            ULARGE_INTEGER uli_now = { ft_now.dwLowDateTime, ft_now.dwHighDateTime };
            ULONGLONG finalValue = (uli_now.QuadPart / 10000) % 7776000000ULL;
            wchar_t buffer[64]; swprintf_s(buffer, L"%llu", finalValue);
            OverwriteStringInMemory(addr, buffer);
        }},
        {L"#�t�ά�Ʈɶ��W", [](LPCWSTR addr) {
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
        {L"#�C���ɶ�", [](LPCWSTR addr) {
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
 else { OverwriteStringInMemory(addr, L"�������}��"); }
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
else { OverwriteStringInMemory(addr, L"�������}��"); }
}},
{L"#�ü�", [](LPCWSTR addr) {
    static std::mt19937 gen(std::random_device{}());
    static std::uniform_int_distribution<> distrib(0, 10);
    wchar_t buf[16]; swprintf_s(buf, L"%d", distrib(gen));
    OverwriteStringInMemory(addr, buf);
}},
// �i�s�W�j#Account ���O
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

// --- Button�T�w���O Hook ---
void ProcessButton2Hook(DWORD edx_value) {
    DWORD* pValue = (DWORD*)edx_value;
    if (!IsBadReadPtr(pValue, sizeof(DWORD)) && *pValue == 0x5B9A78BA) {
        DWORD oldProtect;
        if (VirtualProtect(pValue, sizeof(DWORD), PAGE_READWRITE, &oldProtect)) {
            *pValue = 0x5B9A786E; // �ק�S�w�ȥHĲ�o�}���欰
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

// --- Button���O Hook (�q��) ---
void ProcessEcxString(LPCWSTR stringAddress) {
    if (stringAddress && !IsBadReadPtr(stringAddress, sizeof(wchar_t))) {
        if (wcscmp(stringAddress, L"�q�ߩҦ椧���Y�O����") == 0) {
            std::wstring finalOutput = QA(); // �I�s�q����Ʈw
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
        swprintf_s(timeString, L"%04d/%02d/%02d %02d:%02d:%02d", lt.wYear, lt.wMonth, lt.wDay, lt.wHour, lt.wMinute, lt.wSecond);
        OverwriteStringInMemory(finalStringAddress, timeString);
    }
    else if (wcscmp(finalStringAddress, L"#�p�ɶ}�l") == 0) {
        scriptTime = time(NULL);
        OverwriteStringInMemory(finalStringAddress, L"�p�ɶ}�l");
    }
    else if (wcscmp(finalStringAddress, L"#�p�ɰ���") == 0) {
        if (scriptTime != 0) {
            time_t endTime = time(NULL);
            if (scriptTime > 31536000) { // �P�_�O�_���@�Ӧ��Ī��ɶ��W
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
            swprintf_s(buffer, L"�g�L %d�� %d�� %d��", h, m, s);
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
        { L"�԰����F", 0, 5, 0xF61C6, 0xF61C7 }, { L"�԰����F�H��", 0,100, 0xF61C6, 0xF61F5 },
        { L"�԰����F�d��", 0,100, 0xF61C6, 0xF61F6 }, { L"�԰��ɦ�", 0, 1, 0xF61C8, 0xF61C8 },
        { L"���d�ɦ�", 0, 3, 0xF61C9, 0xF61CA }, { L"���d�ɦ�H��", 0,100, 0xF61C9, 0xF61F9 },
        { L"���d�ɦ��d��", 0,100, 0xF61C9, 0xF61FA }, { L"�_�����F", 0, 5, 0xF61FE, 0xF6200 },
        { L"�_�����~", 0, 1, 0xF61FF, 0xF61FF }, { L"���ɺ��F", 0,20, 0xF6229, 0xF622A },
        { L"���ɺ��F�H��", 0,100, 0xF6229, 0xF622B }, { L"���ɺ��F�d��", 0,100, 0xF6229, 0xF622C },
        { L"���ɸɦ�", 0, 1, 0xF622D, 0xF622D }, { L"���ɧޯ�", 0,26, 0xF6232, 0xF6233 },
        { L"�d���C��", 0, 1, 0xF6662, 0xF6662 }, { L"�԰�����", 0, 1, 0xF6661, 0xF6661 },
        { L"�԰��Ա�", 0, 1, 0xF6669, 0xF6669 }, { L"�զW��", 0, 1, 0xF53CC, 0xF53CC },
        { L"�¦W��", 0, 1, 0xF53CD, 0xF53CD }, { L"�l��զW��", 0, 1, 0xF53B8, 0xF53B8 },
        { L"�۰ʭ簣", 0, 1, 0xF53CE, 0xF53CE }, { L"����1", 0, 1, 0xF53B9, 0xF53B9 },
        { L"����2", 0, 1, 0xF53BA, 0xF53BA }, { L"����3", 0, 1, 0xF53BB, 0xF53BB },
        { L"����4", 0, 1, 0xF53BC, 0xF53BC }, { L"����5", 0, 1, 0xF53BD, 0xF53BD },
    };

    HMODULE hMod = g_assaInfo.base;
    if (!hMod) return false;

    for (const auto& rule : rules) {
        if (rule.keyString == keyString && valueAsInt >= rule.minValue && valueAsInt <= rule.maxValue) {
            BYTE* pCheckbox = (BYTE*)hMod + rule.CheckboxOffset;
            *pCheckbox = (valueAsInt > 0) ? 1 : 0;

            if (valueAsInt > 0 && rule.maxValue > 1 && rule.CheckboxOffset != rule.ValueOffset) {
                BYTE* pValue = (BYTE*)hMod + rule.ValueOffset;

                // �i����ץ��j�ھ� maxValue �O�_�p�� 50 �ӨM�w�O�_�n�� 1
                if (rule.maxValue < 50) {
                    // ���p�d�򪺤U�Կ�������]�w (�p���F�B�ޯ�)�A�ƭȴ� 1 �H���� 0-based ����
                    *pValue = valueAsInt - 1;
                }
                else {
                    // ���j�d�򪺦ʤ��������]�w (�p��q)�A�����ϥμƭ�
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

// ---set2���O (�r�����) Hook ---
void ProcessStringCopyHook(LPCWSTR stringAddress) {
    if (IsBadReadPtr(stringAddress, sizeof(wchar_t))) return;
    static const std::unordered_map<std::wstring, std::wstring> replacements = {
        {L"�M��", L"�M��"}, {L"�۰ʹq�����", L"�۰�KNPC"}, {L"�۰ʾ԰�", L"�۰ʾԤ�"},
        {L"�ֳt�԰�", L"�ֳt�Ԥ�"}, {L"�n�J�D��", L"�n���D��"}, {L"�n�J�ƾ�", L"�n���ƾ�"},
        {L"�n�J�H��", L"�n���H��"}, {L"�۰ʵn�J", L"�۰ʵn��"}, {L"�}������", L"�}������"},
        {L"�԰��ɮ�", L"�Ԥ�ɮ�"}, {L"���ʹJ��", L"�\�����"}, {L"���ʨB��", L"�\�����"},
        {L"�ֳt�J��", L"�\�����"}, {L"�ֳt����", L"�\�����"}, {L"�۰ʲq��", L"�\�����"}
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

// --- Assign ���O Hook ---
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

// --- �۰ʰ��| (Auto Pile) Hook ---
static void ProcessAutoPile() {
    // �i�ץ��j�אּ�ϥΥ����ܼ� g_assaInfo.base
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

// --- �۰ʵo��(Compare) Hook ---
int ShouldDoCustomJump() {
    LPCWSTR commandString = (LPCWSTR)0x4F53D1;
    if (IsBadReadPtr(commandString, sizeof(wchar_t))) return 0;
    static const std::vector<std::wstring> commandsToMatch = {
        L"/status", L"/accompany", L"/eo", L"/offline on", L"/offline off",
        L"/watch", L"/encount on", L"/encount off", L"/pile", L"/pile on",
        L"/pile off", L"�e���_�I�̤��q"
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

// --- �۰ʵo�ܰT���M�� Hook ---
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

// --- ���}���`�w�˨禡 ---
void InstallAllHooks() {
    // �i�ץ��j�אּ�ϥΥ����ܼ� g_assaInfo.base
    HMODULE hMod = g_assaInfo.base;
    if (!hMod) return;

    // �w�˱`�W JMP Hook
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

    // --- �S�� Hook ���w�� (�ݭn�]�w����ؼЦ�}) ---
    gCommandChainJumpTarget = (void*)((BYTE*)hMod + Addr::CommandChainJumpTargetOffset);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::CommandChainHookOffset, MyCommandChainHook, 6, (LPVOID*)&gCommandChainHookReturnAddr);

    gAutoPileJumpTarget = (void*)((BYTE*)hMod + Addr::AutoPileJumpTargetOffset);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::AutoPileHookOffset, MyAutoPileHook, 5, (LPVOID*)&gAutoPileJumpTarget);

    DWORD* pFuncPtrAddr = (DWORD*)((BYTE*)hMod + Addr::StringCopyTargetPtr);
    g_pfnVbaStrCopy = (void*)*pFuncPtrAddr;
    InstallJmpHook(L"Assa8.0B5.exe", Addr::StringCopyHookOffset, MyStringCopyHook, 6, (LPVOID*)&gStringCopyHookReturnAddr);
}