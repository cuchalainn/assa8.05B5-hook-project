// 檔案: Globals.cpp
#include "pch.h"
#include "Globals.h"

// ========== 全域變數定義 ==========

// 初始化結構體，確保初始狀態乾淨
TargetInfo g_assaInfo = { 0, NULL, NULL, NULL };
TargetInfo g_saInfo = { 0, NULL, NULL, NULL };

int ev1 = 0;
int ev2 = 0;
int ev3 = 0;
int pileCounter = 0;
time_t scriptTime = 0;