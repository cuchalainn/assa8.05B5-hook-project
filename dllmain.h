// 檔案: dllmain.h

#pragma once
#include <windows.h>
#include <string>

// 宣告可以被其他 .cpp 檔案呼叫的函式
std::wstring PerformRiddleLogic();

// 使用 extern 宣告全域變數，讓所有 cpp 檔案共用
extern DWORD Assa_PID;
extern HWND Assa_HWND;
extern DWORD Sa_PID;
extern HWND Sa_HWND;
extern int ev1;
extern int ev2;
extern int ev3;
// 【新增】宣告目標程式的基底位址
extern HMODULE Sa_base;