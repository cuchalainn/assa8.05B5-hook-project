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
#include <string>      // �i�s�W�j�Ω�B�z�r�ꪫ��
#include <algorithm>   // �i�s�W�j�Ω�r���ഫ
#include <cctype>      // �i�s�W�j�Ω�r���j�p�g�ഫ
#include <cmath> // �i�s�W�j�Ω� fmod �禡 (�B�I�ƨ��l)
#include <vector> // �i�s�W�j�Ω�r�����
#include <sstream> // �i�s�W�j�Ω�r�����
#include <fstream> // �i�s�W�j�Ω��ɮ�Ū�g (fstream)

// =================================================================
// Assign ���O�֤��޿� (�ץ��� - �䴩 assign save/load)
// =================================================================
bool ProcessAssignStringCommand() {
    try {
        // 1. �q�T�w��}Ū�����O�r��
        LPCWSTR finalStringAddress = (LPCWSTR)0x0019DF9C;
        if (IsBadReadPtr(finalStringAddress, sizeof(wchar_t))) {
            return false;
        }
        std::wstring fullCommand(finalStringAddress);

        // 2. �u�����եΪŮ���ΡA�ˬd�O�_�� save/load ���O
        std::wstringstream wss(fullCommand);
        std::vector<std::wstring> space_parts;
        std::wstring part;
        while (wss >> part) { // ���Ů���ΩҦ�����
            space_parts.push_back(part);
        }

        // �ˬd�O�_�� "assign save" �� "assign load"
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
                        //MessageBoxW(NULL, L"�Ҧ� ev �ܼƤw���\�O�s�� ev_vars.txt", L"Assign SAVE", MB_OK | MB_ICONINFORMATION);
                        return true;
                    }
                }
                else if (action == L"load") {
                    std::ifstream inFile(filename);
                    if (inFile.is_open()) {
                        inFile >> ev1 >> ev2 >> ev3;
                        inFile.close();
                        wchar_t msg[256];
                        //swprintf_s(msg, 256, L"�w�q ev_vars.txt ���\Ū���ܼ�:\n- ev1: %g\n- ev2: %g\n- ev3: %g", ev1, ev2, ev3);
                        //MessageBoxW(NULL, msg, L"Assign LOAD", MB_OK | MB_ICONINFORMATION);
                        return true;
                    }
                }
            }
        }


        // 3. �p�G���O save/load�A�h���ոѪR�즳���ƭȹB����O
        // �榡: assign evX,=,100
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

        // ... (���򪺼ƭȹB���޿觹������) ...
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
        //swprintf_s(successMsg, 256, L"�ܼ� %s ���Ȥw��s��: %g", varName.c_str(), *targetVariable);
        //MessageBoxW(NULL, successMsg, L"Assign ���O���\", MB_OK | MB_ICONINFORMATION);

        return true;
    }
    catch (...) {
        return false;
    }
}

// =================================================================
// �d���ˬd�֤��޿� (C++ ��{)
// =================================================================

// �N�@�ӳ̦h16�Ӧr����Unicode�Ʀr�r��A�q���w��m�}�l�ഫ��64�줸���
ULONGLONG ParseUnicodeNumber(const wchar_t* str, int start, int len) {
    ULONGLONG result = 0;
    for (int i = 0; i < len; ++i) {
        wchar_t c = str[start + i];
        if (c >= L'0' && c <= L'9') {
            result = result * 10 + (c - L'0');
        }
        else {
            // �p�G�J��D�Ʀr�r���A�i�H�����L��
            return 0;
        }
    }
    return result;
}


// �B�z�d�� "����" �M "�|��" �ˬd���D�n�禡
// checkType: 1=�����ˬd, 2=�|���ˬd
// petStats: �]�t�d����e LV, HP, ATK, DEF, DEX �����c����
// returns: ���\�h��^ 7777�A���ѫh��^���������~�X (6666 �� 9999)
int ProcessPetChecks(int checkType, const PetStats* stats) {
    if (checkType == 1) { // --- �����ˬd ---
        if ((stats->HP % 10 == 7) &&
            (stats->ATK % 10 == 7) &&
            (stats->DEF % 10 == 7) &&
            (stats->DEX % 10 == 7)) {
            return 7777; // �����ŦX�A���\
        }
        return 9999; // �ܤ֤@�Ӥ��ŦX�A����
    }

    if (checkType == 2) { // --- �|���ˬd ---
        LPCWSTR* pOuterString = (LPCWSTR*)0x0019FA58;
        LPCWSTR targetString = *pOuterString;
        // �ˬd�r����׬O�_��16
        if (wcslen(targetString) != 16) {
            return 5555;
        }
        // �q�r��ѪR�X�ؼмƭ�
        ULONGLONG targetLv = ParseUnicodeNumber(targetString, 0, 3);
        ULONGLONG targetHP = ParseUnicodeNumber(targetString, 3, 4);
        ULONGLONG targetATK = ParseUnicodeNumber(targetString, 7, 3);
        ULONGLONG targetDEF = ParseUnicodeNumber(targetString, 10, 3);
        ULONGLONG targetDEX = ParseUnicodeNumber(targetString, 13, 3);
        // �ˬd�O�_������@�ӼƭȸѪR��0�A�Y���h�����L�Ĩê�^
        if (targetLv * targetHP * targetATK * targetDEF * targetDEX == 0) {
            return 6666;
        }
        // �i����
        if ((stats->LV == targetLv) &&
            (stats->HP >= targetHP) &&
            (stats->ATK >= targetATK) &&
            (stats->DEF >= targetDEF) &&
            (stats->DEX >= targetDEX)) {
            return 7777; // �����q�L
        }
        return 4444; // �ܤ֤@�����q�L
    }

    return 0; // �������ˬd����
}

// �N���G�Ʀr�g�^�ؼЦr��O����
void WriteCheckResult(int result) {
    LPCWSTR* pOuterString = (LPCWSTR*)0x0019FA58;
    if (IsBadReadPtr(pOuterString, sizeof(LPCWSTR)) || IsBadReadPtr(*pOuterString, sizeof(wchar_t))) {
        return;
    }
    wchar_t buffer[17]; // 16�Ӧr�� + ������
    swprintf_s(buffer, L"%d", result);
    OverwriteStringInMemory(*pOuterString, buffer);
}

// --- �R�W�Ŷ��P�`�Ʃw�q ---
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
    // �i�s�W�jAssign ���O Hook �����q
    constexpr DWORD AssignHookOffset = 0x76DC0;
    // �۰ʰ��|��������
    constexpr DWORD AutoPileHookOffset = 0x6CC7E;        // �۰ʰ��|��Ĳ�o�I
    constexpr DWORD AutoPileSwitchOffset = 0xF6668;     // �۰ʰ��|�\�઺�}����}
    constexpr DWORD AutoPileJumpTargetOffset = 0x6D399; // �۰ʰ��|Hook��������^����}

    // ���M��w�\��״_
    constexpr DWORD MountHookOffset = 0x686DD;          // �״_���M��w�\��

    // �����ˬd�\��
    constexpr DWORD TeamCheckHookOffset = 0x89B3A;      // �P�_����A

    // �i�s�W�j�d���ˬd�\��
    constexpr DWORD PetCheckHookOffset = 0x8A370;       // �d���̤j��q�B�|��B����777�ˬd

    // �ؼйC�� (sa_8002a.exe) �O���鰾��
    constexpr DWORD GameTimeOffset = 0x1AD760;          // �C���ɶ�
    constexpr DWORD WindowStateOffset = 0x1696CC;       // �C���������A (�Ω�P�_NPC��ܵ��O�_�}��)
    constexpr DWORD NpcIdOffset = 0xAFA34;              // NPC ID
    constexpr DWORD WinIdOffset = 0xAFA30;              // ���� ID
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
BYTE* gMountHookReturnAddr = nullptr;
BYTE* gTeamCheckHookReturnAddr = nullptr;
// �i�s�W�j�d���ˬdHook����^��}
BYTE* gPetCheckHookReturnAddr = nullptr;
// �i�s�W�jAssign ���O Hook ��^��}
BYTE* gAssignHookReturnAddr = nullptr;


// �i�u�ơj�N���ƪ� Hook �w���޿责���X�ӡA�����@�Ӧ@�Ψ禡
static bool InstallJmpHook(LPCWSTR moduleName, DWORD offset, LPVOID newFunction, size_t hookSize, LPVOID* returnAddress) {
    HMODULE hMod = GetModuleHandleW(moduleName);
    if (!hMod) {
        return false;
    }

    BYTE* pTarget = (BYTE*)hMod + offset;
    *returnAddress = pTarget + hookSize;

    DWORD oldProtect;
    if (VirtualProtect(pTarget, hookSize, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        pTarget[0] = 0xE9; // JMP ���O
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

// --- �d���̤j��q�B�|��B����777�ˬd (���c��) ---
__declspec(naked) void MyPetCheckHook() {
    __asm {
        // eax �O�� Hook �I���e�� mov eax,[ebp-0x90] �ҳ]�w
        // �ھ� eax ���V���ȡA�P�_������Ӥ���
        // �ˬd�O�_�� "����"
        cmp dword ptr[eax], 0x88406EFF
        je check_max_hp_asm
        // �ˬd�O�_�� "����"
        cmp dword ptr[eax], 0x65785C3E
        je call_cpp_checker
        // �ˬd�O�_�� "�|��"
        cmp dword ptr[eax], 0x570D56DB
        je call_cpp_checker
        // �p�G�����O�A�h�����l���O�ê�^
        original_path :
        push 0x004219D0
            jmp gPetCheckHookReturnAddr

        check_max_hp_asm :// --- �����޿� (�²զX�y��) ---
            push eax
            call esi
            test eax, eax
            jne fail_jump // �p�G call �����G���� 0�A����쥢�Ѹ��|
            mov esi, [ebp - 0x14]
            mov edx, ds: [0x004F5204] //�d���ƭȰ�}
            lea eax, [esi + esi * 8]
            shl eax, 0x03
            sub eax, esi
            lea ecx, [eax + eax * 4]
            mov eax, [edx + ecx * 8 + 8] // Ū���d�������
            mov [ebp - 0x14],eax  // �N���G�s�J�ؼ��ܼ�
            jmp success_path_asm

        call_cpp_checker :// --- ���� & �|���޿� (�I�s C++) ---
            push eax
            call esi
            test eax, eax
            jne fail_jump // �p�G call �����G���� 0�A����
            mov esi, [ebp - 0x14]
            mov edx, ds: [0x004F5204] //�d���ƭȰ�}
            lea eax, [esi + esi * 8]
            shl eax, 0x03
            sub eax, esi
            lea ecx, [eax + eax * 4]
            // �ǳƵ��c��M�ѼƥH�I�s C++ �禡
            sub esp, 20 // �b���|�W�� PetStats ���c����t�Ŷ�
            mov edi, esp              // edi ���V���|�W�� PetStats
            // ��R���c��
            mov eax, [edx + ecx * 8 + 0x1C] // ����
            mov[edi + 0], eax
            mov eax, [edx + ecx * 8 + 8]    // ��
            mov[edi + 4], eax
            mov eax, [edx + ecx * 8 + 0x20] // ��
            mov[edi + 8], eax
            mov eax, [edx + ecx * 8 + 0x24] // ��
            mov[edi + 12], eax
            mov eax, [edx + ecx * 8 + 0x28] // ��
            mov[edi + 16], eax
            // �P�_�O�����٬O�|���ˬd
            mov eax, [ebp - 0x90] // ���s���J��l���ˬd��������
            cmp dword ptr[eax], 0x65785C3E
            je is_last_digit

            is_stats_check :
        push edi          // �Ѽ�2: PetStats ���c����
            push 2            // �Ѽ�1: checkType = 2 (�|��)
            jmp do_call

            is_last_digit :
        push edi          // �Ѽ�2: PetStats ���c����
            push 1            // �Ѽ�1: checkType = 1 (����)

            do_call :
            call ProcessPetChecks // �I�s C++ �禡
            add esp, 8             // �M�z2�ӰѼƪ����|

            // C++ �禡��^��AEAX ���O���G (7777, 6666, 9999)
            push eax               // �O�s���G
            call WriteCheckResult  // �I�s C++ �禡�N���G�g�J�r��
            pop eax                // ��_���G�� EAX
            add esp, 20 // ���� PetStats ���t�����|�Ŷ�[eax]
            mov eax,7777
            mov [ebp - 0x14], eax  // �N���G�s�J�ؼ��ܼ�
		success_path_asm :
            push 0x0048A119        // ����즨�\�B�z�y�{
            ret
        fail_jump :
            push 0x0048A3A2
            ret
    }
}


// --- �����ˬd Hook ---
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


// --- �״_���M��w�\�� ---
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

// --- Let���O Hook ---
void ProcessParameterHook(DWORD eax_value) {
    if (IsBadReadPtr((void*)eax_value, sizeof(LPCWSTR))) return;
    LPCWSTR* pString = (LPCWSTR*)eax_value;
    if (IsBadReadPtr(*pString, sizeof(wchar_t))) return;
    LPCWSTR finalStringAddress = *pString;
    // --- �j�p�g���ӷP�B�z ---
    // 1. �N�ǤJ���r���ഫ�����p�g�� std::wstring�A�Ω������
    std::wstring lookupKey(finalStringAddress);
    std::transform(lookupKey.begin(), lookupKey.end(), lookupKey.begin(),
        [](wchar_t c) { return std::tolower(c); });
    // ---
    static const std::unordered_map<std::wstring, std::function<void(LPCWSTR)>> actionMap = {
        {L"#�t�β@��ɶ��W00", [](LPCWSTR addr) {
            SYSTEMTIME st_now;
            GetSystemTime(&st_now); // ���o�ثe UTC �ɶ�
            FILETIME ft_now;
            SystemTimeToFileTime(&st_now, &ft_now); // �ഫ�� FILETIME �榡
            ULARGE_INTEGER uli_now;
            uli_now.LowPart = ft_now.dwLowDateTime;
            uli_now.HighPart = ft_now.dwHighDateTime;
            // uli_now.QuadPart �O�q 1601/1/1 �ܤ��� 100�`�� (nanosecond) ���j��
            // 1. �N���ഫ���@�� (millisecond) -> ���H 10000
            ULONGLONG totalMilliseconds = uli_now.QuadPart / 10000;
            // 2. �� 7,776,000,000 ���l��
            ULONGLONG finalValue = totalMilliseconds % 7776000000ULL;
            wchar_t buffer[64];
            // �ϥ� %llu �Ӯ榡�� ULONGLONG (unsigned long long)
            swprintf_s(buffer, 64, L"%llu", finalValue);
            OverwriteStringInMemory(addr, buffer);
        }},
        {L"#�t�ά�Ʈɶ��W", [](LPCWSTR addr) {
            SYSTEMTIME st_now;
            GetSystemTime(&st_now); // ���o�ثe UTC �ɶ�
            FILETIME ft_now;
            SystemTimeToFileTime(&st_now, &ft_now); // �ഫ�� FILETIME �榡
            ULARGE_INTEGER uli_now;
            uli_now.LowPart = ft_now.dwLowDateTime;
            uli_now.HighPart = ft_now.dwHighDateTime;
            // uli_now.QuadPart �O�q 1601/1/1 �ܤ��� 100�`�� (nanosecond) ���j��
            // 1. �N���ഫ���� -> ���H 10000000
            ULONGLONG totalMilliseconds = uli_now.QuadPart / 10000000;
            // 2. �� 7,776,000 ���l��
            ULONGLONG finalValue = totalMilliseconds % 7776000ULL;
            wchar_t buffer[64];
            // �ϥ� %llu �Ӯ榡�� ULONGLONG (unsigned long long)
            swprintf_s(buffer, 64, L"%llu", finalValue);
            OverwriteStringInMemory(addr, buffer);
        }},
        {L"#xpos", [](LPCWSTR addr) {
            // �i�ץ��j�אּ�ϥΥ����ܼ� g_assaInfo.base
            if (!g_assaInfo.base) return;

            DWORD xposValue = 0;
            LPCVOID dynamicAddress = (LPCVOID)((BYTE*)g_assaInfo.base + 0xF5078);

            xposValue = *(DWORD*)dynamicAddress;

            wchar_t buffer[64];
            swprintf_s(buffer, 64, L"%d", xposValue);
            OverwriteStringInMemory(addr, buffer);
        }},
        {L"#ypos", [](LPCWSTR addr) {
            // �i�ץ��j�אּ�ϥΥ����ܼ� g_assaInfo.base
            if (!g_assaInfo.base) return;

            DWORD yposValue = 0;
            LPCVOID dynamicAddress = (LPCVOID)((BYTE*)g_assaInfo.base + 0xF507C);

            yposValue = *(DWORD*)dynamicAddress;

            wchar_t buffer[64];
            swprintf_s(buffer, 64, L"%d", yposValue);
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
 else { OverwriteStringInMemory(addr, L"�������}��"); }
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
else { OverwriteStringInMemory(addr, L"�������}��"); }
}},
// �i�ץ��j�N %d �אּ %g �H���TŪ�� double ��������
        { L"#ev1", [](LPCWSTR addr) { wchar_t buf[32]; swprintf_s(buf, 32, L"%g", ev1); OverwriteStringInMemory(addr, buf); } },
        { L"#ev2", [](LPCWSTR addr) { wchar_t buf[32]; swprintf_s(buf, 32, L"%g", ev2); OverwriteStringInMemory(addr, buf); } },
        { L"#ev3", [](LPCWSTR addr) { wchar_t buf[32]; swprintf_s(buf, 32, L"%g", ev3); OverwriteStringInMemory(addr, buf); } },

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
void ProcessEcxString(LPCWSTR stringAddress) {
    if (stringAddress && !IsBadReadPtr(stringAddress, sizeof(wchar_t))) {
        if (wcscmp(stringAddress, L"�q�ߩҦ椧���Y�O����") == 0) {
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
            if (scriptTime > 31536000) {
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
        { L"�԰����F", 0, 5, 0xF61C6, 0xF61C7 }, { L"�԰����F�H��", 0,101, 0xF61C6, 0xF61F5 },
        { L"�԰����F�d��", 0,101, 0xF61C6, 0xF61F6 }, { L"�԰��ɦ�", 0, 1, 0xF61C8, 0xF61C8 },
        { L"���d�ɦ�", 0, 3, 0xF61C9, 0xF61CA }, { L"���d�ɦ�H��", 0,101, 0xF61C9, 0xF61F9 },
        { L"���d�ɦ��d��", 0,101, 0xF61C9, 0xF61FA }, { L"�_�����F", 0, 5, 0xF61FE, 0xF6200 },
        { L"�_�����~", 0, 1, 0xF61FF, 0xF61FF }, { L"���ɺ��F", 0,20, 0xF6229, 0xF622A },
        { L"���ɺ��F�H��", 0,101, 0xF6229, 0xF622B }, { L"���ɺ��F�d��", 0,101, 0xF6229, 0xF622C },
        { L"���ɸɦ�", 0, 1, 0xF622D, 0xF622D }, { L"���ɧޯ�", 0,26, 0xF6232, 0xF6233 },
        { L"�d���C��", 0, 1, 0xF6662, 0xF6662 }, { L"�԰�����", 0, 1, 0xF6661, 0xF6661 },
        { L"�԰��Ա�", 0, 1, 0xF6669, 0xF6669 }, { L"�զW��", 0, 1, 0xF53CC, 0xF53CC },
        { L"�¦W��", 0, 1, 0xF53CD, 0xF53CD }, { L"�l��զW��", 0, 1, 0xF53B8, 0xF53B8 },
        { L"�۰ʭ簣", 0, 1, 0xF53CE, 0xF53CE }, { L"����1", 0, 1, 0xF53B9, 0xF53B9 },
        { L"����2", 0, 1, 0xF53BA, 0xF53BA }, { L"����3", 0, 1, 0xF53BB, 0xF53BB },
        { L"����4", 0, 1, 0xF53BC, 0xF53BC }, { L"����5", 0, 1, 0xF53BD, 0xF53BD },
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

// --- Assign ���O Hook (�ץ���) ---
__declspec(naked) void MyAssignHook() {
    __asm {
        mov edx , [ebp - 0x01A0]
        cmp[edx] , 0x00530041
        jne original_path

        // �O�s��������
        pushad
        // �I�s C++ �禡
        mov eax, offset ProcessAssignStringCommand
        call eax
        // �i�ץ��j�b popad ���e�A�N EAX ������^�ȼȦs����|��
        // (pushad �|���J8�ӼȦs���A�@32 bytes�A�ҥH esp+32 �O pushad ���e����m)
        mov[esp + 32 - 4], eax // �N eax �s��� esp ����m�A�w���i�a
        // ��_�Ҧ��q�μȦs�� (���� EAX �Q�­��л\)
        popad
        // �i�ץ��j�q���|���N�ڭ̼Ȧs����^�ȡA���s���^�� EAX
        mov eax, [esp - 4]
        // �{�b�i�H�w���a�����^�ȤF
        cmp al, 1
        // �p�G���\�A�������w�����\���|
        je assign_success

        original_path :
            // �p�G���ѡA�����l�Q�л\�����O
            mov edx, [ebp - 0x01A0]
            // �M�����^��l�{���X���U�@��
            jmp gAssignHookReturnAddr

        assign_success :
            // �ϥ� push + ret ���覡����
            push 0x00483E5F
            ret
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
        L"/watch", L"/encount on", L"/encount off",
        L"/pile", L"/pile on", L"/pile off", L"�e���_�I�̤��q"
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
    InstallJmpHook(L"Assa8.0B5.exe", Addr::CompareHookOffset, MyCompareHook, 6, (LPVOID*)&gCompareHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::AddEspHookOffset, MyAddEspHook, 5, (LPVOID*)&gAddEspHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::EcxStringHookOffset, MyEcxStringHook, 5, (LPVOID*)&gEcxStringHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::ParameterHookOffset, MyParameterHook, 5, (LPVOID*)&gParameterHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::PrintHookOffset, MyPrintHook, 5, (LPVOID*)&gPrintHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::Button2HookOffset, MyButton2Hook, 6, (LPVOID*)&gButton2HookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::IfitemHookOffset, MyIfitemHook, 6, (LPVOID*)&gIfitemHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::MountHookOffset, MyMountHook, 6, (LPVOID*)&gMountHookReturnAddr);
    InstallJmpHook(L"Assa8.0B5.exe", Addr::TeamCheckHookOffset, MyTeamCheckHook, 5, (LPVOID*)&gTeamCheckHookReturnAddr);
    // �i�s�W�j�w���d���ˬdHook�A�Q�л\����l���O���׬� 5 bytes
    InstallJmpHook(L"Assa8.0B5.exe", Addr::PetCheckHookOffset, MyPetCheckHook, 5, (LPVOID*)&gPetCheckHookReturnAddr);
    // �i�s�W�j�w�� Assign ���O Hook�A�Q�л\����l���O���׬� 6 bytes
    InstallJmpHook(L"Assa8.0B5.exe", Addr::AssignHookOffset, MyAssignHook, 6, (LPVOID*)&gAssignHookReturnAddr);

    // --- �S�� Hook ���w�� ---
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