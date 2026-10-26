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

依 PROTOCOL.md「養成同步」節，只做讀取與事件；`care` 見下方階段 2，`depart`、`adopt`、紀念冊命令仍屬後續階段。

- 狀態快照新增欄位有出現才代表支援，缺少時維持舊行為；出現但格式錯誤（傷心原因不在四種或重複、蛋與終止個體非空陣列、秒數不是 0～4294967295 整數、健康卻有生病秒數、存活卻有死亡原因、紀念冊筆數與 hash 只出現一個或格式錯）就拒絕整份快照。
- pet 新增：`sadReasons`（韌體 `sad_reasons` 轉成 `{device:true,mood,satiety,cleanliness,sick}`，PetRules 看到 `device` 就直接採用、不再自行推算；舊韌體仍由本機門檻計算）、`dangerSeconds`、`sickSeconds`、`deathCause`（未知值記為 `unknown`，畫面顯示原因未知）。
- metadata 新增：`petEvents`（`features` 含 `pet_events` 且連線就緒）、`rules`（`device_info.rules` 的正整數，斷線即清除；格式錯誤拒收該份 device_info）、`memorials`（頂層 `memorial_count`／`memorial_hash`，只保存供後續階段對帳，尚不讀取紀念冊）。
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
