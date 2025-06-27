# 專案名稱：Assa 8.05b5 Hook 專案

## 1. 專案概述 (Project Overview)

本專案是一個 DLL Hook 程式，旨在透過注入 `Assa8.0B5.exe` 行程，擴充其功能、增強使用者介面，並修正部分原有問題。主要使用 C++ 和手動 JMP Hook 技術實現。

## 2. 主要功能 (Features)

- **UI 增強**:
    - 新增「開始遇敵」、「離線掛機」等 6 個自訂功能按鈕。
    - 重新排列並隱藏部分舊版 UI 元件 (如快速行走、穿牆)。
    - 將介面中的簡體中文詞彙（如「激活石器」）替換為繁體。
- **指令擴充**:
    - `Let` 指令：新增 `#遊戲時間`、`#NPCID` 等自訂變數。
    - `Print` 指令：新增 `#計時開始/停止` 功能。
    - `Set` 指令：允許透過腳本修改外掛內部設定。
- **自動化功能**:
    - **自動猜謎**: 攔截謎題視窗，並自動從資料庫尋找答案填入。
    - **自動堆疊**: 在特定條件下自動執行 `/pile` 指令。

## 3. 如何建置 (How to Build)

- **環境**: Visual Studio 2022
- **平台**: x86 (Win32)
- **相依性**: 無外部相依性，專案已包含 MinHook 函式庫。
- **步驟**:
    1. 使用 Visual Studio 開啟 `.sln` 檔案。
    2. 選擇 "Release" 與 "x86" 設定。
    3. 建置專案，即可在 `Release` 資料夾中找到 `hook.dll`。

## 4. 運作原理 (How it Works)

本專案的核心是透過 `DllMain` 進入點，在 `DLL_PROCESS_ATTACH` 事件中安裝一系列的記憶體 Hook。

- **UI 修改 (`UIManager.cpp`)**:
    - 透過 `EnumChildWindows` 遍歷目標視窗的所有子元件。
    - 使用 `SetWindowLongPtrW` 進行「子類化 (Subclassing)」，攔截特定按鈕（如啟動石器）和容器的視窗訊息程序 (WndProc)。
    - 使用 `CreateWindowExW` 建立新的按鈕，並將其父視窗設定為「腳本制作」的容器。

- **功能攔截 (`Hooks.cpp`)**:
    - 透過分析目標程式，找出關鍵功能的記憶體位址（定義於 `Hooks.cpp` 的 `Addr` 命名空間）。
    - 使用手動寫入 `JMP` 指令的方式，將程式執行流程重導向至我們自訂的函式。
    - 在自訂函式中完成擴充邏輯後，再跳轉回原始程式碼的後續位址繼續執行。

## 5. 關鍵檔案說明 (Key Files)

- `dllmain.cpp`: DLL 進入點，負責初始化與資源釋放。
- `UIManager.cpp`: 所有 UI 相關操作，包括建立按鈕、處理點擊事件。
- `Hooks.cpp`: 所有記憶體 Hook 的安裝與核心邏輯。
- `Globals.cpp`: 定義專案所需的全域變數。
- `helpers.cpp`: 提供通用的輔助函式 (如讀寫記憶體)。
- `RiddleDatabase.cpp`: 儲存猜謎問答的資料庫。