# Flutter App 與 BLE 整合企畫

更新日期：2026-09-30

完整產品需求、韌體進度與驗收狀態以專案根目錄的 [專題企畫與進度追蹤](../../docs/project-plan.md) 為準。本文件只保留 Flutter App 與 BLE 的平台規格，避免複製整份硬體企畫後再次產生進度落差。

## 目前階段

手機與連線目前停在 **Stage 2：唯讀 BLE 狀態整合，待硬體驗收**。

- Stage 1 已固定 BLE v1 的 Service／Command／Event UUID、4-byte 分片標頭、16-byte JSON payload、命令結果與完整狀態快照格式。
- ESP32 已實作 BLE Peripheral，可回覆 `get_device_info`、`get_status`，並在狀態改變時通知完整寵物快照。
- Flutter 已實作搜尋、連線、命令 ID 配對、分片重組、完整快照驗證，以及斷線後保留並標示舊資料。
- 遠端照顧、睡眠、A／B／C 控制與 reset 均未啟用；Stage 2 實機驗收前不進入 Stage 3。
- 共用協定以 [PROTOCOL.md](../PROTOCOL.md) 為準；整合檢查與硬體驗收步驟以 [INTEGRATION_PROGRESS.md](../../INTEGRATION_PROGRESS.md) 為準。

## 產品分工

- ESP32 是寵物狀態、計時、疾病／死亡判定、存檔與墓碑的唯一權威來源；App 不自行推算或保存另一份寵物狀態。
- ESP32 在 App 未連線、App 關閉或沒有網路時仍須完整運作。
- App 第一版以前景連線為範圍，不承諾背景常駐或雲端同步。
- Flutter 原生目標為 Android、iOS、Windows；Flutter Web 需在支援 Web Bluetooth 的瀏覽器及 HTTPS／localhost 安全來源執行。iOS Safari 不支援 Web Bluetooth 時須使用原生 App。
- 觸控、滑鼠與實體搖桿不得建立互相矛盾的狀態機；未來若開放遠端操作，所有命令仍由 ESP32 驗證及執行。

## 唯讀 v1 資料

目前完整快照包含：

- 寵物 ID、名字與生命階段。
- 飽食度 `satiety`、心情 `mood`、清潔度 `cleanliness`。
- 有效年齡秒數、健康狀態、死亡旗標與一般睡眠狀態。

App 未收到有效快照前顯示尚未取得，不使用示範數值冒充真實資料。未知版本、列舉值、缺少欄位或分片錯誤必須拒絕，不能猜成健康或清醒。

清潔玩法不建立排便事件或排泄量。ESP32 只保存清潔度；清潔度四階段在 OLED 上對應 0～3 個髒污圖，按下 Clean 使清潔度 `+30` 並依新階段減少或移除髒污。App 若顯示髒污，也應由 `cleanliness` 衍生，不新增另一份可不同步的狀態。

## 已完成檢查

- PlatformIO 正式環境編譯通過。
- `flutter analyze` 通過，沒有問題。
- Chrome Web Release Build 通過。
- 尚未完成 BLE 搜尋／連線、真實狀態通知、搖桿更新、斷線重連及離線養成互不干擾的硬體驗收。

## Flash 容量限制

加入完整 ESP32 BLE stack 後，正式韌體使用 1,216,921／1,310,720 bytes，為 App 分割區的 **92.8%**，僅餘約 93.8 KB。這不是整顆 4 MB Flash 的使用率，但已限制後續韌體功能成長。

Stage 3 前應先評估：

1. 將目前 BLE 實作遷移到較精簡的 NimBLE。
2. 若仍不足，再評估放大 App 分割區，以及因此造成的 OTA 雙槽取捨。
3. 每次新增大型函式庫或功能後記錄正式環境的 Flash 與 RAM 數字。

## Stage 2 硬體驗收

1. 不清除 NVS，燒錄正式 `esp32dev` 韌體。
2. 從 Chrome Web 或支援平台搜尋並連線 `ESP32-PET`。
3. 確認裝置資訊與真實寵物快照。
4. 用實體搖桿改變寵物狀態，確認 App 收到最新完整快照。
5. 測試斷線、重新連線與最新狀態恢復。
6. 確認 App 中止或斷線不影響 ESP32 本地養成、畫面、存檔與操作。

上述項目通過並記錄後，才規劃 Stage 3 遠端照顧、睡眠與不可逆操作的確認流程。
