# ESP32-PET BLE 通訊協定 v1

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
{"v":1,"type":"device_info","id":1,"name":"ESP32-PET","device_id":"esp32-pet-AABBCCDDEEFF","protocol_version":1,"firmware_version":"<韌體實際版本>","max_message_bytes":1024}
```

成功查詢狀態後及每次狀態改變時，透過 Event Notify 發送完整快照：

```json
{"v":1,"type":"status","device_id":"esp32-pet-AABBCCDDEEFF","pet":{"id":"42","name":"Tamama","life_stage":"baby","satiety":80,"mood":70,"cleanliness":95,"age_seconds":"3600","health":"healthy","is_dead":false,"sleep":"awake"}}
```

`max_message_bytes` 是重組後 JSON 上限，不是單封包大小。`firmware_version` 範例值不是目前韌體版本。

## 狀態欄位

### 裝置識別與未來帳號綁定

`device_info.device_id` 與 `status.device_id` 是同一個公開裝置識別字串，格式為 `esp32-pet-` 加上 12 位大寫十六進位字元。以上 ID 僅為示例。韌體讀取 ESP32 出廠 eFuse base MAC 產生此值，不需要 Wi-Fi 連線，也不寫入 NVS。重新開機、重新燒錄、清除寵物存檔與領養新蛋都不改變 ID；更換 ESP32 晶片則會改變。讀取失敗時不啟動 BLE，避免回報共用的假 ID。

這是 v1 新增欄位；舊 App 可以忽略，新 App 遇到沒有此欄位的舊韌體仍可查看寵物，但顯示「裝置 ID 尚未提供」。App 斷線及切換裝置時清除 ID，不以藍牙 remoteId 或廣播名稱代替。

未來 Web 後端可用 `device_id` 作為裝置唯一鍵，另存登入帳號的 `userId` 與裝置之間的擁有關係。此 ID 不是登入密碼或綁定憑證，不能只憑輸入 ID 就認領裝置。正式綁定仍需實體確認、短效一次性配對碼或裝置驗證憑證；這些流程及帳號後端尚未實作。`pet.id` 仍只代表裝置內的寵物，存檔重置後可能重用；上線寵物歷史前仍需加入存檔世代識別。

### 寵物資料

| 欄位 | 型別／值 | 規則 |
|---|---|---|
| `pet.id` | 十進位字串 | 必填；避免 64-bit ID 在 Web 整數中失真 |
| `pet.name` | UTF-8 字串 | 必填，ESP32 提供 |
| `pet.life_stage` | `egg`、`baby`、`adult` | 必填 |
| `pet.satiety`、`pet.mood`、`pet.cleanliness` | 整數 0–100 | 必填；satiety 越高越飽 |
| `pet.age_seconds` | 非負十進位字串 | 必填；ESP32 計算，App 只格式化 |
| `pet.health` | `healthy`、`sick`、`dead` | 必填；未知值不可顯示為健康 |
| `pet.is_dead` | boolean | 必填；須與 `health == "dead"` 一致 |
| `pet.sleep` | `awake`、`normal` | 必填；不含 Deep Sleep |

寵物 ID 與年齡秒數使用十進位字串以保留跨平台精度。App 不自行扣需求值、計算成長或判定死亡。

## 錯誤碼

`malformed_message`（封包／UTF-8／JSON 無效）、`missing_field`（欄位缺少或型別錯誤）、`unsupported_version`、`unknown_command`、`invalid_value`、`busy`、`not_supported`、`internal_error`。不支援版本的回覆可附 `supported_versions`。

v1 只接受 `get_device_info`、`get_status`。A／B／C、照顧、睡眠與 reset 指令均未啟用，收到時回錯誤；新增命令前需同步更新本規格與兩端實作。
