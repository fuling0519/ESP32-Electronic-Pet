# ESP32-PET BLE 通訊協定 v1

更新：2026-10-01；對應韌體 1.1.0（NimBLE-Arduino 2.5.0）。協定 v1 以新增可忽略欄位維持向後相容。文件角色：共用介面規格；驗證與開發進度只看 [主台帳](../docs/project-plan.md#progress)。

本文件是韌體與 Flutter App 共用規格。ESP32 是 GATT Peripheral／Server；App 是 Central／Client。v1 先提供裝置資訊與唯讀狀態，不提供照顧、睡眠控制或重置。

## GATT

| 項目 | UUID | 屬性 |
|---|---|---|
| Service | `7d2a0001-8b7c-4f3a-9c2d-1e5f6a7b8c90` | Primary |
| Command | `7d2a0002-8b7c-4f3a-9c2d-1e5f6a7b8c90` | Write with response |
| Event | `7d2a0003-8b7c-4f3a-9c2d-1e5f6a7b8c90` | Notify |

保留既有 UUID，廣播名稱為 `ESP32-PET`。連線後先訂閱 Event，再依序查詢裝置資訊和寵物狀態。BLE write 成功只代表傳輸成功，不代表寵物操作成功。

## 分包傳輸

Characteristic value 承載 UTF-8 JSON 位元組；一筆 JSON 可拆成多片。不得假設 JSON 小於 180 bytes 或可放在單一 BLE 封包內。

每片前 4 bytes 是標頭：message ID（2-byte 無號小端序）、fragment index（1 byte，從 0 開始）、fragment count（1 byte，1–255）；其後是 JSON 原始位元組。每方向、每連線同時只傳一筆訊息，片段依序送出。接收端先重組位元組，再解碼 UTF-8／JSON，因此片段邊界可以切在中文字中間。

每個 characteristic value 不得超過當次實際 ATT payload（協商 MTU 減 3）；JSON 片段上限為該 payload 減 4。可協商較大 MTU 提升效率，但正確性不依賴特定 MTU。雙方 v1 實作目前固定每片最多 16 bytes JSON，因此封包最多 20 bytes，適用預設 MTU 23。v1 完整 JSON 上限 1024 bytes。遇到索引／片數錯誤、超限、重複片內容衝突或 3 秒未收完，丟棄整筆訊息。v1 不傳點陣圖。

## JSON 規則

- 所有訊息含整數 `v: 1`。
- 命令含 `id`，範圍 1–4294967295；同一連線內未完成的命令 ID 不得重用。
- 無效 UTF-8／JSON、缺欄位或型別錯誤時拒絕整筆訊息。
- 未知額外欄位忽略；不可用預設值補齊狀態。
- 未知命令回 `unknown_command`；未知事件型別可忽略。
- 不支援的協定版本要回報並停止命令流程。未知狀態列舉不可猜成健康、存活或清醒。
- 狀態通知必須是完整快照。App 驗證全部必填欄位後才套用。

## 命令與回覆

App → ESP32：

```json
{"v":1,"id":1,"cmd":"get_device_info"}
{"v":1,"id":2,"cmd":"get_status"}
```

ESP32 對可辨識命令回覆相同 ID；結果可選含供使用者閱讀的 `message`，App 依穩定的 `code` 判斷：

```json
{"v":1,"type":"command_result","id":1,"ok":true,"code":"ok"}
{"v":1,"type":"command_result","id":2,"ok":false,"code":"not_supported"}
```

若訊息損壞到無法取得命令 ID，回覆 `id:null`，App 不得配對到待處理命令。`ok:true` 只搭配 `code:"ok"`。成功查詢裝置資訊後另送：

```json
{"v":1,"type":"device_info","id":1,"name":"ESP32-PET","device_id":"esp32-AABBCCDDEEFF","protocol_version":1,"firmware_version":"<韌體實際版本>","max_message_bytes":1024}
```

成功查詢狀態後及每次狀態改變時，透過 Event Notify 發送完整快照：

```json
{"v":1,"type":"status","device_id":"esp32-AABBCCDDEEFF","pet":{"id":"42","name":"Tamama","life_stage":"baby","satiety":80,"mood":70,"cleanliness":95,"age_seconds":"3600","health":"healthy","is_dead":false,"sleep":"awake"}}
```

`max_message_bytes` 是重組後 JSON 上限，不是單封包大小。`firmware_version` 範例值不是目前韌體版本。

## 狀態欄位

| 欄位 | 型別／值 | 規則 |
|---|---|---|
| `pet.id` | 十進位字串 | 必填；避免 64-bit ID 在 Web 整數中失真 |
| `pet.name` | UTF-8 字串 | 必填，ESP32 提供 |
| `pet.species_id` | 整數 | 1.8.0 起提供：1 小鳥、2 小飛龍；舊封包省略時 App 預設 1，未知合法 ID 顯示未知種類 |
| `pet.life_stage` | `egg`、`baby`、`adult` | 必填 |
| `pet.satiety`、`pet.mood`、`pet.cleanliness` | 整數 0–100 | 必填；satiety 越高越飽 |
| `pet.age_seconds` | 非負十進位字串 | 必填；ESP32 計算，App 只格式化 |
| `pet.health` | `healthy`、`sick`、`dead` | 必填；未知值不可顯示為健康 |
| `pet.is_dead` | boolean | 必填；須與 `health == "dead"` 一致 |
| `pet.is_departed` | boolean | 1.6.0 新增；舊韌體缺省為 false。true 表示已遠行，不能與 is_dead=true、蛋或睡眠並存。健康欄位保留告別時值，UI 狀態優先顯示已遠行 |
| `pet.sleep` | `awake`、`normal` | 必填；不含 Deep Sleep |

寵物 ID 與年齡秒數使用十進位字串以保留跨平台精度。App 不自行扣需求值、計算成長或判定死亡。

## 錯誤碼

`malformed_message`（封包／UTF-8／JSON 無效）、`missing_field`（欄位缺少或型別錯誤）、`unsupported_version`、`unknown_command`、`invalid_value`、`busy`、`not_supported`、`internal_error`。不支援版本的回覆可附 `supported_versions`。

v1 只接受 `get_device_info`、`get_status`。A／B／C、照顧、睡眠與 reset 指令均未啟用，收到時回錯誤；新增命令前需同步更新本規格與兩端實作。

## 固定裝置 ID 與韌體組交接

韌體 1.1.0 在 `device_info` 及 `status` 的 JSON **頂層**增加 `device_id`，範例 `esp32-AABBCCDDEEFF`，格式為 `^esp32-[0-9A-F]{12}$`。以 `esp_efuse_mac_get_default` 回傳的六個工廠 MAC bytes 原順序轉大寫 hex；不是掃描 API 的 remoteId，也不跟隨 BLE 私有位址。固定硬體不因重啟、重養、NVS 清除或韌體重刷改變；換 ESP32 模組則改變。不要人工覆寫工廠識別。

此 ID 為公開定位欄位，不能證明裝置真品或帳號所有權。韌體讀取 ID 失敗時不啟用 BLE，不產生虛構 ID；離線養成仍可運行。

舊前端忽略新增欄位，仍可使用 v1；新前端收到舊韌體缺少 ID 時允許本機唯讀，明確提示不能用於雲端認領。若欄位存在但格式錯誤或同一連線的 info／status ID 不一致，拒絕套用該資料。雲端同步及綁定必須先取得有效 ID，不能改用 pet.id 或 remoteId。

廣播名稱、UUID、Write with response、Notify、1024-byte JSON 上限與分片標頭不變。NimBLE 限制一個 Central，Notify 的 CCCD 由函式庫建立，App 先訂閱再送命令。查詢 `get_status` 即使狀態未變也會再次傳完整快照。

## Deep Sleep 交接約定（未來介面需求）

一般睡眠 `normal` 可持續 BLE；Deep Sleep 會終止 BLE 連線，醒來重新初始化並廣播。前端顯示斷線、舊資料與最後收到時間，由使用者重新連線並查最新快照；不要求背景自動重連，也不能將所有斷線推斷為睡眠。

v1 不新增 `sleep:"deep"`，也不新增遠端睡眠命令。韌體不得在入睡前發布現有前端不接受的列舉；若將來需要可辨識睡眠預告，先另行擴充共用規格。恢復連線後回報實際可用狀態。

`saveGeneration`／個體 UUID、事件佇列、配對驗證、revision 和遠端命令不屬於本版 v1 介面；是否已實作與後續交付追蹤統一看主台帳。

規格中的拒絕規則是相容性要求，不等於已完成所有錯誤輸入測試。兩端分片重複／逾時／重新起始與版本錯誤處理仍需依 BLE-S1／BLE-S2 補測，不把文件敘述當作測試結果。

## 1.4.0 經驗與等級擴充（2026-10-02）

status.pet 新增可選成組欄位 `level`（整數 1～255）、`exp`（整數 0～65535）、`exp_to_next_level`（整數 0～65535）。目前韌體限制 Lv1～20，未滿等門檻為 50 + 25 × (level − 1)，EXP 是本級進度且小於門檻；滿等兩個 EXP 欄位都是 0，前端顯示 MAX。保留協定 v1 與既有 UUID／分片，上述寬範圍供後續調整玩法，App 不自行套升級公式。

舊 App 忽略新欄位；新 App 收到整組缺少時允許舊韌體快照，等級／經驗顯示 --。只缺部分、欄位為 null、型別或範圍錯誤、EXP 達到或超過正門檻、門檻為 0 卻有 EXP 時，拒絕整份快照。新韌體每次完整狀態都提供三欄，遊戲結算後沿用狀態通知；真正 Deep Sleep 仍斷線，醒後重新讀取。

送別不提供遠端操作命令。客戶端需更新才能辨識 `is_departed`；舊客戶端忽略新增欄位時，可能把最後健康快照顯示為仍在養。紀念冊仍只在裝置端瀏覽。
