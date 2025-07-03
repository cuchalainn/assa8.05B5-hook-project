// 檔案: Globals.cpp
// 說明: 提供 Globals.h 中宣告的全域變數的實際定義和初始化。
#include "pch.h"
#include "Globals.h"

// ========== 全域變數定義與初始化 ==========

// 初始化結構體，確保程式啟動時有乾淨的初始狀態
TargetInfo g_assaInfo = { 0, NULL, NULL, NULL };
TargetInfo g_saInfo = { 0, NULL, NULL, NULL };

// 將 ev 變數初始化為 0.0
double ev1 = 0.0;
double ev2 = 0.0;
double ev3 = 0.0;

// 其他計數器和計時器
int pileCounter = 0;
time_t scriptTime = 0;