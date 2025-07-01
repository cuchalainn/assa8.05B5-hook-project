// 檔案: Hooks.h

#pragma once

// 【新增】定義一個結構體來傳遞寵物的數值
struct PetStats {
    int LV;
    int HP;
    int ATK;
    int DEF;
    int DEX;
};

void InstallAllHooks();