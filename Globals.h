// �ɮ�: Globals.h
// ����: �Τ@�޲z�M�שҦ������ܼƩM���A�C
#pragma once
#include <windows.h>
#include <time.h>

// �ϥε��c��Ӳ�´�P�ؼЦ�{��������T�A�����A�޲z��M��
struct TargetInfo {
    DWORD   PID;
    HWND    HWND;
    HMODULE base;
    HANDLE  hProcess;
};

// ========== �����ܼƫŧi ==========
// extern ����r��ܳo���ܼƪ�����b�t�@�� .cpp �ɮפ� (Globals.cpp)
// ����]�t�F�o�Ӽ��Y�ɪ��ɮ׳��i�H�s�����̡C

extern TargetInfo g_assaInfo; // �D���� (Assa) ����T
extern TargetInfo g_saInfo;   // �ؼйC�� (SA) ����T

// �Ω� Assign ���O���ۭq�ܼ� (�䴩�p�ƩM�t��)
extern double ev1, ev2, ev3;

// �۰ʰ��|�p�ƾ�
extern int pileCounter;

// �}���p�ɾ�
extern time_t scriptTime;