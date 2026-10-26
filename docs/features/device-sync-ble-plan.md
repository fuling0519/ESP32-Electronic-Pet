# 裝置與 Web 養成同步（BLE 擴充草案）

更新日期：2026-10-26。狀態：**已定案**（使用者 2026-10-26 決定第 12 節 D1～D4）；介面已併入 [共用協定](../../esp32_pet_app/PROTOCOL.md)「養成同步」節，以該節為準。階段 1 已在兩端實作並經使用者實機驗收（2026-10-26）；階段 2（`care`）已在兩端實作並經使用者實機驗收；階段 3（`depart`／`adopt`）已在兩端實作並經使用者實機驗收；階段 4～5 尚未實作。本文保留設計理由與分工；實作與驗證進度只記在 [主台帳](../project-plan.md#progress)。

## 1. 目標與範圍

使用者 2026-10-24 決定（企劃第 11 節）：接上藍牙後，裝置與 Web 的資料全部雙向連動；韌體有的資訊 Web 都要有，Web 可以多存韌體沒有的資訊。2026-10-26 使用者要求把 ESP32 的所有養成流程接到 Web，本草稿是第一份介面設計。

**涵蓋：** 狀態補齊、裝置事件、日常照顧（餵食／清潔／治療／入睡／起床）、送別、領養新蛋、紀念冊讀取與刪除。

**不涵蓋：**

- Deep Sleep 遠端控制：進入後 BLE 斷線，且正式版只由 SW 喚醒，Web 無從叫醒。
- 從 Web 玩電子雞上的猜拳／記憶音符：Web 陪玩已改為丟玩具／花園合唱，見 [Web 陪玩 BLE 規則](web-game-ble-plan.md)。
- 帳號、配對擁有權與雲端（W3／W4）、韌體 OTA。
- 展示模式（Mock）：仍是獨立的展示寵物與本機紀念冊，不參與同步。

### 設計原則

1. **韌體是唯一裁判。** Web 只送「要做什麼」，從不送數值；可否執行、增減多少、是否升級，全由韌體決定並回報實際增量。
2. **走搖桿同一條路。** 遠端命令在 Application 轉成既有的 `UiAction`（Feed／Clean／Treat／StartNormalSleep／Wake／SendOff／AdoptNewEgg／DeleteMemorial），資格、保存、升級慶祝與音效和搖桿操作一致，不另寫一套規則。
3. **每個寫入命令都可安全重送。** 以 `op` 識別「玩家按了一次按鈕」，回覆遺失時重送不會餵兩次（第 5 節）。
4. **指定目標個體。** 每個寫入命令帶 Web 看到的 `pet_id`；個體已換回 `pet_changed`，避免舊畫面替新寵物做事。
5. **沿用 v1。** 只新增命令、事件、可選欄位與 `features` 旗標；`v` 仍為 1，UUID、分片與 1024 bytes 上限不變。舊 App 忽略新欄位，照舊唯讀。
6. **快照為準，事件只是提示。** `pet_event` 可能因斷線遺失，Web 只用它決定播哪段動畫；數值與狀態永遠以完整 `status` 快照為準。
7. **電子雞不會被卡住。** 遠端命令不會讓電子雞停在等待畫面；玩家正在選單或狀態頁時，韌體先退回主畫面再執行（D2），未保存的選擇視同取消。
8. **不可逆操作由 Web 確認。** 送別、刪除紀念紀錄只在 Web 跳確認框，電子雞不再確認（D3）；韌體收到命令就直接執行。

## 2. 能力宣告

`device_info.features` 新增四個旗標，各自對應一組命令；缺少就不顯示該功能的裝置模式入口（舊韌體收到新命令回 `unknown_command` 也照此處理）。

| 旗標 | 內容 | 階段 |
|---|---|---|
| `pet_events` | 第 4 節 `pet_event` 事件 | 1 |
| `care` | 第 6 節 `care` 命令 | 2 |
| `lifecycle` | 第 7 節 `depart`、`adopt` | 3 |
| `memorials` | 第 8 節 `get_memorials`、`delete_memorial` | 4 |

`device_info` 另加可選欄位，供 Web 顯示倒數與滿額提示，不讓 Web 自己寫死門檻（快速測試版門檻不同）：

```json
{"v":1,"type":"device_info","id":1,"name":"ESP32-PET","device_id":"esp32-AABBCCDDEEFF","protocol_version":1,"firmware_version":"…","max_message_bytes":1024,
 "features":["web_game","pet_events","care","lifecycle","memorials"],"games":["toy","chorus"],
 "rules":{"sick_after_seconds":21600,"death_after_sick_seconds":86400,"memorial_limit":32}}
```

## 3. 狀態快照擴充（階段 1）

第 3 節的欄位依「有出現才代表支援」判斷（同 1.4.0 的等級欄位），不另設旗標；缺少時 Web 維持現有行為。

### 3.1 `pet` 內新增

| 欄位 | 型別／值 | 規則 |
|---|---|---|
| `sad_reasons` | 字串陣列，元素為 `mood`、`satiety`、`cleanliness`、`sick` | 韌體依第 4.2.2 節含緩衝的判斷（心情 ≤40 進入、≥45 解除；飽食／清潔 ≤25 進入、≥30 解除）。空陣列代表不傷心；蛋與終止個體固定為空。Web 有此欄位時不再自行推算，避免兩邊緩衝狀態不同步。 |
| `danger_seconds` | 整數 ≥0 | 飽食或清潔持續為 0 的累計秒數；達 `rules.sick_after_seconds` 生病。未處於危險區間為 0。 |
| `sick_seconds` | 整數 ≥0 | 生病後累計有效秒數；達 `rules.death_after_sick_seconds` 死亡。健康時為 0。 |
| `death_cause` | `untreated_sickness` | 只在 `is_dead:true` 時出現。 |

兩個秒數都在 uint32 範圍，JS 整數可安全表示，因此不用十進位字串。

### 3.2 頂層新增

| 欄位 | 型別／值 | 規則 |
|---|---|---|
| `memorial_count` | 整數 0～33 | 紀念冊目前筆數（含溢位槽）。≥32 時不能送別；>32 時須先刪除才能領養。 |
| `memorial_hash` | 8 位小寫 hex | 紀念冊有效紀錄穩定編碼的 CRC32。內容不變就不變（重開機也一樣），Web 與快取不同才重讀第 8 節清單。 |

### 3.3 `activity` 新增三個值

| 值 | 意義 | 可接受的寫入命令 |
|---|---|---|
| `idle` | 主畫面閒置（沿用 `webGameStartAllowed()` 的條件） | `care` 的 feed／clean／treat／sleep、`depart`、`delete_memorial`、`game_start` |
| `menu`（新） | 停在可退回的選單或狀態頁，沒有進行中的動畫或保存（見下表） | 同 `idle`，但 `game_start` 仍回 `busy`（陪玩維持 2026-10-25 決定 A） |
| `sleeping`（新） | 一般睡眠畫面，沒有其他流程 | `care` 的 wake、`delete_memorial` |
| `ended`（新） | 死亡或遠行後的結束畫面（死亡紀念、結束選單、明信片）或從那裡進入的紀念冊瀏覽頁，動畫已播完、送別保存不在重試中 | `adopt`、`delete_memorial` |
| `device_game`／`web_game`／`busy` | 同既有定義 | 全部回 `busy` |

**`menu` 的範圍（D2）：** 韌體收到命令時先退回主畫面（從結束畫面進入的則退回結束畫面），再執行，和玩家長按返回的效果相同但不播取消音。

| 會退回並執行 | 仍回 `busy` |
|---|---|
| 主選單、主畫面選中狀態捷徑、餵食／清潔／休息的選擇頁、詳細狀態頁、紀念冊分類／說明／紀錄頁 | 刪除紀念紀錄確認頁、聲音設定（有未保存的選擇）、送別說明／確認／明信片動畫、猜拳／記憶音符、照顧／成長／升級／死亡動畫、Deep Sleep 準備、領養受阻頁 |

右欄都是「玩家正在做決定或有東西在播」的畫面，硬退回會吃掉玩家的操作或打斷動畫，所以不搶。

規格本來就要求未知值不可當成 `idle`，舊版 Web 收到新值只會顯示「電子雞在忙」，不會誤開功能。

## 4. 裝置事件 `pet_event`（階段 1）

讓 Web 對電子雞上發生的事播出相應動畫，例如有人在電子雞上餵食時，Web 也播吃飯。

```json
{"v":1,"type":"pet_event","pet_id":"42","event":"fed","source":"device"}
```

| `event` | 觸發 | 附加欄位 |
|---|---|---|
| `fed`、`cleaned`、`treated` | 照顧成功 | `source` |
| `slept`、`woke` | 進入／離開一般睡眠 | `source` |
| `hatched`、`grew` | 孵化、幼年長成成年 | — |
| `level_up` | 升級 | `level`（新等級） |
| `got_sick`、`died` | 規則觸發 | — |
| `departed`、`adopted` | 送別完成、領養新蛋（`pet_id` 為新個體） | `source` |
| `memorial_added`、`memorial_deleted` | 紀念冊變動 | `memorial_pet_id`、`source` |

- `source`：`device`（搖桿）或 `web`（本連線命令）。Web 自己的命令以第 5 節 `action_result` 為準，`source:"web"` 的事件可忽略，避免同一動作播兩次。
- 只在連線中送出，不排隊、不補發；斷線期間發生的事由重連後的快照呈現（例如直接顯示已長大，不補播成長轉場）。
- 傳送佇列滿時優先丟事件，不可擠掉狀態快照或命令回覆。

## 5. 寫入命令共通規則（階段 2～4）

### 5.1 格式與 `op`

```json
{"v":1,"id":21,"cmd":"care","action":"feed","pet_id":"42","op":"9f2c41d07ab3e815"}
```

- `op`：16 位小寫 hex，Web 在玩家每次按下按鈕時用 `crypto.getRandomValues` 產生；同一次操作的重送（包括斷線重連後）沿用同一個 `op`。命令 `id` 仍照 v1 規則配對回覆，但 `id` 每條連線重新起算，不能用來去重。
- 韌體在記憶體保留最近 8 筆已接受的 `op`，先於資格檢查查詢；回放、衝突、淘汰與斷線行為的定案細節見 PROTOCOL.md「寫入命令共通規則」。
- 同一 `op` 卻是不同命令、動作或目標，回 `op_conflict`。
- `op` 不寫入 NVS，重開機就忘記。為避免重開機後重複執行，Web 只在「同一頁面、尚未收到 `action_result`、且距第一次送出不到 30 秒」時重送；超過就放棄，提示玩家看最新狀態。

### 5.2 兩段式回覆

1. **`command_result`**：立即回覆。`ok:true` 代表已接受並排入執行，之後一定會送 `action_result`；拒絕時帶錯誤碼（`busy`、`pet_changed`、`not_allowed`、`memorial_full`、`not_found`、`op_conflict`、`invalid_value`、`missing_field`）。
2. **`action_result`**：真正執行完的結果。

```json
{"v":1,"type":"action_result","id":21,"op":"9f2c41d07ab3e815","action":"feed","outcome":"done","changes":{"satiety":20,"exp":3,"levels_gained":0}}
```

| `outcome` | 意義 |
|---|---|
| `done` | 已執行；`changes` 為實際增量（滿值時增量為 0）。保存時機同搖桿：睡覺／起床立即寫入，餵食／清潔／治療延後保存 |
| `failed` | 執行時狀態已改變或保存失敗，附 `code`（`not_allowed`、`pet_changed`、`save_failed`） |

需要兩段式的原因：治療要等倒藥動畫 1.2 秒後才結算，送別要等保存完成；全部都用同一種回覆方式，Web 只需一套等待邏輯。Web 等待 `action_result` 的上限為 10 秒。

### 5.3 電子雞端呈現

D1 已確認：與搖桿相同。遠端餵食在電子雞上同樣播 3 秒吃飯、開心與摸頭音；清潔播水面動畫與星芒；治療播倒藥與恢復星芒；有升級就接慶祝。Web 收到 `action_result` 後播自己的花園版動畫，兩邊各播各的，不要求逐格同步。

## 6. 日常照顧 `care`（階段 2）

```json
{"v":1,"id":21,"cmd":"care","action":"clean","pet_id":"42","op":"…"}
```

| `action` | `activity` 須為 | 寵物條件（沿用 `PetData`） | `changes` | 電子雞呈現 |
|---|---|---|---|---|
| `feed` | `idle`／`menu` | 幼年／成年、清醒、未終止；生病也可餵 | `satiety`、`exp`、`levels_gained` | 3 秒吃飯＋開心＋摸頭音 |
| `clean` | `idle`／`menu` | 同上 | `cleanliness`、`exp`、`levels_gained` | 2.4 秒水面＋星芒＋孵化音 |
| `treat` | `idle`／`menu` | 同上且生病中 | `exp`、`levels_gained`、`grew`（布林，治療後補成長） | 倒藥 1.2 秒後結算＋恢復星芒 |
| `sleep` | `idle`／`menu` | 幼年／成年、清醒、未終止 | — | 切到睡眠畫面；保存失敗為 `failed`／`save_failed` |
| `wake` | `sleeping` | 一般睡眠中、未終止 | `grew`（布林） | 起床回主畫面 |

- 數值已滿仍可餵食、清潔並播動畫，回 `done`、增量 0、不給 EXP，與搖桿規則一致。
- 不符合寵物條件（例如蛋、未生病卻治療）回 `not_allowed`；電子雞在第 3.3 節右欄的畫面回 `busy`。資格先檢查、通過才退回主畫面，被拒絕的命令不會打斷玩家。
- 不提供遠端 Deep Sleep。

## 7. 生命週期（階段 3）

### 7.1 送別 `depart`

```json
{"v":1,"id":40,"cmd":"depart","pet_id":"42","op":"…"}
```

- 條件：`activity` 為 `idle` 或 `menu`、幼年／成年、清醒、未終止（生病可送別，文案相同）；`memorial_count` ≥32 回 `memorial_full`。
- D3 已確認：只由 Web 確認。Web 先顯示與電子雞相同的資訊（名字、有效陪伴時間、「送別後無法召回」，預設選「留下」），玩家確認後才送命令；韌體收到後直接走既有流程（保存 → 取景拍照 → 明信片），活動變為 `ended`。
- `done` 表示遠行旗標已保存。紀念紀錄若第一次寫入失敗，`changes.memorial_saved` 為 false，電子雞照既有規則每 10 秒重試；寫入成功後以 `memorial_added` 事件與 `memorial_hash` 告知。
- Web 收到 `done` 後播自己的送別動畫（拍照、走遠、明信片），並把照片存進 Web 自己的紀念紀錄。

### 7.2 領養新蛋 `adopt`

```json
{"v":1,"id":41,"cmd":"adopt","pet_id":"42","op":"…"}
```

- `pet_id` 為已結束的那一隻；條件：目前個體已死亡或遠行、`activity` 為 `ended`、其紀念紀錄已保存、`memorial_count` ≤32（否則 `memorial_full`，Web 引導先刪除紀念紀錄）。
- 種類與名字一律由韌體硬體亂數產生，Web 不能指定。
- `changes`：`{"new_pet_id":"43","name":"Kororo","species_id":2}`；隨後的快照即為新蛋。
- 舊個體已結束且紀錄保留，Web 沿用現有的領養確認框即可。

## 8. 紀念冊同步（階段 4）

### 8.1 讀取 `get_memorials`

```json
{"v":1,"id":30,"cmd":"get_memorials","offset":0}
```

唯讀，不需 `op`。成功時先回 `command_result` ok，再送：

```json
{"v":1,"type":"memorials","id":30,"hash":"5c1e09ab","total":7,"offset":0,
 "items":[{"pet_id":"3","name":"Tamama","species_id":1,"age_seconds":"3600","kind":"resting","stage":"adult","protected":false}]}
```

- 每頁最多 5 筆，依 NVS 保存順序；`offset` ≥ `total` 回空陣列。最長名字與 ID 的 5 筆約 883 bytes，仍在 1024 以內。
- `kind`：`resting`（長眠）或 `departed`（遠行）。`stage`：`egg`、`baby`、`adult`、`unknown`（舊墓碑沒有記錄階段）。
- `protected`：目前已告別、尚未領養的那一筆。兩邊統一規則：這一筆不能刪。
- 讀到一半 `hash` 變了，就從 `offset` 0 重讀。

### 8.2 刪除 `delete_memorial`

```json
{"v":1,"id":31,"cmd":"delete_memorial","memorial_pet_id":"3","op":"…"}
```

- 以紀錄的 `pet_id` 指定。在同一份連續有效存檔內 `petId` 遞增不重複，所以不用索引（索引會因刪除而位移）。
- 條件：`activity` 為 `idle`、`menu`、`sleeping` 或 `ended`。電子雞正在瀏覽紀念冊時，依 D2 先退回主畫面（或結束畫面）再刪，避免游標停在被刪掉的紀錄上；正在刪除確認頁時回 `busy`。
- 找不到回 `not_found`（Web 視為已刪除、算成功）；`protected` 回 `not_allowed`。
- D3 已確認：只由 Web 確認，沿用 Web 紀念冊既有的刪除確認框；韌體直接刪除。

### 8.3 Web 對帳規則

Web 裝置紀錄的鍵為 `device_id` + `pet_id`，並核對名字與種類。NVS 完全重建後 ID 可能被重用，鍵相同但名字或種類不同時，視為不同紀錄。

連線後與 `memorial_hash` 改變時依序：

1. 送出待刪佇列；`not_found` 也算完成，`busy` 或斷線則留在佇列等下次。
2. 讀完整清單。
3. 裝置有、Web 沒有：新增 Web 紀錄，只有裝置欄位，Web 額外資訊留空。
4. 兩邊都有：裝置欄位以裝置為準，保留 Web 額外資訊（照片、陪伴日期、等級）。
5. Web 有、裝置沒有：視為已在電子雞上刪除，Web 也刪（雙向連動）。第一次對帳時若會移除舊紀錄，先顯示「同步後會移除 N 筆」讓玩家知道。

**斷線時刪除（D4 已確認）：** 玩家在斷線時刪除裝置紀錄，Web 把 `device_id`、`pet_id`、名字與種類存進 LocalStorage 的待刪佇列，紀錄在紀念冊顯示「待同步」，不再出現在一般列表的筆數裡。重連後第 1 步自動補刪，成功後移除紀錄與佇列項目。佇列項目屬於特定 `device_id`，連到別台電子雞不會送出；`protected` 被拒時取消刪除並恢復顯示。

展示模式的紀錄不參與對帳。

## 9. 選配（階段 5）

- 聲音設定：`set_sound`（`{"muted":true}`）與快照的 `sound` 欄位，讓 Web 能切換電子雞靜音。
- 觸摸：`pet_event` 增加 `petted`／`poked`，讓 Web 也播反應；觸摸不改數值，優先度低。
- 傳輸效率：協商較大 MTU、放寬每片 16 bytes 的上限（規格本來就允許），加快紀念冊讀取。

## 10. 錯誤碼新增

`op_conflict`（`op` 重用於不同命令）、`memorial_full`（紀念冊已滿）、`not_found`（指定的紀念紀錄不存在）、`save_failed`（只出現在 `action_result.code`）。既有的 `busy`、`not_allowed`、`pet_changed`、`invalid_value`、`missing_field` 照原意使用。

## 11. 傳輸量與實作影響

以最長名字與 ID 估算（JSON 不含空白）：

| 訊息 | 目前 | 擴充後 | 傳送時間（每 20 ms 一片 16 bytes） |
|---|---|---|---|
| `status` | 372 bytes | 562 bytes | 約 0.5 → 0.7 秒 |
| `memorials` 一頁（5 筆） | — | 883 bytes | 約 1.1 秒 |
| 完整紀念冊（33 筆，7 頁） | — | 約 6 KB | 約 8 秒，實際名字較短時更快 |
| `action_result` | — | 173 bytes | 約 0.2 秒 |
| `pet_event` | — | 144 bytes | 約 0.2 秒 |

靠 `memorial_hash` 避免每次連線都讀完整清單；只有內容變了才讀。

**韌體：**

- 傷心緩衝判斷目前在 `UiController::updateSadState()`，要抽到不依賴硬體的類別，讓 UI 與 BLE 共用同一份狀態。
- `GameRequest` 佇列擴充為通用的 BLE 請求，加 `op` 快取（8 筆，約數百 bytes RAM）。
- `makeStatus` 的 `StaticJsonDocument<768>` 與命令 filter 要放大；`memorial_hash` 在紀念冊變動時重算。
- `UiController` 需要「這頁可否退回」的判斷（第 3.3 節）與不播取消音的退回方法；Flash／RAM 增量逐階段記錄。

**Web：** `BleSource` 實作寫入命令、`op` 重送與 `action_result` 等待；DataManager 在裝置模式依 `features` 與 `activity` 開放按鈕；`memorials.js` 實作第 8.3 節對帳與待刪佇列。

**測試：** 韌體原生測試涵蓋每個命令的資格、`pet_changed`、`busy`、`op` 回放與衝突、確認逾時；Web 離線測試以假 BLE 跑相同情境。真實藍牙、動畫與音效仍需實機驗收。

## 12. 使用者決定（2026-10-26）

- **D1.** Web 照顧時，電子雞也播動畫和音效，與搖桿相同（第 5.3 節）。
- **D2.** 停在選單或狀態頁時也接受遠端命令，韌體自動退回主畫面再執行（第 3.3 節 `menu`）。哪些頁面可退回是依此決定整理的細部規則，實作前若要調整再改第 3.3 節表格。
- **D3.** 不可逆操作不在電子雞上確認，只由 Web 確認框確認（第 7.1、8.2 節）。
- **D4.** Web 斷線時刪除裝置紀念紀錄，存入待刪佇列、畫面標「待同步」，重連後自動補刪（第 8.3 節）。

## 13. 階段與分工

| 階段 | 範圍 | 風險 |
|---|---|---|
| 1 | 第 3、4 節：快照擴充與事件 | 只讀，最低 |
| 2 | 第 5、6 節：共通寫入規則與日常照顧 | 第一次讓 Web 改變真實寵物 |
| 3 | 第 7 節：送別、領養 | 不可逆，只靠 Web 確認 |
| 4 | 第 8 節：紀念冊讀取、刪除與對帳 | 資料對應與刪除 |
| 5 | 第 9 節：選配與整體實機回歸 | 低 |

每階段流程：

1. 主工作階段把該階段介面寫進 PROTOCOL.md，並更新主台帳。
2. 韌體與 Web 各一個子代理，在各自的 worktree 並行實作，只改各自的檔案；`project-plan.md` 與 PROTOCOL.md 由主工作階段統一修改。
3. 合併後跑原生測試、Web 離線測試與 `tools/build_all.ps1 -Auto`，做提交前企劃核對。
4. 使用者實機驗收後，才開始下一階段。

不建議多個階段同時進行：第 2～4 階段都會改 `BleLink.cpp`、`main.cpp`、`ble-source.js`、`data-manager.js`。

## 14. 已知限制

- **沒有配對驗證。** 附近任何能連上的 BLE Central 都能照顧、送別或刪除紀錄。緩解只剩只允許一個 Central，以及動畫中或玩家正在決定時回 `busy`。依 D3，送別與刪除不需在電子雞上確認，等於信任能連上的 Central；這是使用者接受的取捨，展示環境以外使用前應先做配對驗證。正式擁有權驗證屬 W3／BLE-S3。
- `op` 快取只在記憶體；回覆前重開機、Web 又在 30 秒內重送，仍可能重複執行一次。
- `pet_event` 可能遺失，只影響 Web 是否補播動畫，不影響數值。
- NimBLE 只允許一個 Central，Flutter App 與 Web 不能同時連線。
- Deep Sleep 期間斷線，Web 只能顯示最後收到的快照與時間。
