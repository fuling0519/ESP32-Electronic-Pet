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
