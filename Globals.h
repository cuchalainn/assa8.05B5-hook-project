// �ɮ�: Globals.h
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
extern TargetInfo g_assaInfo; // �D���� (Assa) ����T
extern TargetInfo g_saInfo;   // �ؼйC�� (SA) ����T

// �i�ק�j�N ev �ܼƧאּ�@�ӥ]�t 10 �� double ���}�C
extern double ev[10];

// �۰ʰ��|�p�ƾ�
extern int pileCounter;

// �}���p�ɾ�
extern time_t scriptTime;