# Web 資料層交接

規則及唯一進度來源為 `docs/project-plan.md`，BLE 格式依該文件指定的 `esp32_pet_app/PROTOCOL.md`。本文件僅說明介面及測試方式。

`app.js` → `petDataManager` → MockSource／BleSource。畫面不讀取 `PetSources`，也不修改 Adapter 的寵物物件。

`read()` 回傳複本：`{ pet, metadata }`。pet 使用 `speciesId`、`isDeparted`、`level`、`exp`、`expToNextLevel`、`stage`、`healthState`、`sleepMode`、三項需求、個體 ID、名字及有效年齡；metadata 保存 source、connection、receivedAt、stale 與訊息。BLE ID／年齡使用十進位字串，Mock 年齡使用安全整數。`view()` 僅為既有畫面推導 condition／health 別名，不另存一份狀態。疾病可與一般睡眠同時存在。

Mock 的 `command()` 處理照顧，`preview()` 處理展示控制，`tick(seconds)` 推進有效整秒；展示模式前景運作才呼叫 tick。BLE 只接受兩種唯讀查詢，DataManager 不會將展示操作轉送裝置。資料來源切換不共用個體或數值。

LocalStorage 保留 v1 envelope，相容舊展示 condition，寫入時改存獨立健康／睡眠欄位。個體 UUID、名字與需求保留；疾病／衰減計時餘數為新增欄位，舊存檔缺少的歷史計時無法重建，從零起算。舊 BLE 快取以 wire 重新驗證及映射；不從快取 pet 的舊欄位猜測。損毀存檔保留，明確重新試玩才清除展示存檔。

執行離線測試：

```powershell
node web-preview/offline-tests.cjs
```

測試使用隔離的 VM、記憶體 LocalStorage 與 DOM 替身，不連藍牙、不碰使用者實際瀏覽器存檔。測試並不代替實際瀏覽器渲染、藍牙權限或硬體驗收。

實際瀏覽器人工回歸：確認原美術、照顧動畫與數值；生病→睡眠→起床仍生病、治療後恢復；死亡→墓園→領養新 UUID；重新整理保留個體；切換裝置模式顯示空狀態或帶接收時間的舊快照，所有照顧按鈕停用。實機可用後，再依主台帳及協定驗收連線、通知、重連及深睡斷線。

## 10/9 規格對齊

以主企劃 10/9 版及其共用協定為準。新蛋各 50% 抽小鳥／小飛龍，失敗重試不重抽。角色外觀預覽可選小飛龍，僅改畫面；目前個體與紀念冊仍保存真正種類。裝置模式依 species_id 顯示，舊封包為小鳥，未知合法種類只顯示未知。

小飛龍的 19 格原點陣逐 byte 從正式 WyvernSprite.h 匯出，Web 以 3 倍單色遮罩呈現；不使用首頁背景上的裝飾龍當正式素材。睡眠／傷心／吃飯採不同幀率，紀念冊依各筆種類與階段繪製。

Mock 心情使用 ≤50 低需求倍率與加權睡眠進度；舊 10 分鐘睡眠餘數清零。照顧與遊戲依新版發 EXP，Lv20 MAX；遊戲獎勵依完成場次一次結算。Web 遊戲為丟玩具（5 球）與花園合唱（3 首），未完成退出不給（2026-10-25 起，原猜拳／記憶音符已移除）。遊戲視覺節奏與音訊能力仍需實際瀏覽器回歸，Node 不驗證揚聲器聽感。

送別保存獨立終止旗標，長眠／遠行分開；紀念冊保存失敗會阻止領養。LocalStorage 與 ESP32 NVS 是不同系統；前端無權清測試 namespace、燒錄或控制 Deep Sleep。

## 10/26 養成同步階段 1

依 PROTOCOL.md「養成同步」節，只做讀取與事件；`care` 見下方階段 2，`depart`、`adopt` 見階段 3，紀念冊讀取與刪除見階段 4。

- 狀態快照新增欄位有出現才代表支援，缺少時維持舊行為；出現但格式錯誤（傷心原因不在四種或重複、蛋與終止個體非空陣列、秒數不是 0～4294967295 整數、健康卻有生病秒數、存活卻有死亡原因、紀念冊筆數與 hash 只出現一個或格式錯）就拒絕整份快照。
- pet 新增：`sadReasons`（韌體 `sad_reasons` 轉成 `{device:true,mood,satiety,cleanliness,sick}`，PetRules 看到 `device` 就直接採用、不再自行推算；舊韌體仍由本機門檻計算）、`dangerSeconds`、`sickSeconds`、`deathCause`（未知值記為 `unknown`，畫面顯示原因未知）。
- metadata 新增：`petEvents`（`features` 含 `pet_events` 且連線就緒）、`rules`（`device_info.rules` 的正整數，斷線即清除；格式錯誤拒收該份 device_info）、`memorials`（頂層 `memorial_count`／`memorial_hash`；階段 4 以 hash 判斷是否要重新對帳，見下方）。
- 有快照秒數與 `rules` 時，傷心原因下方顯示生病／離世倒數；連線中以收到快照後經過的時間估算，為「約」值。缺任一項不顯示。
- `activity` 的 `menu`、`sleeping`、`ended` 都不是 `idle`，Web 陪玩維持不可開局並顯示原因。
- `pet_event` 只在連線就緒、宣告 `pet_events`、`pet_id` 為目前個體（`adopted` 除外）且 `source` 不是 `web` 時交給 `petDataManager.onPetEvent`；不改快取、不觸發狀態訂閱。畫面只據此播動畫：`fed`／`cleaned`／`treated` 播餵食／清潔／治療動畫，`level_up` 顯示 LEVEL UP；孵化與長大仍由快照的階段變化播轉場，死亡、遠行、領養與紀念冊由下一份快照處理。已有動畫、成長轉場或陪玩進行中時略過事件。

離線測試：`node --test --test-isolation=none web-preview/*tests.cjs`（本階段見 `pet-sync-tests.cjs`）。以上皆未接實機 ESP32 驗證。

## 10/26 養成同步階段 2（照顧命令）

- `BleSource.care(action)`：`action` 為 `feed`／`clean`／`treat`／`sleep`／`wake`，只在連線就緒且 `features` 含 `care` 時可用，一次只能有一個進行中。每次玩家操作以 `crypto.getRandomValues` 產生 16 位小寫 hex `op`，送 `{cmd:"care",action,pet_id,op}`，沿用單一寫入鏈。`command_result` ok 只代表受理；以 `op` 比對 `action_result`（回放的 `id` 可能不同），`done` 回傳 `{action,changes}`（只留非負整數增量與 `grew` 布林）。`command_result` 拒絕或 `outcome:"failed"` 以帶 `code` 的 Error 拒絕。每 10 秒未收到結果就以同一 `op` 重送（未就緒時略過該輪），距首次送出滿 30 秒放棄，回 `code:"timeout"`（結果不明）並重讀 `get_status`。`op` 只在記憶體，重新整理頁面即放棄。
- `petDataManager.command(kind)`：裝置模式且支援 `care` 時回傳上述 Promise（複本）；展示模式不變。metadata 新增 `care`、`careSending`。
- `device-care.js`（`PetDeviceCare`）：依 `activity` 與快照判斷按鈕（餵食、清潔、治療限生病、睡覺／起床共用 `rest` 按鈕：快照為一般睡眠時送 `wake`）。`feed`／`clean`／`treat`／`sleep` 需 `idle` 或 `menu`，`wake` 需 `sleeping`；不可用時與陪玩相同，按鈕變淡、提示放在 title，點擊只說明原因。錯誤碼（`busy`、`pet_changed`、`not_allowed`、`save_failed`、`op_conflict`、`invalid_value`、逾時）各有繁中說明。
- 畫面：送出時鎖住所有照顧與陪玩按鈕；收到 `done` 後才播花園版餵食／清潔／治療動畫，狀態列顯示實際增量（例如「餵好了：飽食度 +20、EXP +3。」），有升級顯示 LEVEL UP。睡覺／起床沒有花園動畫，只顯示結果。數值、睡眠與健康一律等下一份狀態快照，不在本機加總。韌體為這次網頁命令送出的 `pet_event`（`source:"web"`）在 BLE 層略過，不會重播動畫。未宣告 `care` 的韌體維持唯讀，照顧按鈕隱藏。

離線測試見 `care-sync-tests.cjs`（假 BLE 寫入與假計時器）。未接實機 ESP32 驗證，電子雞端動畫、音效與實際增量都尚未驗收。

## 10/26 養成同步階段 3（送別與領養）

- `BleSource.lifecycle(cmd)`：`cmd` 為 `depart`／`adopt`，只在連線就緒且 `features` 含 `lifecycle` 時可用，`pet_id` 為快照中的目前個體（領養時即已結束的那一隻）。與 `care` 共用同一條寫入路徑（`write()`）：每次操作一個新 `op`、以 `op` 比對 `action_result`、10 秒同 `op` 重送、30 秒放棄並重讀狀態、全部寫入命令一次只能有一個（`careSending` 代表任何寫入命令進行中）。`changes` 另保留 `memorial_saved` 布林與領養的 `new_pet_id`、`name`、`species_id`。
- `petDataManager.farewell()`／`adopt()`：裝置模式且支援 `lifecycle` 時回傳上述 Promise；展示模式不變。metadata 新增 `lifecycle`。
- `device-lifecycle.js`（`PetDeviceLifecycle`）：`shown()` 決定入口（送別：幼年／成年且未終止；領養：已死亡或遠行），`blocker()` 給不可用原因（送別需清醒且 `activity` 為 `idle`／`menu`；領養需 `ended`）。錯誤碼各有繁中說明；`memorial_full` 請玩家先在 ESP32 上刪除一筆紀念紀錄（Web 刪除裝置紀錄屬階段 4）。滿額不在 Web 預先判斷，由韌體回覆。
- 畫面：裝置模式在花園下方顯示「送別」，沿用 `#farewell-dialog`（名字、快照 `age_seconds` 的陪伴時間、「送別後無法召回」，預設焦點「留下」）；確認後才送 `depart`。`done` 只顯示結果（`memorial_saved:false` 時說明電子雞還在保存紀念紀錄），遠行由下一份 `is_departed:true` 快照驅動既有送別動畫，同一個體只播一次；明信片場景照片存進該裝置寵物的 Web 紀念紀錄（以 `device_id` + `pet_id` 比對）。結束畫面的「領養新蛋」在支援時於裝置模式顯示，沿用 `#adopt-dialog` 並改為裝置文案（種類與名字由電子雞決定）；`done` 顯示新名字與種類，之後的蛋快照照常顯示蛋與孵化。結果文字在之後 8 秒內不被快照的連線訊息蓋掉。本頁命令的 `departed`／`adopted` 事件（`source:"web"`）在 BLE 層略過；電子雞端的事件維持階段 1 行為，由快照驅動。

離線測試見 `lifecycle-sync-tests.cjs`。未接實機 ESP32 驗證，電子雞端的送別／領養流程、`memorial_saved:false` 重試與滿額回覆都尚未驗收。

## 10/26 養成同步階段 4（紀念冊同步）

依 PROTOCOL.md「養成同步」節的「紀念冊」段與企劃 §8.3、D3、D4；只在 `device_info.features` 含 `memorials` 時啟用，展示模式的紀錄一律不參與。

- `BleSource.getMemorials()`：從 `offset` 0 逐頁送 `get_memorials`（唯讀、不帶 `op`），檢查每頁 `hash`（8 位小寫 hex）、`total`、`offset` 與最多 5 筆的 `items`（`pet_id`、`name`、`species_id`（缺少視為 1）、`age_seconds`、`kind`、`stage`（含 `unknown`）、`protected`），任何一頁格式錯誤就拒絕整次讀取。途中 `hash` 改變就從 0 重讀，最多 3 次。回傳 `{hash,items}`。metadata 新增 `memorialSync`（連線就緒且宣告 `memorials`）。
- `BleSource.deleteMemorial(petId)`：與 `care`／`depart`／`adopt` 共用 `write()`（同一 `op` 每 10 秒重送、30 秒放棄、一次只能一個寫入命令），送 `{cmd:"delete_memorial",memorial_pet_id,op}`，不帶 `pet_id`。`not_found`／`not_allowed` 以帶 `code` 的 Error 拒絕。`petDataManager.deviceMemorials()`／`deleteDeviceMemorial(id)` 回傳複本 Promise。
- `memorial-sync.js`（`PetMemorialSync`）：連線就緒（info 與 status 都到）後、以及 `memorial_hash` 和該裝置上次同步的不同時對帳。順序：送出該 `device_id` 的待刪佇列 → 讀完整清單 → 裝置有、Web 沒有就新增（只有裝置欄位，照片、日期、等級留空）→ 兩邊都有就以裝置欄位覆蓋並保留 Web 額外資訊，同一隻的多筆每次連線紀錄合併成一筆 → Web 有、裝置沒有的已結束紀錄移除（雙向連動）。比對鍵是 `device_id` + `pet_id`，再核對名字與種類，ID 相同但名字或種類不同視為另一筆。例外：快照中目前已結束、但電子雞清單還沒有的那一隻（紀念紀錄可能還在保存）保留不刪；仍存活個體的首次看見紀錄照舊保留。上次同步的 hash 與佇列存在 LocalStorage `aviary.memorial-sync`（依 `device_id` 分開）。
- 首次對帳若會移除紀錄，先跳出 `#memorial-sync-dialog`「同步後會移除 N 筆」並列出名字，要玩家選「同步並移除」或「先保留」。理由：這些多半是雙向同步前留下、可能帶有只存在網頁的照片，移除後無法復原；先保留對電子雞沒有任何影響，下次連線會再問。回答一次後，之後在電子雞上刪除的紀錄就直接同步移除，紀念冊說明列顯示移除了哪些。
- 若電子雞清單在上次同步後變了，送出佇列前先讀清單，佇列中 ID 已不在、或同 ID 換成別的名字／種類的項目直接視為已刪除、不送出，避免 NVS 重建後刪錯（比 §8.3 多一次讀取的安全延伸）。
- 佇列結果：`done` 或 `not_found` 移除紀錄與佇列項目；`not_allowed`（受保護的那筆）取消刪除、恢復顯示並說明；`busy`、逾時、斷線、`save_failed`、`op_conflict` 留在佇列，10 秒後或下次連線再送。`activity` 不是 `idle`／`menu`／`sleeping`／`ended` 時不送，等電子雞回到主畫面。
- 紀念冊畫面（`memorials.js`）：已同步的裝置紀錄沿用原刪除確認框，說明會一起刪除電子雞上的紀錄；確認後進入佇列並等 6 秒的復原時間，期間按「復原」取消；連線中時間到就送出，電子雞確認後顯示「已從電子雞刪除……無法復原」且不再提供復原。斷線時刪除標成「待同步」（淡色卡片、提示文字、「取消刪除」按鈕），不算進分頁筆數，重新整理後仍保留，連上該台電子雞後自動補刪；別台電子雞的佇列不會送出。電子雞標記 `protected` 的那筆、正在送出的那筆、以及電子雞還沒存進紀念冊的目前個體不能刪除，卡片上寫原因，選取模式也不能選。卡片依電子雞保存順序排列；裝置來的紀錄沒有日期時顯示「從電子雞同步，告別日期不明」，`stage:"unknown"` 畫成成年並寫「電子雞沒有記下成長階段」。尚未對帳過的裝置與舊韌體維持原本只刪本機的行為。舊版在本機刪除（隱藏）但電子雞仍有的紀錄，對帳後會依電子雞重新顯示。
- `pet_event` 的 `memorial_added`／`memorial_deleted`（`source` 不是 `web`）只觸發重新讀取清單；本頁刪除對應的 `source:"web"` 事件在 BLE 層略過，不會重複動作。

離線測試見 `memorial-sync-tests.cjs`（假紀念冊、假 GATT 寫入、假計時器與記憶體 LocalStorage）。未接實機 ESP32 驗證：真實分頁傳輸時間、電子雞端刪除畫面與 `memorial_hash` 更新時機都尚未驗收。

## 10/26 養成同步階段 5（遠端聲音開關與觸摸事件）

依 PROTOCOL.md「養成同步」節；韌體端與規格同步另行交付。只在 `device_info.features` 含 `sound` 時啟用聲音開關；觸摸事件需 `pet_events`。舊韌體兩者都不出現，行為不變。

- 狀態：頂層 `sound` 為 `"on"`／`"muted"`，其他值拒收整份快照（同其他格式錯誤欄位）；缺少時視為未支援或未知。metadata 新增 `soundControl`（連線就緒且宣告 `sound`）與 `sound`（`'on'|'muted'|null`，只取自最近快照）。
- `BleSource.setSound(muted)`（經 `petDataManager.setDeviceSound(muted)`）：送 `{cmd:"set_sound",muted,op}`，不帶 `pet_id`；和 `care` 共用 `write()`（同一 `op` 每 10 秒重送、30 秒放棄、一次只能一個寫入、以 `op` 對應 `action_result`）。成功回 `{action:'set_sound',changes:{muted}}`；`busy`、`save_failed`、`missing_field`、`invalid_value`、`op_conflict` 以錯誤碼拒絕。
- `device-sound.js`（`PetDeviceSound`）：裝置模式且有 `sound` 時在「送別」列下方顯示「電子雞聲音：開／靜音」按鈕（`#device-sound-button`，與頁面右上角自己的「網頁聲音」無關）。標籤只依快照，按下後不自行改標籤，等下一份快照；送出中停用，`activity` 不是 `idle`／`menu`／`sleeping`／`ended`（或快照沒有 `sound`）時變淡並以提示文字說明原因，點擊時顯示原因而不送命令。
- 觸摸：`pet_event` 新增 `petted`／`poked`（`source:"device"`、目前個體）。`device-sound.js` 呼叫 `PetTouch.remote()`（`pet-interactions.js`），條件與網頁觸摸相同（健康、清醒、幼年／成年、未遠行、無照顧／成長／升級動畫、無玩具／合唱／遊戲或送別對話框，且 `activity` 不是 `ended`），通過就播約 1.2 秒的摸摸愛心或驚嚇；不符合就靜默略過，只在主控台留一行 `[pet_event] … no animation: 原因`。事件不改狀態、不送命令。

離線測試見 `sound-touch-tests.cjs`。未接實機 ESP32 驗證：真實韌體的 `sound` 欄位、`set_sound` 回覆時序、`busy` 條件與觸摸事件頻率都尚未驗收。
