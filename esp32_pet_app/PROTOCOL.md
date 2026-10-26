# ESP32-PET BLE 通訊協定 v1

更新：2026-10-01；對應韌體 1.1.0（NimBLE-Arduino 2.5.0）。協定 v1 以新增可忽略欄位維持向後相容。文件角色：共用介面規格；驗證與開發進度只看 [主台帳](../docs/project-plan.md#progress)。

本文件是韌體與 Flutter App 共用規格。ESP32 是 GATT Peripheral／Server；App 是 Central／Client。v1 先提供裝置資訊與唯讀狀態；Web 陪玩與「養成同步」節的寫入命令只在韌體以 `features` 宣告後才可使用，不提供重置。

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

v1 基本命令為 `get_device_info`、`get_status`；宣告 `features` 含 `web_game` 的韌體另接受下方「Web 陪玩」節的四個遊戲命令；宣告 `care`、`lifecycle`、`memorials` 的韌體另接受「養成同步」節的對應命令。未宣告的命令、A／B／C 與 reset 指令均未啟用，收到時回錯誤；新增命令前需同步更新本規格與兩端實作。

## 固定裝置 ID 與韌體組交接

韌體 1.1.0 在 `device_info` 及 `status` 的 JSON **頂層**增加 `device_id`，範例 `esp32-AABBCCDDEEFF`，格式為 `^esp32-[0-9A-F]{12}$`。以 `esp_efuse_mac_get_default` 回傳的六個工廠 MAC bytes 原順序轉大寫 hex；不是掃描 API 的 remoteId，也不跟隨 BLE 私有位址。固定硬體不因重啟、重養、NVS 清除或韌體重刷改變；換 ESP32 模組則改變。不要人工覆寫工廠識別。

此 ID 為公開定位欄位，不能證明裝置真品或帳號所有權。韌體讀取 ID 失敗時不啟用 BLE，不產生虛構 ID；離線養成仍可運行。

舊前端忽略新增欄位，仍可使用 v1；新前端收到舊韌體缺少 ID 時允許本機唯讀，明確提示不能用於雲端認領。若欄位存在但格式錯誤或同一連線的 info／status ID 不一致，拒絕套用該資料。雲端同步及綁定必須先取得有效 ID，不能改用 pet.id 或 remoteId。

廣播名稱、UUID、Write with response、Notify、1024-byte JSON 上限與分片標頭不變。NimBLE 限制一個 Central，Notify 的 CCCD 由函式庫建立，App 先訂閱再送命令。查詢 `get_status` 即使狀態未變也會再次傳完整快照。

## Deep Sleep 交接約定（未來介面需求）

一般睡眠 `normal` 可持續 BLE；Deep Sleep 會終止 BLE 連線，醒來重新初始化並廣播。前端顯示斷線、舊資料與最後收到時間，由使用者重新連線並查最新快照；不要求背景自動重連，也不能將所有斷線推斷為睡眠。

v1 不新增 `sleep:"deep"`，也不新增遠端 Deep Sleep 命令（一般睡眠的入睡／起床見「養成同步」節）。韌體不得在入睡前發布現有前端不接受的列舉；若將來需要可辨識睡眠預告，先另行擴充共用規格。恢復連線後回報實際可用狀態。

`saveGeneration`／個體 UUID、事件佇列、配對驗證與 revision 不屬於本版 v1 介面；遠端命令只限「Web 陪玩」與「養成同步」兩節所列；是否已實作與後續交付追蹤統一看主台帳。

規格中的拒絕規則是相容性要求，不等於已完成所有錯誤輸入測試。兩端分片重複／逾時／重新起始與版本錯誤處理仍需依 BLE-S1／BLE-S2 補測，不把文件敘述當作測試結果。

## 1.4.0 經驗與等級擴充（2026-10-02）

status.pet 新增可選成組欄位 `level`（整數 1～255）、`exp`（整數 0～65535）、`exp_to_next_level`（整數 0～65535）。目前韌體限制 Lv1～20，未滿等門檻為 50 + 25 × (level − 1)，EXP 是本級進度且小於門檻；滿等兩個 EXP 欄位都是 0，前端顯示 MAX。保留協定 v1 與既有 UUID／分片，上述寬範圍供後續調整玩法，App 不自行套升級公式。

舊 App 忽略新欄位；新 App 收到整組缺少時允許舊韌體快照，等級／經驗顯示 --。只缺部分、欄位為 null、型別或範圍錯誤、EXP 達到或超過正門檻、門檻為 0 卻有 EXP 時，拒絕整份快照。新韌體每次完整狀態都提供三欄，遊戲結算後沿用狀態通知；真正 Deep Sleep 仍斷線，醒後重新讀取。

客戶端需更新才能辨識 `is_departed`；舊客戶端忽略新增欄位時，可能把最後健康快照顯示為仍在養。遠端送別與紀念冊讀取／刪除見「養成同步」節，未宣告對應 `features` 的韌體仍只在裝置端操作。

## Web 陪玩（2026-10-25 定案）

設計理由、雙端畫面與分工見 [Web 陪玩 BLE 規則](../docs/features/web-game-ble-plan.md)；本節是介面規格。實作與驗證進度只看主台帳。

**能力與狀態。** `device_info` 新增可選字串陣列 `features`，含 `"web_game"` 才支援本節命令；可選字串陣列 `games` 列出可開局的遊戲（目前 `["toy","chorus"]`），缺少時只有 `toy`（2026-10-25 第一版韌體）。`status` 頂層新增可選字串 `activity`：`idle`（主畫面閒置，可開局）、`device_game`（電子雞上的猜拳／記憶音符中）、`web_game`（本連線的 Web 陪玩中）、`busy`（其他頁面或流程）。缺少時視為舊韌體，不開放裝置模式陪玩；未知值不可當成 `idle`。`activity` 改變時推送完整快照。

**命令**（皆先回 `command_result`）：

```json
{"v":1,"id":7,"cmd":"game_start","game":"toy","pet_id":"42"}
{"v":1,"id":8,"cmd":"game_heartbeat","session":"3051"}
{"v":1,"id":12,"cmd":"game_finish","session":"3051","score":{"throws":5,"catches":3}}
{"v":1,"id":14,"cmd":"game_finish","session":"3052","score":{"correct":2,"total":3}}
{"v":1,"id":13,"cmd":"game_cancel","session":"3051"}
```

- `game_start`：`game` 為 `toy`（丟玩具）或 `chorus`（花園合唱），其他值回 `invalid_value`；`pet_id` 與目前個體不同回 `pet_changed`；寵物不是健康清醒的幼年／成年回 `not_allowed`；`activity` 不是 `idle` 回 `busy`。成功另送 `{"v":1,"type":"game_session","id":7,"session":"3051","game":"toy","min_ms":15000,"heartbeat_ms":5000,"timeout_ms":15000}`（`min_ms`：`toy` 15000、`chorus` 12000），電子雞切到「網頁陪玩中」。`session` 為十進位字串、非 0，每次開機重新起算。
- `game_heartbeat`：每 `heartbeat_ms` 一次；超過 `timeout_ms` 沒收到，韌體取消該局。局號不符回 `invalid_session`。
- `game_finish`：`score` 依遊戲驗證（`toy`：`throws` 必須為 5、`catches` 為 0～5 整數；`chorus`：`total` 必須為 3、`correct` 為 0～3 整數；不符或缺欄回 `invalid_value`，欄位型別錯誤回 `missing_field`）；開局未滿 `min_ms` 回 `too_early`；兩者都不結束該局。成功時由韌體套用獎勵（`toy` 固定心情 +6、EXP +10；`chorus` 依唱對首數 3／2／0～1 為心情 +10／+6／+3、EXP +15／+10／+5；心情上限 100）、解除鎖定並另送 `{"v":1,"type":"game_result","id":12,"session":"3051","mood_gain":6,"exp_gain":10,"levels_gained":0}`，增量為實際值。以同一局號重送時原樣回覆，不重複加值。
- `game_cancel`：不發獎勵並解除鎖定；對已結束的局重送也回 `ok`。

**韌體主動結束** 時送 `{"v":1,"type":"game_session_end","session":"3051","reason":"device_cancel"}`，`reason` 為 `device_cancel`（電子雞長按）、`heartbeat_timeout`、`pet_state`（生病、睡眠、成長、死亡、送別或換個體）。藍牙斷線時韌體立即取消該局（無法通知），重連後由 `activity` 得知。

**錯誤碼新增：** `not_allowed`、`pet_changed`、`invalid_session`、`too_early`。

**限制：** 沒有配對驗證，附近能連線的 Central 都能送結果；以單一連線、限主畫面開局、最短時長、單場上限與韌體換算獎勵降低風險。局號與最近一局結果只在記憶體，重開機不補發。

## 養成同步（2026-10-26 定案；階段 1 已實作）

設計理由、使用者決定（D1～D4）、Web 對帳與分工見 [裝置與 Web 養成同步](../docs/features/device-sync-ble-plan.md)；本節是介面規格。目前韌體只宣告 `pet_events`（快照擴充、`activity` 新值與事件），`care`、`lifecycle`、`memorials` 尚未實作；實作與驗證進度只看主台帳。

**能力。** `device_info.features` 新增 `pet_events`、`care`、`lifecycle`、`memorials`，各自開放下方對應的事件或命令；缺少即不支援。`device_info` 另加可選物件 `rules`：`sick_after_seconds`、`death_after_sick_seconds`、`memorial_limit`（皆為正整數），Web 以此顯示倒數與滿額，不自行寫死門檻。

**狀態快照（有出現才代表支援，缺少時維持舊行為）。** `pet` 新增：

| 欄位 | 型別／值 | 規則 |
|---|---|---|
| `pet.sad_reasons` | 字串陣列，元素 `mood`、`satiety`、`cleanliness`、`sick`，不重複 | 韌體含緩衝的傷心判斷；空陣列為不傷心，蛋與終止個體固定為空。有此欄位時客戶端不得自行推算 |
| `pet.danger_seconds` | 整數 0～4294967295 | 飽食或清潔持續為 0 的累計秒數 |
| `pet.sick_seconds` | 整數 0～4294967295 | 生病後累計有效秒數；健康時為 0 |
| `pet.death_cause` | `untreated_sickness` | 只在 `is_dead:true` 時出現；未知值顯示為未知原因 |

頂層新增 `memorial_count`（整數 0～255，含溢位槽）與 `memorial_hash`（8 位小寫 hex，紀念冊有效紀錄穩定編碼的 CRC32，內容不變就不變）。兩者須同時出現或同時缺少。

`activity` 新增：`menu`（停在韌體可自動退回的選單或狀態頁）、`sleeping`（一般睡眠畫面閒置）、`ended`（死亡或遠行後的結束畫面，或由其進入的紀念冊瀏覽頁，且無動畫與保存重試）。`game_start` 仍只接受 `idle`。可退回頁面的清單由韌體決定，客戶端只看 `activity`。

**事件 `pet_event`**（`pet_events`）：

```json
{"v":1,"type":"pet_event","pet_id":"42","event":"fed","source":"device"}
```

`event` 為 `fed`、`cleaned`、`treated`、`slept`、`woke`、`hatched`、`grew`、`level_up`（附 `level`）、`got_sick`、`died`、`departed`、`adopted`（`pet_id` 為新個體）、`memorial_added`、`memorial_deleted`（附 `memorial_pet_id`）。玩家操作類事件附 `source`：`device` 或 `web`。只在連線中盡力送出，不排隊、不補發；客戶端只用來選擇動畫，不得據此改變狀態。傳送佇列滿時優先捨棄事件。未知 `event` 忽略。

**寫入命令共通規則。** `care`、`depart`、`adopt`、`delete_memorial` 皆含必填 `op`：16 位小寫 hex，代表玩家的一次操作，重送沿用同值。韌體在記憶體保留最近 8 筆；同一 `op` 再來時不重新執行，已完成就重送相同的 `action_result`（`id` 換成本次命令 ID），仍在執行就再回 `command_result` ok；同一 `op` 搭配不同命令、動作或目標回 `op_conflict`。`op` 不保存，客戶端只在同一頁面、未收到 `action_result` 且距首次送出不滿 30 秒時重送。

回覆分兩段：`command_result` 立即回覆，`ok:true` 代表已接受並保證之後送出 `action_result`；拒絕時帶錯誤碼。執行完成後送：

```json
{"v":1,"type":"action_result","id":21,"op":"9f2c41d07ab3e815","action":"feed","outcome":"done","changes":{"satiety":20,"exp":3,"levels_gained":0}}
```

`outcome` 為 `done`（已執行並保存，`changes` 為實際增量）或 `failed`（附 `code`：`not_allowed`、`pet_changed`、`save_failed`）。客戶端等待上限 10 秒，逾時視為結果不明，重讀狀態。資格先檢查，通過後韌體才從 `menu` 頁退回主畫面（從結束畫面進入的退回結束畫面）並執行，電子雞照搖桿操作播放動畫與音效。

**`care`**（`care`）：

```json
{"v":1,"id":21,"cmd":"care","action":"feed","pet_id":"42","op":"9f2c41d07ab3e815"}
```

| `action` | `activity` | 寵物條件 | `changes` |
|---|---|---|---|
| `feed` | `idle`、`menu` | 幼年／成年、清醒、未終止 | `satiety`、`exp`、`levels_gained` |
| `clean` | `idle`、`menu` | 同上 | `cleanliness`、`exp`、`levels_gained` |
| `treat` | `idle`、`menu` | 同上且生病 | `exp`、`levels_gained`、`grew` |
| `sleep` | `idle`、`menu` | 同上 | 空物件 |
| `wake` | `sleeping` | 一般睡眠中、未終止 | `grew` |

數值已滿仍為 `done`，增量為 0。其他 `action` 回 `invalid_value`；`pet_id` 不符回 `pet_changed`；寵物條件不符回 `not_allowed`；`activity` 不符回 `busy`。治療在倒藥動畫結束後才結算，`action_result` 約晚 1.2 秒。

**`depart`／`adopt`**（`lifecycle`）：

```json
{"v":1,"id":40,"cmd":"depart","pet_id":"42","op":"…"}
{"v":1,"id":41,"cmd":"adopt","pet_id":"42","op":"…"}
```

- `depart`：`activity` 為 `idle` 或 `menu`，幼年／成年、清醒、未終止（生病可送別）；`memorial_count` 達 `rules.memorial_limit` 回 `memorial_full`。韌體不再要求裝置端確認，客戶端必須先自行確認。`done` 代表遠行旗標已保存；`changes.memorial_saved` 為 false 時，韌體依既有規則重試寫入紀念紀錄，成功後以 `memorial_hash` 反映。
- `adopt`：`pet_id` 為已結束的目前個體，`activity` 為 `ended`，其紀念紀錄已保存且 `memorial_count` 不超過 `rules.memorial_limit`（否則 `memorial_full`）。名字與種類由韌體亂數決定；`changes` 為 `new_pet_id`（十進位字串）、`name`、`species_id`。

**紀念冊**（`memorials`）：

```json
{"v":1,"id":30,"cmd":"get_memorials","offset":0}
{"v":1,"id":31,"cmd":"delete_memorial","memorial_pet_id":"3","op":"…"}
```

`get_memorials` 唯讀、不需 `op`，`offset` 為 0 起的整數；成功時另送一頁，每頁最多 5 筆，依保存順序：

```json
{"v":1,"type":"memorials","id":30,"hash":"5c1e09ab","total":7,"offset":0,"items":[{"pet_id":"3","name":"Tamama","species_id":1,"age_seconds":"3600","kind":"resting","stage":"adult","protected":false}]}
```

`kind` 為 `resting` 或 `departed`；`stage` 為 `egg`、`baby`、`adult` 或 `unknown`；`protected` 為目前已告別、尚未領養的那一筆。分頁途中 `hash` 改變時，客戶端從 `offset` 0 重讀。

`delete_memorial` 以紀錄的 `pet_id` 指定；`activity` 須為 `idle`、`menu`、`sleeping` 或 `ended`。找不到回 `not_found`（客戶端視為已刪除），`protected` 回 `not_allowed`。韌體不要求裝置端確認，客戶端必須先自行確認。

**錯誤碼新增：** `op_conflict`、`memorial_full`、`not_found`、`save_failed`（僅出現在 `action_result.code`）。

**限制：** 沒有配對驗證，且送別與刪除不需裝置端確認，附近能連線的 Central 都能執行；只靠單一連線與 `busy` 規則降低風險。`op` 快取只在記憶體，回覆前重開機又重送時可能重複執行一次。單片 16 bytes 下，擴充後的狀態約 0.7 秒、紀念冊每頁約 1 秒送完。
