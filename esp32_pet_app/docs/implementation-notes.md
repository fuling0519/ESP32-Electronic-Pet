# 實作內容說明

更新日期：2026-09-30

## 目前成果

Flutter 原型已更新為 Stage 2 唯讀 BLE 狀態 App，不再使用早期未實作的 A／B／C 與 reset 示範流程。

- `lib/main.dart`：搜尋並連線 `ESP32-PET`，查詢裝置資訊與寵物狀態，配對命令 ID，重組 BLE 分片，驗證完整快照，並在斷線後保留及標示最後資料。
- `PROTOCOL.md`：定義 BLE v1 UUID、4-byte 分片標頭、每片 16-byte JSON payload、1024-byte 完整訊息上限、命令結果、錯誤碼與狀態欄位。
- `src/ble/BleLink.*`：ESP32 BLE Peripheral；BLE callback 只收取分片，主迴圈處理唯讀命令與狀態通知，不經 BLE 修改養成狀態或 NVS。
- Web 平台骨架、FlutterBluePlus 2.3.13 與 `pubspec.lock` 已納入版控。

## 整合原則

- ESP32 是寵物狀態、離線養成與保存的唯一權威來源；App 不自行扣數值、判定成長、死亡或維護另一份存檔。
- v1 只接受 `get_device_info` 與 `get_status`。遠端照顧、睡眠、A／B／C 與 reset 均停用。
- App 未收到有效快照前顯示尚未取得；未知版本、缺少欄位、錯誤型別或未知列舉不得以預設值補成健康狀態。
- Flutter Web 需支援 Web Bluetooth 並從 HTTPS／localhost 啟動；iOS Safari 不支援時使用原生 iOS App。
- 清潔不建立排便事件。韌體以 `cleanliness` 四階段顯示 0～3 個髒污，Clean `+30` 後依新階段減少或移除；App 若呈現髒污，也由同一欄位衍生。

## 已完成檢查

- PlatformIO `esp32dev` 正式環境編譯通過。
- `flutter analyze` 通過，沒有問題。
- Chrome Web Release Build 通過。

加入完整 ESP32 BLE stack 後，正式韌體 Flash 為 1,216,921／1,310,720 bytes（92.8%），App 分割區只剩約 93.8 KB。後續功能應先評估 NimBLE；若調整分割區，須一併評估 OTA 雙槽取捨。

## 尚未完成

- BLE 搜尋、連線、通知分片、搖桿驅動狀態更新、斷線重連與離線養成互不干擾仍待真實 ESP32 驗收。
- Windows、Android 與 iOS 原生建置及實機驗收尚未完成；iOS 仍需 macOS／Xcode。
- 墓碑群、遠端照顧、睡眠、reset 等功能未納入 BLE v1。
- 不可逆操作未定義確認流程，因此不得提前開放。

## 下一步

依 [整合進度](../../INTEGRATION_PROGRESS.md) 完成 Stage 2 硬體驗收並記錄結果。在進入 Stage 3 前先處理 Flash 容量餘裕，再規劃遠端照顧與睡眠命令。
