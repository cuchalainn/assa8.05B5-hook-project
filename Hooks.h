// 檔案: Hooks.h
#pragma once

// 定義一個結構體來傳遞寵物的數值，方便 C++ 函式處理
struct PetStats {
    int LV;
    int HP;
    int ATK;
    int DEF;
    int DEX;
};

// 向外公開的總安裝函式
void InstallAllHooks();