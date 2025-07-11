// 檔案: UIManager.h
#pragma once
#include <windows.h>
#include <string>
#include <vector>

// 向專案的其他部分宣告 UI 初始化函式
void PrepareUI();

// 宣告為全域變數，以便其他 .cpp 檔案可以存取和釋放
extern HFONT g_hCustomFont;
extern HFONT g_hSnapButtonFont;
extern HFONT g_hPoemLabelFont;