// 檔案: Globals.h
// 說明: 統一管理專案所有全域變數和狀態。
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
// extern 關鍵字表示這些變數的實體在另一個 .cpp 檔案中 (Globals.cpp)
// 任何包含了這個標頭檔的檔案都可以存取它們。

extern TargetInfo g_assaInfo; // 主控端 (Assa) 的資訊
extern TargetInfo g_saInfo;   // 目標遊戲 (SA) 的資訊

// 用於 Assign 指令的自訂變數 (支援小數和負數)
extern double ev1, ev2, ev3;

// 自動堆疊計數器
extern int pileCounter;

// 腳本計時器
extern time_t scriptTime;