# App 實作定位

文件角色：程式閱讀指南；核對日期 2026-10-02。實作與實機驗收進度只在 [主台帳](../../docs/project-plan.md#progress)。

## 入口與資料流

- `lib/pet_snapshot.dart` 的 `PetSnapshot.parse` 驗證寵物完整快照與可選的等級／EXP 欄位組；`lib/main.dart` 使用裝置提供的數值顯示，UI 不自行推算需求、成長或升級門檻。`test/pet_snapshot_native.dart` 可使用 Dart 獨立重跑相容性與非法值測試。
- `_connect` 搜尋服務／特徵並訂閱 Event，之後發送裝置資訊與狀態查詢。
- `_request` 配對命令 ID；`_onFrame` 重組並解碼 JSON；`_handleMessage` 處理回覆、快照及公開裝置 ID。
- `_deviceId` 顯示於可選取文字；同一連線的 ID 格式及一致性有檢查。缺 ID 可本機唯讀；它不代表認領權限。
- 斷線保留最後資料並標記離線；新連線開始清除舊快照／ID，避免把上一台裝置當作這台。
- 韌體對應入口為根目錄 `src/ble/BleLink.*`，透過 NimBLE 提供共用 v1 協定。

## 修改邊界

訊息 schema、命令與相容性要求只在 [PROTOCOL.md](../PROTOCOL.md) 維護。UI 資料模型不應直接承擔未來 API／資料表責任；雲端各層需 adapter／mapper。

未知資料不能猜成健康或清醒。規格要求與所有錯誤路徑是否已驗證是兩件事，尚需補測的項目在主台帳 BLE-S1／BLE-S2。平台套件宣告支援不代表 App 已在該平台完成交付；平台進度看 APP-NATIVE。
