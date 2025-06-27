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

好的，專案已成功收尾，功能也已完善，現在為您整理一份關於UI修改的總結與註釋，方便您加入到 `README.md` 中。

---

## UI 修改與客製化功能詳解

本專案對 Assa 原始介面進行了深度客製化，不僅重塑了部分版面配置，還新增了多個實用功能按鈕。其核心是透過在 DLL 注入時，精準地尋找並操作視窗控制代碼 (HWND) 來達成。

### 1. 注入時：介面初始化與重塑

在 DLL 注入後，程式會立刻執行一系列的介面「預處理」操作，目的是在使用者與外掛互動前，就將介面調整成我們想要的樣子。這部分邏輯主要在 `UIManager.cpp` 的 `PrepareUI()` 和 `FindAndHookControlsProc()` 函式中實現。

* **改變按鈕文字 (繁體化與更名)**
    * **方法**：透過 `EnumChildWindows` 遍歷主視窗下的所有子元件。在回呼函式 `FindAndHookControlsProc` 中，使用 `GetWindowTextW` 獲取每個元件的標題。
    * **說明**：將預先定義好的簡體中文或舊有名稱（如「激活石器」、「自動戰斗」）與獲取到的標題進行比對，一旦匹配成功，就使用 `SetWindowTextW` 將其更新為我們想要的繁體中文或新名稱（如「啟動石器」、「自動戰鬥」）。

* **隱藏無用 UI**
    * **方法**：同樣在 `FindAndHookControlsProc` 中，透過比對視窗標題（如「地圖顯示」、「繁化主頁」）來找到目標按鈕。
    * **說明**：找到目標後，使用 `ShowWindow(hwnd, SW_HIDE)` 將其完全隱藏。同時，程式會將這些被隱藏按鈕的原始位置 (`RECT`) 和父視窗控制代碼 (`HWND`) 儲存在全域變數中，以供後續創建新按鈕時使用。

* **移動舊有 UI**
    * **方法**：在 `PrepareUI` 中，程式會先找到「自動猜謎」按鈕並將其隱藏，然後取得其父容器 (`hNewParent`)。
    * **說明**：接著，使用 `SetParent()` 將「快速行走」和「穿牆行走」這兩個按鈕的父視窗更改為 `hNewParent`，再使用 `MoveWindow()` 將它們精確地移動到容器內的新座標，實現了版面的重新佈局。

### 2. 啟動時：動態新增功能與事件監控

當使用者點擊「啟動石器」按鈕後，會觸發一個更深層的 UI 創建流程。這部分邏輯主要由 `LaunchButtonWndProc` 觸發，並在 `CreateAllButtons()` 中執行。

* **新增各種按鈕與標籤**
    * **方法**：使用標準的 Win32 API `CreateWindowExW` 來創建新的 UI 元件。
    * **說明**：
        * **主要功能按鈕**：在「腳本制作」面板 (`g_hTargetParent`) 中，創建「開始遇敵」、「呼喚野獸」等六個 `BUTTON` 控制項。
        * **吸附功能按鈕**：在先前記錄的「地圖顯示」和「繁化主頁」的父容器 (`g_hMapDisplayParent`, `g_hHomePageButtonParent`) 中，於相同位置創建「視窗吸附」和「子窗吸附」按鈕。
        * **詩句文字標籤**：在「快速行走」所在的新容器 (`g_hInfoContainer`) 中，創建一個 `STATIC` 控制項，並透過 `\r\n` 實現換行。
    * **外觀設定**：所有新創建的元件，都會透過 `SendMessage(hButton, WM_SETFONT, ...)` 來設定客製化的字型與大小，並使用 `SetWindowPos(hLabel, HWND_TOP, ...)` 確保文字標籤不被遮擋。

* **監控點擊的方式 (Subclassing)**
    * **方法**：這是本專案 UI 能夠穩定運作的核心。我們放棄了傳統依賴父視窗轉發 `WM_COMMAND` 訊息的方式，而是直接對**每一個**我們創建的按鈕本身進行「子類化 (Subclassing)」。
    * **說明**：
        1.  在 `CreateAllButtons` 中，每當一個按鈕被 `CreateWindowExW` 創建出來後，程式會立刻呼叫 `SetWindowLongPtrW(hButton, GWLP_WNDPROC, (LONG_PTR)CustomButtonProc)`。
        2.  這個操作將該按鈕原始的訊息處理程序替換為我們自己寫的 `CustomButtonProc` 函式。
        3.  在 `CustomButtonProc` 中，我們直接攔截最原始的滑鼠點擊訊息 `WM_LBUTTONUP`。一旦收到，就代表一次點擊完成。
        4.  這使得我們的按鈕**不再依賴父視窗的訊息轉發**，也**無需關心主視窗是否處於啟用狀態**，從而實現了無論何時都能「一鍵觸發」的穩定效果。
        5.  `CustomButtonProc` 在處理完我們的點擊邏輯後，會將其他所有訊息（如繪圖、滑鼠懸停等）交還給按鈕原始的處理程序，以保證按鈕的外觀和基本行為正常。