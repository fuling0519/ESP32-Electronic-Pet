# Web 陪玩經 BLE 結算與雙端互鎖

更新日期：2026-10-25。狀態：**已定案**（使用者 2026-10-25 確認第 11 節 A、B、C 皆照草稿）；介面已併入 [共用協定](../../esp32_pet_app/PROTOCOL.md)「Web 陪玩」節，以該節為準。本文保留設計理由與分工，實作與驗證進度只記在 [主台帳](../project-plan.md#progress)。

## 1. 目標與範圍

使用者 2026-10-25 決定：Web 小遊戲要影響真實寵物；同一時間只能在一端陪玩，另一端顯示「正在陪玩」且不能做其他事。

本草稿只涵蓋「Web 陪玩一場的開始、維持、結算、取消」及「雙端顯示對方正在陪玩」。照顧、睡眠、送別等遠端命令仍屬 BLE-S3／W5，不在此範圍。

### 設計原則

1. **韌體是唯一裁判。** Web 只回報玩了什麼、成績如何；可否開始、獎勵多少、是否已發過，全部由韌體決定。Web 永遠不送心情或 EXP 數值。
2. **裝置本體永遠有出口。** 任何網頁端狀況（關分頁、手機休眠、斷線、網頁當掉）都不能讓電子雞卡在鎖定畫面；韌體長按一定能結束。
3. **沿用 v1。** 只新增命令、事件與可忽略欄位；`v` 仍為 1，UUID、分片與 1024 bytes 上限不變。舊 App 忽略新欄位，照舊唯讀。
4. **和韌體小遊戲同一把尺。** 單場心情最多 +10、EXP 最多 +15，且有最短時長，Web 不會比韌體更好刷。

## 2. 能力宣告

`device_info` 新增可選陣列 `features`。含 `"web_game"` 才代表韌體支援本草稿的命令；缺少時 Web 不顯示裝置模式的陪玩入口（舊韌體回 `unknown_command` 也照此處理）。

```json
{"v":1,"type":"device_info","id":1,"name":"ESP32-PET","device_id":"esp32-AABBCCDDEEFF","protocol_version":1,"firmware_version":"…","max_message_bytes":1024,"features":["web_game"]}
```

## 3. 狀態快照新增 `activity`

`status` 頂層新增可選字串 `activity`（與 `device_id` 同層，不放在 `pet` 內，因為它描述裝置正在做什麼）：

| 值 | 意義 | Web 顯示 |
|---|---|---|
| `idle` | 可以開始陪玩（主畫面閒置） | 陪玩入口可用（寵物也須符合第 5 節資格） |
| `device_game` | 有人正在電子雞上玩猜拳／記憶音符 | 等待畫面「牠正在電子雞上陪你玩」，陪玩停用 |
| `web_game` | 本連線正在進行 Web 陪玩 | 正常遊戲畫面 |
| `busy` | 照顧動畫、成長／升級、選單或設定頁等其他流程 | 陪玩暫時停用，提示「請先讓電子雞回到主畫面」 |

缺少 `activity` 視為舊韌體，Web 不開放裝置模式陪玩。未知值不可猜成 `idle`。`activity` 改變時韌體照常推送完整快照。

## 4. 命令

所有命令沿用 `{"v":1,"id":…,"cmd":…}`，ESP32 先回 `command_result`。新增欄位型別錯誤回 `missing_field`；值不合法回 `invalid_value`。

### 4.1 `game_start`

```json
{"v":1,"id":7,"cmd":"game_start","game":"toy","pet_id":"42"}
```

- `game`：Web 遊戲代號。首版只有 `toy`（丟玩具）；之後的 `chorus`（花園合唱）等需先在第 6 節表格登記，韌體不認得回 `invalid_value`。
- `pet_id`：Web 看到的寵物 ID。和目前個體不同回 `pet_changed`，避免舊畫面替新寵物開局。
- 寵物不符合資格（第 5 節）回 `not_allowed`；`activity` 不是 `idle` 回 `busy`。
- 成功時另送事件：

```json
{"v":1,"type":"game_session","id":7,"session":"3051","game":"toy","min_ms":15000,"heartbeat_ms":5000,"timeout_ms":15000}
```

`session` 是韌體產生的十進位字串（每次開機重新起算、每局遞增，不為 0）。韌體立刻切到「網頁陪玩中」畫面，`activity` 變為 `web_game`。

### 4.2 `game_heartbeat`

```json
{"v":1,"id":8,"cmd":"game_heartbeat","session":"3051"}
```

Web 每 `heartbeat_ms`（5 秒）送一次。韌體超過 `timeout_ms`（15 秒）沒收到心跳，取消這局。session 不符回 `invalid_session`。

### 4.3 `game_finish`

```json
{"v":1,"id":12,"cmd":"game_finish","session":"3051","score":{"throws":5,"catches":3}}
```

- `score` 的欄位依遊戲而定（第 6 節），韌體檢查範圍；不合理回 `invalid_value`，該局保持進行中，可再送正確結果或取消。
- 從開局起算未滿 `min_ms` 回 `too_early`，該局保持進行中。
- 成功時套用獎勵、解除鎖定、照常存檔與推送狀態，並另送：

```json
{"v":1,"type":"game_result","id":12,"session":"3051","mood_gain":6,"exp_gain":10,"levels_gained":0}
```

`mood_gain`／`exp_gain` 是實際增加量（心情接近 100 或滿等時會較少），Web 直接顯示這兩個數字。

**重送不重複發獎勵：** 回覆遺失時 Web 可用同一 `session` 重送；韌體記得最近一局的結果，原樣回覆 `ok` 與相同 `game_result`，不再加值。

### 4.4 `game_cancel`

```json
{"v":1,"id":13,"cmd":"game_cancel","session":"3051"}
```

不發獎勵並解除鎖定。對已結束的局重送也回 `ok`（冪等）。

### 4.5 韌體主動結束

韌體因任何原因提早結束一局時送：

```json
{"v":1,"type":"game_session_end","session":"3051","reason":"device_cancel"}
```

| `reason` | 情況 |
|---|---|
| `device_cancel` | 玩家在電子雞上長按結束 |
| `heartbeat_timeout` | 超過 15 秒沒有心跳 |
| `pet_state` | 生病、睡眠、成長、死亡、送別或換了個體 |
| `disconnected` | 藍牙斷線（只記錄於韌體，重連後 Web 由 `activity` 得知已解鎖） |

## 5. 開始資格

和韌體自己的陪玩一致：幼年／成年、健康、清醒、未死亡、未遠行；不是蛋。心情滿值仍可玩。另外電子雞必須在**主畫面閒置**（不在選單、狀態頁、設定、照顧／成長／升級動畫、Deep Sleep 準備或任何遊戲中）。

> 決定 A（2026-10-25）：只有主畫面閒置時可從 Web 開局；選單、狀態頁等一律回 `busy`，避免玩家正在電子雞上操作時畫面被搶走。

## 6. 遊戲與獎勵表（韌體內建）

| `game` | `score` 欄位 | 驗證 | 獎勵 | `min_ms` |
|---|---|---|---|---|
| `toy` 丟玩具 | `throws` 整數、`catches` 整數 | `throws == 5`、`0 ≤ catches ≤ 5` | 固定心情 +6、EXP +10 | 15000 |
| `chorus` 花園合唱（未定） | `correct`、`total` | 待遊戲設計 | 依答對比例，上限 +10／+15 | 待定 |

新增遊戲 = 在此表登記 + 韌體加一列規則 + 更新 `features` 或韌體版本；Web 不能自訂獎勵。

## 7. 雙端互鎖行為

### 電子雞在 Web 陪玩中（`web_game`）

- 畫面：寵物開心待機動畫（摸頭開心圖集循環）＋兩側狀態圖示；128×64 下方只放得下一行 12 px 字，所以「網頁陪玩中」與「長按結束」每 2 秒輪流顯示，取代原本的 EXP 列（決定 B）。
- 搖桿短按、方向與觸摸都忽略；**長按 = 結束這局（不發獎勵）**，送 `game_session_end`（`device_cancel`），回到主畫面。
- 養成時間照常推進；生病、成長、死亡等依既有優先順序切走並結束這局（`pet_state`）。
- 這段期間不進自動睡眠或 Deep Sleep；結束後才恢復原本的閒置計時。
- 斷線立即結束這局並回主畫面，不等心跳逾時。

### Web 在電子雞陪玩中（`device_game`）

- 花園顯示寵物與等待提示「牠正在電子雞上陪你玩」，陪玩入口停用；收到新的狀態快照時更新數值。
- 電子雞玩完後，狀態推送新的心情／EXP，Web 只顯示，不另外結算。

### 斷線與重連

- Web 斷線時遊戲畫面立刻停止並提示「藍牙斷線，這局沒有結算」；重連後依 `activity` 顯示。
- Web 不自動重送 `game_finish` 到新連線：斷線時韌體已取消那局。
- 同一局只屬於開局的那個連線（NimBLE 只允許一個 Central）。

## 8. 錯誤碼新增

`not_allowed`（寵物狀態不能玩）、`pet_changed`（`pet_id` 與目前個體不同）、`invalid_session`（局號不存在或已被韌體取消）、`too_early`（未滿最短時長）。既有的 `busy`、`invalid_value`、`missing_field` 照原意使用。

## 9. 已知限制

- **沒有配對驗證。** 附近任何能連上的 BLE Central 都能開局並送結果。緩解：一次只允許一個 Central、必須電子雞在主畫面閒置才能開局、有最短時長與單場上限、韌體自行換算獎勵。正式的擁有權驗證屬 W3／BLE-S3，不在本草稿。
- 局號與「最近一局結果」只存在記憶體；結算前斷電或重開機，該局不補發。
- 依賴 BLE-S2（實機唯讀連線）先驗收通過；目前實體 BLE 仍在驗證中。

## 10. 實作分工與順序（確認後）

1. 更新 PROTOCOL.md（本草稿定案版）與主台帳新增工作項。
2. 韌體：`BleLink` 解析新命令並交給 Application；Application 管理局號、心跳、逾時、資格與獎勵表；`UiController` 新增「網頁陪玩中」畫面與 `activity` 判定；原生測試涵蓋開局／心跳逾時／重送結算／太早結算／斷線／長按／狀態中斷。需補「網、頁、陪、玩、中」等中文字圖並記錄 Flash／RAM 增量。
3. Web：`ble-source.js` 新增命令與事件；DataManager 的 `startGame`／`answerGame` 在裝置模式改走 BLE；丟玩具在裝置模式結束時改送 `game_finish`、顯示韌體回傳的增量；`device_game` 等待畫面。
4. 驗證：兩端自動測試後，以實機確認整局結算、雙端互鎖畫面、斷線與長按出口。

## 11. 使用者決定（2026-10-25）

- **A.** 只有電子雞在主畫面閒置時才能從 Web 開局。
- **B.** 電子雞畫面採開心待機＋「網頁陪玩中」＋「長按結束」（兩行文字因版面輪流顯示）。
- **C.** 丟玩具最短 15 秒、心跳 5 秒、逾時 15 秒（一場估計 20 秒以上，尚未實測）。
