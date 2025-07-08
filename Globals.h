// 檔案: Globals.h
#pragma once
#include <windows.h>
#include <time.h>

// 使用結構體來組織與目標行程相關的資訊，讓狀態管理更清晰
struct TargetInfo {
    DWORD   PID;
    HWND    HWND;
    HMODULE base;
    HANDLE  hProcess;
};

// ========== 全域變數宣告 ==========
extern TargetInfo g_assaInfo; // 主控端 (Assa) 的資訊
extern TargetInfo g_saInfo;   // 目標遊戲 (SA) 的資訊

// 【修改】將 ev 變數改為一個包含 10 個 double 的陣列
extern double ev[10];

// 自動堆疊計數器
extern int pileCounter;

// 腳本計時器
extern time_t scriptTime;