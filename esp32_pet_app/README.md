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

Android 手機可用 Chrome 開啟 HTTPS 測試網址，從網頁按「搜尋裝置」，在系統選擇視窗選擇 ESP32-PET，再點網頁中的裝置列連線。允許瀏覽器使用藍牙；ESP32 保持供電。手機直接透過 BLE 連 ESP32，提供網頁的電腦不需要藍牙。電腦的 localhost 網址不可直接給手機使用。

臨時測試可先執行 `flutter build web --release`，只對 `build/web` 啟動本機靜態 HTTP 服務，再用 Cloudflare Quick Tunnel 轉成 HTTPS。測試時保留網頁服務與 tunnel 程序；電腦關機或 tunnel 停止後網址失效，重新啟動會得到新網址。此方式不加入雲端資料庫，且不代表 BLE 實機驗收已通過。

FlutterBluePlus 固定為 `2.3.13`，`pubspec.lock` 已產生並納入版控。使用 `flutter analyze` 及 `flutter build web --release` 檢查；最近結果見主進度連結。Windows 原生建置另需 Visual Studio 的 **Desktop development with C++**；Android 需 Android SDK；iOS 建置需 macOS 與 Xcode。套件宣告平台支援不代表本 App 已通過該平台驗證。

## 文件入口

- [整體進度與下一步](../docs/project-plan.md#progress)：BLE-S2、FW-CAP、APP-NATIVE 與 W0～W8。
- [共用協定](PROTOCOL.md)：UUID、JSON、裝置 ID、分片與睡眠連線約定。
- [Web 需求](../docs/web-phase-plan.md)：網站功能與分工。
- [程式定位](docs/implementation-notes.md)。

本文件只維護啟動與建置方法，容量、完成狀態及實機結果不重複記錄。
