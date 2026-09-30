# ESP32 Pet Flutter App

本 App 是 ESP32 電子寵物的狀態檢視介面。ESP32 離線獨立運作，並是寵物狀態與保存的唯一權威來源。BLE 共用規格以 [PROTOCOL.md](PROTOCOL.md) 為準。

程式以 BLE DTO → adapter → Web domain model 隔離硬體協定與網站資料結構；domain model 與欄位映射見 [Web 資料結構隔離規格](../docs/web-data-structure.md)。Web UI 不直接解析韌體欄位，雲端 API／DB 也不直接使用 BLE JSON。

## 專案位置與建置

App 放在韌體倉庫的 `esp32_pet_app/`；PlatformIO 的 `src/`、`include/`、`lib/` 結構不變。Flutter 與 C++ 分開編譯。

第一個驗證目標選 Chrome Web；Web 平台骨架已建立。Chrome Web Bluetooth 需從 localhost 或 HTTPS 安全來源啟動：

```powershell
cd esp32_pet_app
flutter pub get
flutter run -d chrome
```

FlutterBluePlus 固定為 `2.3.13`，`pubspec.lock` 已產生並納入版控。`flutter analyze` 與 Chrome Web Release Build 已通過；BLE 實機驗收仍待完成。Windows 原生建置另需 Visual Studio 的 **Desktop development with C++**；Android 需 Android SDK；iOS 建置需 macOS 與 Xcode。套件宣告平台支援不代表本 App 已通過該平台驗證。

## 目前狀態

- 階段 2 已加入 ESP32 BLE Peripheral 與 Flutter 唯讀狀態流程，分片上限為每片 16 bytes JSON。
- App 顯示連線、搜尋、等待資料及斷線狀態；離線時保留並標示最後收到的快照。
- 遠端照顧、睡眠、A／B／C 按鍵及 reset 尚未啟用。
- 韌體正式環境已編譯通過，但 BLE 使 App 分割區 Flash 使用率達 92.8%（1,216,921／1,310,720 bytes，剩約 93.8 KB）；後續 BLE 功能擴充前需先處理容量風險。
- 自動檢查與 Web Release Build 已通過；BLE 掃描、連線、狀態通知、斷線重連及離線養成互不干擾仍待實機驗證，不可視為硬體驗收完成。
