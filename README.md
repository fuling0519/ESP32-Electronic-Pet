# ESP32 Electronic Pet／神秘蛋電子寵物

這是以 ESP32 NodeMCU-32S 製作的離線電子寵物。完整需求、已完成項目與驗收條件見 [專題企劃與進度追蹤](docs/project-plan.md)。此 README 是新開發者與 Agent 的接手入口；目前進度是 **A3 寵物核心進行中**，不是已完成的遊戲。

## 快速接手

1. 先讀本 README，再讀 [企劃](docs/project-plan.md) 第 2、3、4、6、8、10、11 節；[架構筆記](docs/architecture.md) 可補充模組邊界，但其中部分 UI 描述較舊，需與目前程式核對。
2. 先看 `git status --short`；工作目錄可能有尚未提交的 UI、顯示及字圖修改，勿直接重置或覆蓋。
3. 打開 [專案設定](platformio.ini)，確認本機 PlatformIO 已安裝 ESP32 平台與 U8g2，再編譯。
4. 需要實機驗證的項目請列出操作步驟、預期結果及需人類回報的現象。Agent 負責程式和可推導的細節，人類主要提供想法、硬體現況與實機測試結果。
5. 完成一項工作後，同步更新企劃核取項、本 README 的現況，以及實測／未實測註記；文件中的「已實作」不代表上板驗收通過。

## 開發環境與建置

- 語言：C++。
- 開發工具：Visual Studio Code + PlatformIO，或 PlatformIO CLI。
- 框架：Arduino for ESP32；PlatformIO 平台為 `espressif32`，環境 `esp32dev`，板型 `nodemcu-32s`。
- 顯示依賴：`olikraus/U8g2`，由 `platformio.ini` 的 `lib_deps` 安裝。沒有自建字型產生器或其他已宣告第三方依賴。
- 序列埠監控速度：115200。
- 尚未固定 `espressif32` 與 U8g2 的具體版本；首次建置需記錄 PlatformIO 實際解析版本，若 API 不一致再處理版本釘選。

```text
pio run -e esp32dev
pio run -e esp32dev -t upload
pio device monitor -b 115200
```

上板前核對 COM 埠、接線和供電；可從 VS Code 的 PlatformIO Build／Upload／Monitor 執行同樣工作。目前開機序列訊息以 `src/main.cpp` 為準，舊 README 所列的 `System Booting...` 範例已不適用。此 README 更新沒有執行韌體編譯或實機測試。

## 硬體接線與目前操作

- OLED：SH1106，128×64，I2C 地址 `0x3C`，SDA GPIO21，SCL GPIO22。
- 搖桿：VRX GPIO34、VRY GPIO35、SW GPIO26；方向移動，短按確認，長按返回。獨立按鈕目前無接線或程式。
- 蜂鳴器：GPIO27；現有程式依無源蜂鳴器設計。
- 腳位與校正參數集中在 [HardwareConfig.h](include/HardwareConfig.h)。硬體代號在不同資料寫為 HW-504／B103348，接手時核對實物。ESP32 輸入應維持 3.3V 規格。
- BH1750、溫濕度與 MPU6050 尚未加入。電池、充電、保護與整機續航也尚未實測。

## 真實功能現況

- 已有 OLED 驅動、輸入事件、蜂鳴器音序、開機／主畫面／選單／兩頁中文詳細狀態畫面。
- `PetData` 目前有飽食度、心情、清潔度、等級、經驗、生病與死亡旗標；目前只顯示預設資料，沒有養成時間更新。
- Feed／Play／Rest 主選單目的頁仍是 `Not implemented`；主畫面狀態卡與主選單 Status 都進入同一個兩頁詳細狀態。詳細狀態長按返回主畫面。
- `Save` 只有初始化占位，尚無保存／載入；每次開機重新建立預設寵物。
- 蛋、孵化、餵食、遊戲、一般睡眠、Deep Sleep、治療、死亡處理、墓碑群皆未實作。
- MVP 已決定包含疾病、治療、死亡、墓碑群與新蛋重養；**不加入體力**。墓碑需有名字與出生／死亡時間；沒有可信時鐘時顯示「時間未知」。具體首版規則見企劃第 4.3 節。

## 專案結構與修改邊界

- `src/main.cpp`：Application 持有唯一 `PetData`，串接輸入、UI、音效、保存。
- `src/hardware/`：Display 封裝 U8g2／I2C、Input 封裝 GPIO／搖桿事件、Sound 封裝 LEDC 蜂鳴器。
- `src/ui/`：畫面切換與圖示；`include/ui/`：狀態頁使用的 U8g2 精簡中文字型與先前的字圖素材。
- `src/pet/`：寵物資料與衍生需求狀態。
- `src/storage/`：保存邊界，目前無實際 NVS 讀寫。
- `include/HardwareConfig.h`：集中腳位及硬體參數。
- `docs/project-plan.md`：需求、決策、進度與驗收；`docs/architecture.md`：早期架構說明。

UI 不應直接操作硬體腳位。主迴圈目前以非阻塞方式更新輸入與音效；增加養成計時、保存與睡眠時維持這個方向。修改現有程式前先讀實際檔案，避免依據已過時的文件覆蓋工作目錄變更。

## 下一位開發者／Agent 的注意事項

- A3 下一步是完成狀態更新、UI 數值同步、疾病／治療／死亡規則、寵物身分與年齡；測試 0／100 與門檻邊界。體力不在 MVP。
- 主選單需新增「遊玩紀錄」，其中「墳墓群」是必做項目，逐局遊戲歷史可後做。墓碑需持久化、同一寵物不可重複入墓；新蛋需新 ID。首版 32 筆上限與滿額處理見企劃。
- 出生／死亡日曆時間需可信時鐘。首版可透過裝置介面手動設定；斷電後若時鐘無法持續，不能推算不存在的離線時間或編造日期。
- Deep Sleep 使用 SW GPIO26 與 Timer 喚醒；OLED／蜂鳴器需停止，醒來重新初始化硬體。RTC Memory 只供睡眠恢復，Preferences／NVS 才供斷電保存。詳見企劃第 6 節。
- 程式的 `Input::init()` 目前會在開機採樣搖桿中心；睡醒時搖桿偏移會影響校正。休眠實作需處理按鍵仍按著、重複觸發與中心校正。
- 改存檔格式時加版本與遷移。斷電續玩、墓碑、孵化結果與睡眠結算都需實機驗證；不要只以編譯成功勾選驗收。
- 如果文件與程式不同，以程式確認當前行為，再把產品需求與差異記回企劃。未提交的改動屬現有工作，不要自行清除。

## 階段

A1 硬體 → A2 UI → A3 寵物核心（目前）→ A4 蛋／孵化 → A5 養成／疾病／遊戲 → A6 NVS 保存 → A6-S Deep Sleep → A7 離線整合 → B 環境互動 → 第二階段手機與連線。一般睡眠在 A5，BH1750 明暗睡眠在 B1。進度與驗收以企劃第 10 節為準。

不要提交 `.pio/`、本機設定、輸出檔或憑證；既有工作目錄變動先辨識來源再處理。

## Local Secrets

No Wi-Fi credentials, API keys, tokens, or other secrets are included. If a future feature needs them:

1. Copy `include/secrets.example.h` to `include/secrets.h`.
2. Put local-only values in `include/secrets.h`.
3. Never commit `include/secrets.h`.

## License

A license has not been selected yet. This repository must not be published as an officially licensed open-source project until the owner chooses one and replaces `LICENSE` with its full text.

Common choices:

- **MIT**: simple and permissive; allows reuse with attribution and provides limited warranty protection.
- **Apache-2.0**: permissive and includes an explicit patent grant; longer and more formal than MIT.
- **GPL-3.0**: requires distributed derivative works to remain under GPL-3.0; suitable when preserving software freedom is a priority.

The project owner should choose based on the intended contribution and distribution model before the first public release.
