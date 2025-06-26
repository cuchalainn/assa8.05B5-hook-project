// 檔案: Globals.h
#pragma once
#include <windows.h>
#include <time.h>

// 使用結構體來組織與目標行程相關的資訊，讓狀態管理更清晰
struct TargetInfo {
    DWORD PID;
    HWND  HWND;
    HMODULE base;
    HANDLE  hProcess;
};

// ========== 全域變數宣告 ==========
extern TargetInfo g_assaInfo; // 主控端 (Assa) 的資訊
extern TargetInfo g_saInfo;   // 目標遊戲 (SA) 的資訊

extern int ev1, ev2, ev3;
extern int pileCounter;
extern time_t scriptTime;