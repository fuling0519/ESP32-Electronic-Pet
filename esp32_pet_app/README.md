# ESP32 Pet Flutter App

本 App 是 ESP32 電子寵物的狀態檢視介面。ESP32 離線獨立運作，並是寵物狀態與保存的唯一權威來源。BLE 共用規格以 [PROTOCOL.md](PROTOCOL.md) 為準。

## 專案位置與建置

App 放在韌體倉庫的 `esp32_pet_app/`；PlatformIO 的 `src/`、`include/`、`lib/` 結構不變。Flutter 與 C++ 分開編譯。

第一個驗證目標選 Chrome Web；Web 平台骨架已建立。Chrome Web Bluetooth 需從 localhost 或 HTTPS 安全來源啟動：

```powershell
cd esp32_pet_app
flutter pub get
flutter run -d chrome
```

FlutterBluePlus 固定為 `2.3.13`，`pubspec.lock` 已產生並應納入版控。此階段尚未完成 Flutter 靜態分析、平台建置或 BLE 實機驗收。Windows 原生建置另需 Visual Studio 的 **Desktop development with C++**；Android 需 Android SDK；iOS 建置需 macOS 與 Xcode。套件宣告平台支援不代表本 App 已通過該平台驗證。

## 目前狀態

- 階段 2 已加入 ESP32 BLE Peripheral 與 Flutter 唯讀狀態流程，分片上限為每片 16 bytes JSON。
- App 顯示連線、搜尋、等待資料及斷線狀態；離線時保留並標示最後收到的快照。
- 遠端照顧、睡眠、A／B／C 按鍵及 reset 尚未啟用。
- 編譯、自動檢查與實機連線仍待驗證；不可視為實機驗收完成。