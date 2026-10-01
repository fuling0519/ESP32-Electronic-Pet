# 韌體容量與 BLE 交接報告

紀錄日期：2026-10-01。韌體版本：1.1.0；協定：BLE v1。

文件角色：當日建置證據與實機操作流程。本文「待驗收」只代表當日狀態；最新結果只更新 [主台帳 FW-CAP／BLE-S2](project-plan.md#progress)，不持續改寫歷史數字。當日使用未提交工作目錄，未建立 release tag；重現時需核對相同來源，不能只靠 1.1.0 字串認定內容相同。

## 容量結果

同一工作目錄、固定 Arduino framework、正常 esp32dev、release build、相同分割區比較：

- 舊 Arduino BLE：Flash 1,216,921／1,310,720 bytes（92.8%），靜態 RAM 53,748 bytes。於本次修改前重新建置確認。
- NimBLE 2.5.0＋device_id＋heap 診斷：Flash 690,841／1,310,720 bytes（52.7%），靜態 RAM 50,196 bytes（15.3%）；firmware.bin 697,344 bytes。
- Flash 淨減少 526,080 bytes（約 526.1 KB／513.8 KiB）；靜態 RAM 淨減少 3,552 bytes。
- App 分割區剩 619,879 bytes（约 619.9 KB／605.4 KiB）。已達原企劃不高於 80% 的初期空間目標。

這是整批變更的前後差值，不是只替換 BLE 函式庫的單獨實驗。firmware.bin 與 PlatformIO Flash used 使用不同計量方式，分別記錄，不混用。KB 採 1000 bytes、KiB 採 1024 bytes。

保留 framework 的 default.csv：NVS offset 0x9000／size 0x5000，兩個 OTA App 槽各 0x140000（1,310,720 bytes）。本次未改 partition、未新增 OTA 功能、未更動寵物與墓碑 NVS 格式。4 MB 是設定板型的值，仍需核對實際模組。

運行 heap 尚未實測。韌體在 BLE 初始化前後、連線／斷線及連線期間每 30 秒輸出 free／minimum／largest bytes，皆採 MALLOC_CAP_8BIT。static RAM 不能替代這些數據。

## 兩組共用的確定介面

以 [PROTOCOL.md](../esp32_pet_app/PROTOCOL.md) 為唯一通訊規格。廣播名稱 ESP32-PET、Service／Command／Event UUID、Write with response、Notify、4-byte 分片標頭及每片 16-byte JSON 不變。

新增 device_id 放在 device_info／status 頂層，格式為 esp32- 加工廠 MAC 原順序的 12 位大寫 hex，例如 esp32-AABBCCDDEEFF。公開 ID 不是配對憑證。讀取失敗不啟用 BLE；不使用假 ID。重養與清除寵物存檔不影響 ID。

限制一個 Central 連線。App 先訂閱 Event，再查裝置資訊与完整狀態；未訂閱時不推進通知分片，通知傳送失敗會重試該片。get_status 在狀態未變時仍回傳快照。命令仍只有 get_device_info／get_status，未開照顧、睡眠或 reset。

現有 Flutter 顯示可選取複製的 ID，驗證格式與同一連線內來源一致性。缺 ID 的舊韌體仍允許本機唯讀，雲端綁定／同步不得使用該裝置；不能以 pet.id 或掃描 remoteId 代替。

## Deep Sleep 與小遊戲的分工

韌體組負責容量、NimBLE 底層與後續 Deep Sleep／小遊戲。Web 組 W0 接收本報告與協定，確認目標瀏覽器、帳號／部署與 BLE 實機驗收，無須重新做容量遷移。

Deep Sleep 尚未實作：約定裝置入睡後 BLE 斷線，醒來重新初始化與廣播；Web 保留最後資料、提示離線，由使用者重連查新快照。斷線不等於已知睡眠，v1 不新增 deep 列舉。

小遊戲先做裝置本機玩法；結果若只改現有心情等欄位，前端沿用狀態通知。遊戲歷史、手機控制或新命令先另提規格。

仍未交付：saveGeneration／個體 UUID、revision、事件佇列、實體配對證據、雲端 API、遠端照顧。Web 計畫中的這些內容是後續工作。

## 建置及實機驗收

正常版與五個既有測試環境 build、Flutter analyze、Flutter Web release build 均已通過。Web build 有 CupertinoIcons 字型提示，現有程式只使用 Material Icons，建置成功。

各測試環境最終 Flash used／靜態 RAM（bytes）：

- esp32dev_death_test：690,525／50,196。
- esp32dev_growth_test：691,049／50,196。
- esp32dev_memorial_test：691,061／50,196。
- esp32dev_sleep_test：690,705／50,196。
- esp32dev_dirty_preview：354,225／24,728；此環境排除主 Application，未啟動 BLE，不能拿來代表正常產品容量。

重現正常版使用 `pio run -e esp32dev`；其餘環境以各自名稱建置。Flutter 使用 `flutter analyze` 與 `flutter build web --release --no-pub`。程式檢查通過不等於 radio 或養成實機驗收通過。

實機請由團隊依既有流程燒錄正常版，不執行 erase flash。依序驗收：

1. 開機畫面、搖桿、音效、照顧、一般睡眠與重開機保存正常。
2. 序列輸出顯示 device_id 與 firmware=1.1.0，記下 BLE 初始化前後 heap。
3. 以 Chrome／已知 OS 開啟現有 App，找到 ESP32-PET、訂閱通知並讀到與序列一致的 ID。
4. 確認 info／status 的頂層 ID 一致；連續查詢未改變的寵物也收到完整快照。
5. 實體照顧後狀態同步；中文名字及預設 MTU 下的分片重組正常。
6. 斷線後顯示舊資料，裝置重新可搜尋；重連取得最新值。重複至少十次，包含傳輸中斷線。
7. 重開機及領養新蛋後，裝置 ID 不變、寵物 ID 依既有規則變化。NVS 清除後穩定性可用獨立測試板驗證，不為此清除正式寵物。
8. 使用既有快速生命週期環境確認死亡／墓碑保存與領養，保留正式存檔。
9. BLE 持續連線操作至少十分鐘，記錄 heap 趨勢及最大連續區塊，觀察是否異常下降、重啟或卡住。

記錄 OS／瀏覽器／板卡、韌體及前端 commit、每項結果與序列數據。未通過 radio／heap／ID 穩定性驗收前，不宣稱前端硬體相容已完成。

本次沒有燒錄、清除存檔或執行實機操作。若需要退回旧 BLE，還原 BleLink 與 platformio.ini 的程式／依賴即可；存檔格式與分割區未改。

實作 API 依據：[NimBLE 官方遷移文件](https://h2zero.github.io/NimBLE-Arduino/md_1_8x__to2_8x__migration__guide.html) 及專案安裝的 2.5.0 標頭。
