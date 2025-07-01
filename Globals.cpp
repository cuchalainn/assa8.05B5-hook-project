// 檔案: Globals.cpp
#include "pch.h"
#include "Globals.h"

// ========== 全域變數定義 ==========

// 初始化結構體，確保初始狀態乾淨
TargetInfo g_assaInfo = { 0, NULL, NULL, NULL };
TargetInfo g_saInfo = { 0, NULL, NULL, NULL };

// 【修正】將 ev1, ev2, ev3 初始化為 0.0
double ev1 = 0.0;
double ev2 = 0.0;
double ev3 = 0.0;
int pileCounter = 0;
time_t scriptTime = 0;