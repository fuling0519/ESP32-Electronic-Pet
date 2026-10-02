# 架構與模組邊界

文件角色：程式結構說明；2026-10-02 已核對原始碼。功能與驗收進度只在 [主企畫第 10 節](project-plan.md#progress)。

## 資料流

`main.cpp` 的 Application 擁有唯一 `PetData`，協調 Input、PetClock、UiController、Sound、Save、Memorials 與 Ble::Link。輸入轉成抽象事件，UI 回傳操作意圖，應用層執行照顧／保存。UI 持有唯讀寵物參考，不自行建立另一份養成狀態。

`PetClock` 把 millis 差值轉成整秒；Application 依清醒或一般睡眠呼叫 PetData 推進需求、年齡及疾病。UiController 依 displayRevision 更新畫面。真正斷電沒有可信時長時只恢復快照。

## 模組責任

- `include/HardwareConfig.h` 集中 GPIO 與硬體參數；`src/hardware/` 封裝 SH1106／U8g2、搖桿輸入及非阻塞音序。UI 不讀 GPIO，也不直接呼叫 U8g2。
- `src/pet/` 定義 PetData、PetClock、ABB 命名、版本 1 寵物快照與不變條件。生命階段依有效年齡與健康，與等級／EXP 分開。
- `src/ui/` 管理畫面、選單、焦點、轉場、墓碑與確認操作。主畫面狀態卡可選取，與 Status 選單進入同一詳細頁；未選中狀態卡時短按進主選單。
- `src/games/RpsGame` 管理猜拳的三回合、出拳、非阻塞倒數、比分與可領取一次的獎勵結果。Application 提供 ESP32 亂數；UiController 轉換輸入與繪製手勢，第三回合揭曉後回傳 FinishGame，由 Application 消耗結果、檢查健康／清醒條件並增加心情與 EXP；PetData 管理門檻／餘額／滿等，UI 在總結果之後播放兩秒升級動畫，沿用既有保存與 BLE 通知。
- 角色採使用者的 64×44 蛋／幼鳥／成鳥素材；`PetIcons` 加上需求／病情圖示與清潔度四階髒污。排版來源見 [OLED 手冊](ui-layout-guide.md)，素材轉換入口見根 README。
- `src/storage/Save` 逐欄位編碼、CRC32、序號及 NVS A／B 槽；寵物與墓碑分開保存。Memorials 管理 32 筆＋1 個死亡溢位槽，以 petId 去重。保存墓碑成功後才能領養。
- `src/ble/BleLink` 封裝 NimBLE，callback 收取分片與連線／訂閱事件，主迴圈組合 JSON、處理唯讀查詢及通知。BLE 不寫 NVS 或改養成規則。固定 device_id 取自 eFuse 工廠 MAC，與寵物存檔 ID 分開。
- `esp32_pet_app/` 是獨立 Flutter 建置；前端重組、驗證與顯示裝置快照。正式欄位與相容性要求只看 [PROTOCOL](../esp32_pet_app/PROTOCOL.md)。

## 擴充約束

主迴圈維持非阻塞更新。小遊戲應產生一次性結果交回領域／應用層。Deep Sleep 需協調保存、BLE 結束連線、外設停止、相對計時與喚醒重建；預留列舉與快照欄位不代表完整省電模組。

新增雲端層須分開 BLE payload、Web domain、API 與資料庫模型；純 Web 的帳號與偏好不放進韌體。規劃見 [Web 計畫](web-phase-plan.md)。
