// 檔案: Globals.cpp
// 說明: 提供 Globals.h 中宣告的全域變數的實際定義和初始化。
#include "pch.h"
#include "Globals.h"

// ========== 全域變數定義與初始化 ==========

TargetInfo g_assaInfo = { 0, NULL, NULL, NULL };
TargetInfo g_saInfo = { 0, NULL, NULL, NULL };

// 【修改】定義一個包含 10 個 double 的陣列，並全部初始化為 0.0
double ev[10] = { 0.0 };

int pileCounter = 0;
time_t scriptTime = 0;