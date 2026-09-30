# 實作內容說明

更新日期：2026-09-26

## 目標

依下載資料 `index.html` 的掌上型寵物與硬體儀表板視覺，建立 Flutter 跨平台原型；將 ESP32 NodeMCU-32S 硬體企劃、狀態字型／字圖規格與使用者新增的平台需求彙整到專案文件。

## 讀取並整理的來源

- `Downloads/index.html`：包含黃色掌上型外殼、LCD 寵物畫面、A／B／C 操作鍵、飽足／心情／清潔／年齡及開發者遙測的 HTML 片段。檔案引用 `handleButton()` 和 `resetPet()`，但沒有附上實作這些函式的 JavaScript，因此視為視覺稿，不當成已定義的行為規格。
- `Downloads/project-plan.md`：描述 NodeMCU-32S 離線電子寵物、搖桿操作、三項需求、疾病／治療／死亡／墓碑、一般睡眠與 Deep Sleep、NVS 保存及後續感測器規劃。
- `Downloads/StatusFont12.h`：U8g2 使用的 WenQuanYi Bitmap Song 12×12 子集，檔內提供字型來源及授權註記。
- `Downloads/StatusGlyphs.h`：OLED 狀態頁中文字圖名稱與 14–15×16 尺寸索引；它包含其他 `ui/glyphs/*.h` 檔案，單獨這個索引檔不足以取得全部點陣圖。

## 已建立或更新的檔案

- `lib/main.dart`：建立 Flutter 寵物控制介面原型，包含 ESP32 BLE 搜尋／連線、A／B／C 遠端按鍵、寵物螢幕風格、飽食度／心情／清潔度進度及年齡。數值等 ESP32 回傳後才顯示；未連線或尚未收到資料時顯示 `--`。
- `PROTOCOL.md`：定義跨平台共用的 BLE GATT Service、Command、Event UUID，以及 JSON 命令、ack 與狀態通知範例。欄位使用 `satiety`（飽食度）、`mood`（數值）與 `mood_text`（顯示文字），避免把飽足度誤當飢餓百分比。
- `docs/project-plan.md`：在硬體原企劃副本上新增 Flutter 原生 App／Web 平台、HTML 介面映射、中文字型與中文字圖處理原則、資料來源要求、BLE 驗收方向，並列出睡覺、心情不好、吃飯、玩樂、排便、喝水作為下一步共同討論的候選資訊／互動。原始下載檔沒有被覆寫。
- `README.md`：補充 Windows、手機與 Flutter Web 的開發／建置指令，說明瀏覽器 BLE 的適用限制。
- `setup_windows.bat`、`run_windows.bat`、`build_windows.bat`、`run_web.bat`、`build_web.bat`：在 Windows 安裝 Flutter SDK 後建立平台骨架、啟動或建置 Windows／Web 版本。

## 整合原則

- ESP32 是寵物狀態與離線養成的權威來源；App／網站是控制與檢視介面，不替 ESP32 推算或保存假資料。
- 原硬體的搖桿操作保留。A／B／C 是 HTML 設計中的虛擬遙控鍵，不能據此新增實體按鍵 GPIO。
- `StatusFont12.h` 是 C++／U8g2 字型資料，`StatusGlyphs.h` 是韌體中文字圖索引；兩者不直接作為 Flutter 字型載入。Flutter 先用一般 Unicode 中文字型呈現相同欄位名稱與狀態，韌體 OLED 繼續使用既有點陣資產。
- HTML 的 CPU／SRAM／Flash/NVS 數值是樣板內容。App 只有在韌體真的回報即時資料時才呈現遙測，不能把樣板數字當成設備實況。
- MVP 仍以原企劃的飽食度、心情、清潔度為需求值，不自動新增體力或口渴值。喝水、排便先作待討論互動，不預設它們一定是新的數值欄位。
- Flutter Web 的介面可在瀏覽器顯示；Web BLE 需要瀏覽器支援 Web Bluetooth 且從 HTTPS／localhost 安全來源開啟。iPhone Safari 網站不支援 BLE 時，應使用原生 iOS App。

## 尚未完成／驗收限制

- 本環境未安裝 Flutter／Dart SDK；沒有執行 `flutter pub get`、靜態分析、模擬器、手機、Windows 或瀏覽器建置。
- 平台骨架資料夾需在安裝 Flutter 的 Windows 開發機執行 `setup_windows.bat` 後產生；目前提供的是 Flutter 原始碼與建置腳本，不是已編譯的 `.exe` 或已部署網站。
- ESP32 韌體資料夾不在本專案內，BLE Peripheral 尚未實作，故掃描、連線及實際傳輸都未經硬體驗證。
- 目前 Flutter 畫面只顯示三項需求與年齡。睡眠／睡覺、心情不好、吃飯、玩樂、排便、喝水、生病／治療／死亡、墓碑／新蛋等完整畫面與互動流程尚待共同定義及實作。
- HTML 樣板使用 `hunger` 等名稱且心情預設值和韌體企劃不一致；協定草案已對齊為 `satiety` 與 `mood`，但需在韌體 BLE 實作前共同確認最終 schema 和預設狀態。
- `flutter_blue_plus` 目前需依其授權條款評估發布用途；商業發佈前須確認授權需求。

## Windows 上的下一步

1. 安裝 Flutter SDK 及 Visual Studio 的 **Desktop development with C++** 工作負載。
2. 解壓專案後執行 `setup_windows.bat`，產生 Windows、Android、iOS、Web 平台骨架並取得套件。
3. 執行 `run_windows.bat` 或 `run_web.bat` 檢視原型；Windows BLE 需有可用的 BLE 藍牙介面卡。
4. iOS App 建置仍需 macOS／Xcode；Web BLE 需支援 Web Bluetooth 的瀏覽器與安全來源。

## 下一步共同規劃

先討論使用者提出的六項候選情境——睡覺、心情不好、吃飯、玩樂、排便、喝水——各自要在 Flutter 顯示什麼、是否能遠端操作、對現有需求值有何影響，以及哪些資訊由 ESP32 回報。確認後再更新 UI、JSON schema 和韌體命令；在此之前不增加新的寵物需求值。
