// �ɮ�: dllmain.h

#pragma once
#include <windows.h>
#include <string>

// �ŧi�i�H�Q��L .cpp �ɮשI�s���禡
std::wstring PerformRiddleLogic();

// �ϥ� extern �ŧi�����ܼơA���Ҧ� cpp �ɮצ@��
extern DWORD Assa_PID;
extern HWND Assa_HWND;
extern DWORD Sa_PID;
extern HWND Sa_HWND;
extern int ev1;
extern int ev2;
extern int ev3;
// �i�s�W�j�ŧi�ؼе{�����򩳦�}
extern HMODULE Sa_base;