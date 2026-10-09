# 小飛龍驗收（1.8.0）

最新增量：2026-10-09 的 1.8.1 已修復存檔失敗的連續重試、清理舊測試資料並燒錄正式版至 COM3；階段改為幼年／成年。備份、容量與驗收狀態見 [存檔修復](storage-recovery.md)。以下保留 1.8.0 初次實作紀錄。

2026-10-09 已實作（未提交工作目錄）；C++ WASI 回歸與建置通過，尚未上板。素材與全部行為見 [主企畫](project-plan.md)、[原始提案](previews/wyvern-proposal/plan.md)。真實 C++ UI 畫面見 [驗收預覽](previews/wyvern-implemented/ui-preview.png)。

## 版本與入口

| 環境 | 角色 | NVS |
|---|---|---|
| esp32dev | 正式新蛋小鳥／小飛龍各 50% | pet-save |
| esp32dev_wyvern_test | 固定成龍；閒置主頁 8 秒後傷心 | pet-w-sad-a |
| esp32dev_wyvern_baby_test | 固定幼龍；閒置主頁 8 秒後傷心 | pet-w-sad-b |
| esp32dev_wyvern_treatment_test | 固定生病成龍 | pet-w-treat-a |
| esp32dev_wyvern_treatment_baby_test | 固定生病幼龍 | pet-w-treat-b |

固定龍測試版 RESET 建立測試寵物，使用自己的 NVS，不改正式 pet-save。既有小鳥 sad／treatment 測試版仍固定小鳥。正式版現有小鳥存檔讀回仍是小鳥；新種類只在建立新蛋時抽選。此輪未清存檔、未燒錄。

在專案根目錄以 PlatformIO 建置或燒錄指定環境，產物為 `.pio/build/<環境>/firmware.bin`。請選測試環境驗收，勿為了看到龍刪除正式存檔。降回 1.7.x 的韌體不認識 Wyvern=2，不能保證讀回新版龍存檔。

## 實機檢查

1. 成龍／幼龍待機翅膀每格約 166.7 ms（6 FPS），生病／傷心每格 1 秒；幼龍一般睡眠每格 1 秒，成龍睡眠保持單格，Z／Zz／Zzz 仍沿用每 650 ms 字數切換。
2. 三條線與 Zzz 應符合已確認參考圖，龍角／翅膀／脚部未裁切。餵食四格各 375 ms、循環兩次；碗每秒滿→少→空，底部對齊龍腳。
3. 測試傷心版可在串口使用 0～9 preset，見 [傷心驗收](sad-state-test.md)。確認低需求與生病共用傷心圖、回復門檻及提示音沿用既有規則。
4. 治療：傷心第 1 格保留、三條線隱藏、藥水不黏到龍角；約 1.2 秒後治癒、空瓶 0.2 秒，再正常表情與兩側星光 2 秒。小鳥倒藥也隱藏三條線。符合治療條件必成功，沒有機率失敗。
5. 升等角色仍跳躍兩秒；成龍角頂與 LEVEL UP! 保留一行空隙。治療若觸發升等，恢復特效先完成；成長／死亡沿用原有優先順序。
6. 送別明信片、紀念冊遠行／長眠、死亡消散都應顯示正確階段的小飛龍。混合小鳥／小飛龍紀念冊切換不串種類。
7. 重啟、一般睡眠及 Deep Sleep 回來保留種類、身分與數值。正式新蛋各次獨立抽選，連續同種類合理，不能要求每十顆剛好各五。
8. BLE 狀態含整數 species_id（1／2）；新版 App 孵化後顯示種類與對應 emoji，蛋顯示共用蛋。Flutter 改動已完成但本機缺 SDK，解析测试及 App 建置未執行；需手機驗收。

## 自動檢查

- Windows 原生入口：`tools/test_rps.ps1`（需要 gcc/g++、既有 U8g2 來源，以及允許執行編譯出的 EXE）。
- 本機應用程式控制阻擋 PE 時：`tools/test_wasi.py --node <現有Node完整路徑>` 使用 `.pio/host-tools/zig-windows-x86_64-0.13.0/zig.exe`；C++11 測試、真實 byte codec／NVS 故障模型及 U8g2 字型在 Node WASI 執行。WASI 模型不等於 ESP32 NVS 斷電驗收。
- 12 目標通過；證據 `.pio/wyvern-wasi-tests.log`。含舊 V1、混合紀念冊、同次保存重試不重抽、未知種類拒絕、6 FPS 切格邊界與 1 FPS 傷心／睡眠、照護／聲音／遊戲／生命週期回歸。
- 生成工具將 19 格轉成 6688 bytes 點陣；全格來源像素核對見 `.pio/wyvern-pixels-check.log`。真實 UI PBM 在 `.pio/rps-preview/wyvern-*`，由 `tools/render_wyvern_preview.py` 產生 PNG／GIF。
- 八環境建置通過：正式、Deep Sleep、既有小鳥傷心／治療、四個小飛龍環境；正式 Flash 733813／1310720 bytes（56.0%）、RAM 54508／327680 bytes（16.6%）。記錄 `.pio/wyvern-build.log`。未記為實機通過。
