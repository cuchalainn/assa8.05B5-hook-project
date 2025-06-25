// �ɮ�: RiddleDatabase.cpp
#include "pch.h"
#include "RiddleDatabase.h"
#include "Globals.h"
#include <unordered_map>
#include <windows.h>
#include <cstdio>
#include <algorithm>
#include <stdexcept>

// �i�ץ��j�N�R�W�Ŷ��w�q�������A�Ӥ��O�禡����
namespace Addr_Riddle {
    constexpr DWORD RiddleStringOffset = 0x16982D;
    constexpr DWORD RiddleGuardOffset = 0x169A4D;
    constexpr DWORD RiddleOption1Offset = 0x169A52;
    constexpr DWORD RiddleOption2Offset = 0x169B62;
    constexpr DWORD RiddleOption3Offset = 0x169C72;
    constexpr DWORD RiddleGuardValue = 0x44A1B0A2;
}

// �禡�������@
static std::wstring GetRiddleAnswer(const std::wstring& riddleText) {
    static const std::unordered_map<std::wstring, std::wstring> riddleAnswerMap = {
        //���
        {L" �j�����P�M�����h�����l�వ����خƲz�H", L"$���P��"},
        {L" �ΰ������P���ΰ������i�H�������خƲz�H", L"$���P���K"},
        {L"�u�R����l��������l�v���٪��O���@�خƲz", L"$��l��l"},
        {L"�u�k���I�ީޡv�y�H�W�H���W�r�O�H", L"$�N�ĩԩ_"},
        {L"�d��S�B�ͤ�B����B�ͤ�S���U�O���ݩʦX", L"$26"},
        {L"�d�|���������l���F77�n93���֦b���O�H", L"$����M�a"},
        {L"�d�|����������H�ȹB�����ФH���W�r�O�H", L"$����"},
        {L"�Φh�h�����������B���ɡB�j����K�B��i��", L"$������"},
        {L"�γ̴Ϊ������s�@���Ǩ��W�٬��H", L"$�S��Ǩ�"},
        {L"�ηs�A����K�B���R���Q�B�~���u�}�����వ", L"$�s�A���N��K"},
        {L"�����o���B�ݰ_�ӤS�n�z�A�ٷ|�L�ӥΤj���Y", L"$�����_�Q"},
        {L"�V�ڭ̱��P���������O���ӧ������H�]�g���W", L"$�_���J"},
        {L"�^���d�|�����������ү���m���y�СC�]�ҡ�", L"$�F17:�n20"},
        {L"�^���Ʋz�u���ز�ͳ����v��������C", L"$�s���ͳ���"},
        {L"�b�[�[�����l������ν�594S������W�٥��T", L"$����Lv1�Z2"},
        {L"�b�[�[���K�Q�ө����A�ꪺ���Ԧ��X�i�H�]��", L"$32"},
        {L"�b�F�i�q�]�a�W�^�A���F�������s���~�٦���", L"$���ԩ_�h��"},
        {L"�b�e���W�i�H��ܥX���a�������O���R�R���B", L"$�d�|����"},
        {L"�b���R�R���檺���ź��R�R�����n�h�֥��Y�H", L"$150"},
        {L"�b�ĥ쨺�������W��ê�q�檺�O��?", L"$�a�߲����@��"},
        {L"�h�h���K�Q�Z�����ҽ�ӫ~���`�ȬO�H�]�u��", L"$34170"},
        {L"��d�|�����W�Ǫ��p�Ĥl���X�ӤH�H�]�b�μ�", L"$5"},
        {L"����H�ȹB�[�[���������ФH���W�r�O�H", L"$�d�|��"},
        {L"�������}�]�e���x�s�I�n�Υ���M�����d����", L"$����"},
        {L"�_�������㩱�̰�������F��M�N�S�����Z��", L"$10850"},
        {L"�_���J���d�����̦����d���O�I�ީީM������", L"$�p�S����"},
        {L"�_���J���C�~�R�F�}�M����Z������o�b�дo", L"$�j�O��"},
        {L"���e�v�����a���@�ذѼƭȪ����ܩO�H", L"$�y�O"},
        {L"���s���s�u�H�H�H�v�̭����s���W�r�O�H�@�u", L"$�d��S"},
        {L"���s�դh���U�⪺�W�r�O�H", L"$�N�|"},
        {L"�溸�y��溸��b�e���M�᭱�A����_�Թy�b", L"$�W"},
        {L"��Ϫ��a�b�Q�����ɱN�����Ϯg�^�h�@�����O", L"$�誺���F"},
        {L"�s�骺�}�]�a�U3�Ӫ��H���W�r�O�H", L"$�K�����p�u"},
        {L"���R�R�����������k�l���쪺�n�l�O�H", L"$�[���n"},
        {L"�����嬰�u��Ʋz�K�H�@�˦��Ұ�������F�N", L"$�K�H���F�N"},
        {L"�Х��T�^��������ܾ԰��e��������έ��]�t", L"$�թM�����F"},
        {L"�Ц^���d�Z���d���O���y�СH�]�ҡ��F0:�n0 ", L"$�F78:�n97"},
        {L"�Ц^���ηs�A����K�B���R���Q�B�~���u�}��", L"$���ܷs�A����K�ӯN"},
        {L"�Ц^���Ʋz�u�W��������׿L�N�v��������O", L"$�L�ܤ[�����׿L�N"},
        {L"�мg�X�i�H�@���^�_����έ��@�[�O�����F�G", L"$���f�����F"},
        {L"�ĩi�N�������a���Ʋz�H�W�r�O�H", L"$�j��"},
        {L"�d�����s�u�H�H�H�v�̪��d�����W�r�O�H�u��", L"$������"},
        //���
        {L" �uī�G��Ĥ��ơv�Ʋz�������嬰�H�U���?", L" ���Oī�G������"},
        {L" �M������O�쪺�����X�O�C", L"�쪺����4"},
        {L"�u�������������áv���򡳸̭����ӬO�H", L"�s����@"},
        {L"�u�a�߲����@�áv�O���@�⮳�۪Z�����O�H", L"�k��"},
        {L"�@�J��]�L�Ӫ�����H�����B�H���|�ܦ����", L"���|���"},
        {L"�U�C���@�خƲz�O���s�b���H", L"�s�A�n�Y������"},
        {L"�U�C���@�ӬO�b�ͤ�J�|�����Ƥ��O�S�����H", L"����"},
        {L"�U�C���ӵL�k�^�_25��O�H", L"���״�"},
        {L"�U�C���خƲz�S���^�_��O22���ĪG?", L"�ۯN�a��"},
        {L"�U��3�Ӻ��F�����ݩ�D�^�_�t�G�N���O����", L"�u�@�����F"},
        {L"�U�����@�ӨS���u����v1UP �v���ĪG�H", L"��l�}"},
        {L"�U�����خƲz�����C����Ħ�H", L"�C����"},
        {L"�p���Y��p�ҴΡA���@�Ӥ���Q�O�H", L"�p���Y"},
        {L"����l�M���ͽ֦b���R�R���H", L"����"},
        {L"�H�U�����ݩʪ��d���O�H", L"�����"},
        {L"�H�U���a�ݩʪ��d���O���@�ӡH", L"�J�p��"},
        {L"�H�U�S���u�s�K4�^�X�e��v�ĪG���Ʋz�O�H", L"�������G�s"},
        {L"�H�U���ǨS���u��2�^�_�v�ĪG���O�H", L"�ͥ]�ߵ�"},
        {L"�H�U���ǨS���u��5�^�_�v�ĪG���O�H", L"���F��"},
        {L"�H�U�ݩʨS���y�a10�z���O���@�ӡH", L"���t"},
        {L"�[�u�Ϊ��D�㤤�A�̫K�y���O�F��O����ۡH", L"��1"},
        {L"�[�[���K�Q�ө��̡A�Ʋz�Ѯv�èS���i�D�ڭ�", L"�Ʋz������"},
        {L"�[�[���D��Φ��椰��H", L"��������Lv3��l"},
        {L"�[�[�����橱����H�U���تF��H", L"�ĩi�N�������U�l"},
        {L"�[�[���x�s�I������רӴN�|���ڭ̬����O�H", L"�Q�_�_����"},
        {L"�[�|�d���j���q���ʡ]JOT�^��A���u����9��", L"�Q�|�O"},
        {L"�[�|�d�q�̫n�䪺�������W�٬O�H", L"�_�س�"},
        {L"�d�Z���x�s�I�O������ץh�~�|���ڭ̰O���O", L"�S����"},
        {L"�d�|���������Τ����d���������X���H", L"10"},
        {L"�d�|�������������{�b�X���п�X���T������", L"17"},
        {L"�u�����ۤv�^�_���G�N�O?", L"�v¡�����F"},
        {L"�i�H��_�ڭ̾y�O���O�֡H", L"���e�v"},
        {L"�۾��ɥN���@�ɪ��W�٬O?", L"����"},
        {L"�۾��ɥN�s�����]���w���O�H", L"250�x��"},
        {L"���t�ݩʬO�H", L"�a���@����"},
        {L"�Y�^�_��O���Ʋz���ɭԪ��a���⪺�����O�H", L"�I�Y"},
        {L"�b������Z���W�w��50��59�����ŬO�H", L"���|�d��"},
        {L"�b�[�|�d�q�̥_�䪺�����W�٬O?", L"��i��i"},
        {L"�b�d�|�������̪�������10�~�e�a�_���ɭԬO", L"7��"},
        {L"�b�d�|���������Q�ή��s������򤰻�i�H��", L"�ָ�"},
        {L"�b�d�|���������񥴤u���d����LV20�H�W30��", L"�h�Q�ե���"},
        {L"�b�d�|�������������]���Y�B��^���b��y��", L"26STONE"},
        {L"�b�_���J�����D�㩱���A�U�C���@�تF��̫K", L"����j����"},
        {L"�b�F�������D�㩱���A�U�C���@�تF��̩��Q", L"�°�����"},
        {L"�b���R�R������|���ڭ̪��^�_���@�h�A��W", L"�`�g��"},
        {L"�b�ĩi�N�����X�ͪ��H�Ү�������d���O����", L"�Q�O�Q�O"},
        {L"�b�ĩi�N���̤j�۹����үS��O�֡H", L"�ĩi�N��"},
        {L"�b��|�A�n���d���^�_�n��h�֥��Y�H", L"���ο�"},
        {L"���H���������������o�줰��O�H", L"������"},
        {L"�{�l�O��⪺�T���t�d���O�H", L"�[�J��"},
        {L"���@�Ӥ��O�N�S�������H", L"�ڽ�"},
        {L"��d�Z������ͪ��w�̪�E�q�`���O�w����f", L"����"},
        {L"���a�H�����Y�v�O�¦⪺���h�֤H?", L"4"},
        {L"���a���ۤ�����n�s������H", L"�M��"},
        {L"���a�b�۾��̵L�k�����ʧ@�O�H", L"��ı"},
        {L"���a����άO�d���𵴮ɦb�Y�W��¶���P�P��", L"3��"},
        {L"���a���W�̦h�i�H�a�h�֥��Y���H", L"1000000"},
        {L"���ê��d�����W�r�O�H", L"�p�d"},
        {L"����H�����X�o�ɪ���s�n�O�H", L"�z�z�z����"},
        {L"����H�����b���������d�h�[���ɶ��O�H", L"3��"},
        {L"����H�������j�H�W�r�s����H", L"������"},
        {L"�������W�ɩЫθ̦@���h�֧��ԩO�H", L"10�T"},
        {L"��ˤ~�৤����H�����O?", L"�[�J�ζ�"},
        {L"�d�u�T�S�̤��̤p���̧̦W�r�O���@�ӡH", L"�d�u�|�w"},
        {L"�_���J���۫ΥD�H�O�X���}�l�@���Y�۪��O�H", L"13"},
        {L"���ɪ��������ƭȬO�H�U�����@�ӡH", L"��+2��+2��-2�y+1"},
        {L"�n���u�M�I���ͳ����v�����ƬO�U�����@�ӡH", L"����"},
        {L"�n�����������N���ߥH�U���@�ӬO���ݭn���H", L"�d�|���������Q"},
        {L"���Ȥs���Q�O���ͬO��H�浹�֡H", L"�Q�O��"},
        {L"���g�ä֦~�̶��ҫ������@�äH���O�¦V���k", L"�k"},
        {L"�Ʋz�u�׺�v���D�㻡���O�H�U�����@�ӡH", L" �]�ת���l"},
        {L"�Ʋz�u���P���Q����N�v���ĪG�O�H�U���@��", L"5�^�X���k�s�K"},
        {L"�Ʋz�u�N�J�����l��N�v���ĪG�O�H�U���@��", L"�ۤ�3�^�X�e�� "},
        {L"�Ʋz�u���D�C��v�O�ѤU�C���ا��Ʃһs�y�X", L"�C��+�o"},
        {L"�Ʋz�u���ɨ������v���ĪG�O�H�U���ӡH", L"��70�^�_(1��) "},
        {L"��O�^�_40����O�^�_�Ĳ~�l�O�����C�⪺�O", L"����"},
        {L"�Q�O���Z���C��O�����򤰻��H", L"�¦��զ�"},
        {L"�Q�O���Z�̪�Ҿ֦����ޯ�O�U�C���@�ӡH", L"����"},
        {L"�Q�輯�������������A�u�Ѭr���H�H�H�Ӫ��v", L"���a��"},
        {L"���Q�O���ͻ��ܪ��D��O�H", L"���i��ĳ������"},
        {L"�����ζ������^�_���G�N�O?", L"���f�����F"},
        {L"���۩ҹ}�i���d�����W�r�O�H", L"�[�S�O��"},
        {L"���T���v���ɭԨ��@�تF��O����|�Q��i�h", L"���G"},
        {L"�򥻤W�ӻ��A���M�����O�W�ɡA���O���m�α�", L"���Y"},
        {L"�զ��ζ��̤j�H�ƬO�h�֤H�H", L"5�H"},
        {L"��ѫǤ����j�U�s����W��?", L"����H�j�U"},
        {L"�o�ӥ@�W�ο��������a��O���̡H", L"�N�|�q"},
        {L"�o�ӥ@�ɿ������O?", L"���Y"},
        {L"�o�ӹC�����^��W�٬O�H", L"STONEAGE"},
        {L"�q�L�����q�D�ݭn����?", L"�q����"},
        {L"�ͤ�S������O�����C��H", L"����"},
        {L"���ť�1�ܨ�2�n�h�ָg���?", L"��"},
        {L"��i��i���̪��u�������v�O��˪��H�H", L"�ѧB"},
        {L"��i��i�����2�~�e���g�b������|�O�H", L"�ĩi�N��"},
        {L"�g�����H�������O�H", L"����H�ȹB"},
        {L"�D��y�P���Ȫ���z�����T�����O���@�ӡH", L"�b�ĩi�N����E�W�K����"},
        {L"��۩_�سا����a�j���ɡA��������ĵ�ƭ��O", L"�̧�"},
        {L"���R�R���������M�C�~�b�@�_���Q�O�Q�O���s", L"�Q�O����"},
        {L"�����嬰�u�̭����C��K�v���Ʋz�O�U�����@", L"�C����"},
        {L"�������O�b���̥ͪ����Ӫ��O�H", L"����"},
        {L"�п�X�U�C�W�ٿ��~���F��C", L"��ɭ���"},
        {L"�԰����Τ}�b�����ˮ`�]�άO�����^����A��", L"�^�X�������e�ٷ|�s�b"},
        {L"�ĩi�N�����Ǯժ��Ѯv��̮��ۤ���H", L"���Y"},
        {L"�d�����ޥ������O�@���a���ޯ�s������H", L"����"},
        {L"�@�ê��d�����W�r�O?", L"�I�ަN"},
    };
    auto it = riddleAnswerMap.find(riddleText);
    if (it != riddleAnswerMap.end()) {
        return it->second;
    }
    return L"";
}
// �i�ק�j�s�����}�禡 QA()�A�ĥΤF�z�̷s���޿�
std::wstring QA() {
    if (!Sa_PID || !Sa_base) {
        return L"CANCEL"; // �ؼХ��j�w
    }
    HANDLE hProcess = OpenProcess(PROCESS_VM_READ, FALSE, Sa_PID);
    if (!hProcess) {
        return L"CANCEL"; // �}�Ҧ�{����
    }
    std::wstring finalOutput = L"";
    wchar_t unicodeBuffer[256] = { 0 };
    char big5Buffer[256] = { 0 };
    LPCVOID dynamicRiddleAddr = (LPCVOID)((BYTE*)Sa_base + Addr_Riddle::RiddleStringOffset);
    // �uŪ���@���A���A����
    if (ReadProcessMemory(hProcess, dynamicRiddleAddr, big5Buffer, sizeof(big5Buffer) - 1, NULL)) {
        MultiByteToWideChar(950, 0, big5Buffer, -1, unicodeBuffer, 256);
        std::wstring correctAnswer = GetRiddleAnswer(unicodeBuffer);
        if (!correctAnswer.empty()) {
            // �p�G���F�зǵ��סA�~�����ﶵ�P�_
            finalOutput = correctAnswer;
            DWORD guardValue = 0;
            LPCVOID dynamicGuardAddr = (LPCVOID)((BYTE*)Sa_base + Addr_Riddle::RiddleGuardOffset);
            ReadProcessMemory(hProcess, dynamicGuardAddr, &guardValue, sizeof(guardValue), NULL);
            if (guardValue == Addr_Riddle::RiddleGuardValue) {
                auto checkOption = [&](DWORD optionOffset) -> bool {
                    char optBig5[256] = { 0 };
                    wchar_t optUnicode[256] = { 0 };
                    LPCVOID dynamicOptionAddr = (LPCVOID)((BYTE*)Sa_base + optionOffset);
                    if (ReadProcessMemory(hProcess, dynamicOptionAddr, optBig5, sizeof(optBig5) - 1, NULL)) {
                        MultiByteToWideChar(950, 0, optBig5, -1, optUnicode, 256);
                        return (correctAnswer == optUnicode);
                    }
                    return false;
                    };
                if (checkOption(Addr_Riddle::RiddleOption1Offset)) finalOutput = L"1";
                else if (checkOption(Addr_Riddle::RiddleOption2Offset)) finalOutput = L"2";
                else if (checkOption(Addr_Riddle::RiddleOption3Offset)) finalOutput = L"3";
            }
        }
    }
    CloseHandle(hProcess);
    // �p�G finalOutput ���M�O�Ū� (Ū�����ѩ��D�w�L�ǰt)�A�h��^ CANCEL
    if (finalOutput.empty()) {
        return L"CANCEL";
    }
    return finalOutput;
}