// �ɮ�: Globals.h
#pragma once
#include <windows.h>
#include <time.h>

// �ϥε��c��Ӳ�´�P�ؼЦ�{��������T�A�����A�޲z��M��
struct TargetInfo {
    DWORD PID;
    HWND  HWND;
    HMODULE base;
    HANDLE  hProcess;
};

// ========== �����ܼƫŧi ==========
extern TargetInfo g_assaInfo; // �D���� (Assa) ����T
extern TargetInfo g_saInfo;   // �ؼйC�� (SA) ����T

extern int ev1, ev2, ev3;
extern int pileCounter;
extern time_t scriptTime;