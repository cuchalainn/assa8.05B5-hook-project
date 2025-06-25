// 檔案: Globals.cpp
#include "Globals.h"
// ========== 全域變數定義 ==========
// ========== 全域變數定義 ==========
DWORD Assa_PID = 0;
HWND Assa_HWND = NULL;
DWORD Sa_PID = 0;
HWND Sa_HWND = NULL;
HMODULE Sa_base = NULL;
int ev1 = 0;
int ev2 = 0;
int ev3 = 0;
int pileCounter = 0; // 用於計數自動堆疊次數
time_t scriptTime = 0; // 用於記錄腳本啟動時間