# Web 10/9 規格整合驗證

日期：2026-10-16，Asia/Taipei。本機工作目錄 `D:/pet/ESP32-Electronic-Pet`，未提交／推送。

規格來源為 GitHub main 的完整主企劃，blob `713c4e8b6fe261e70f114afae4890d9b374f6da5`（10/9 版）；BLE 格式依同次取得的 PROTOCOL.md。主企劃保留原文及主管的實機回報，另於第 10 節新增本次 Web 記錄。沒有修改韌體檔案。

自動測試指令：`node web-preview/offline-tests.cjs`。22 組通過，0 失敗。涵蓋：

- 舊存檔／損毀保留、獨立健康與睡眠、UUID、64-bit BLE 數值。
- 兩種類共同照顧、治療、成長、疾病、死亡、送別與紀念紀錄。
- 領養種類各 50% 的兩種抽樣分支、同次失敗候選不重抽、重新載入種類保留。
- 新心情倍率、加權睡眠、同秒需求先變化及清醒心情扣分、疾病死亡時間截斷。
- 原小飛龍 19 格／6688 bytes、6 FPS／1 FPS／單格睡眠、375ms 四格餵食邊界。
- 猜拳／記憶音符的回合、揭曉／回饋等待、逾時、退出與一次結算。
- BLE 可選種類／遠行／EXP 的合法與非法組合、分片、版本中止、唯讀 allowlist。
- 保存退避與最新資料、明確重試、成功後重設。
- DOM 替身載入 game-ui.js／app.js，角色預覽不改種類、照顧、治療只給一次 EXP、送別標籤及模式切換。

JavaScript 語法與 `git diff --check` 另行檢查。原始素材來源保留在 wyvern-sprites.js 註解，資料逐 byte 匯出自正式 WyvernSprite.h；不是首頁插圖或 AI 重畫。

未驗證：真實瀏覽器像素／排版、手機與鍵盤操作、Web Audio 聽感及自動播放限制、JavaScript Adapter 的實機 BLE。現有自動化瀏覽器阻擋 file URL，因此未繞過限制或把 DOM 替身當成真實瀏覽器驗收。主管 10/9 的 Flutter／筆電回報保留，未套用到本次 JavaScript 版本。

Web 是本地展示及唯讀裝置檢視，不執行 ESP32 NVS 維護、燒錄或 Deep Sleep；紀念冊不是完整裝置歷史，也未複製裝置 32＋1 容量／刪除流程。外觀預覽與真正個體種類分開，紀念紀錄永遠用實際種類。
