# ESP32 Electronic Pet／神秘蛋電子寵物 2026/10/01

這是以 ESP32 NodeMCU-32S 製作的離線電子寵物。本文件負責環境、建置及操作入口。

**開發與實機驗收進度只看 [主企畫第 10 節](docs/project-plan.md#progress)**；所有文件的用途見 [文件導覽](docs/README.md)。容量實測數字由主進度連到有日期的建置紀錄，README 不另外複製。

## 快速接手

1. 先讀本 README，再讀 [企劃](docs/project-plan.md) 第 2、3、4、6、8、10、11 節；[架構筆記](docs/architecture.md) 說明模組邊界；實作細節仍需與程式核對。
2. 先看 `git status --short`；工作目錄可能有尚未提交的 UI、顯示及字圖修改，勿直接重置或覆蓋。
3. 打開 [專案設定](platformio.ini)，依照下方「新環境建置」安裝 PlatformIO 並編譯；依賴會由設定檔安裝。
4. 需要實機驗證的項目請列出操作步驟、預期結果及需人類回報的現象。Agent 負責程式和可推導的細節，人類主要提供想法、硬體現況與實機測試結果。
5. 完成工作後更新主企畫第 10 節的實作、驗證與下一步；建置或操作方法有變才改 README，介面有變才改共用協定。
6. 修改畫面、字型或操作文案前必讀 [OLED 排版引導手冊](docs/ui-layout-guide.md)：先規劃元素範圍、留白與動態內容，再修改繪圖；編譯成功不能代替視覺檢查。

## 小鳥 Web 展示入口

下載本專案後，直接以瀏覽器開啟 [web-preview/index.html](web-preview/index.html)，同資料夾的 app.js、style.css 與 assets/ 必須一起保留。此入口是本機模擬展示，不需要安裝套件；裝置連線按鈕目前提供說明。正式 Web 進度與驗收只見主企畫第 10 節 W2。

## 開發環境與建置

- 語言：C++。
- 開發工具：Visual Studio Code + PlatformIO，或 PlatformIO CLI。
- 框架：Arduino for ESP32；PlatformIO 平台為 `espressif32`，環境 `esp32dev`，板型 `nodemcu-32s`。
- 主要依賴：`olikraus/U8g2` 與 `bblanchon/ArduinoJson`，由 `platformio.ini` 的 `lib_deps` 安裝；BLE 使用 `h2zero/NimBLE-Arduino@2.5.0`。
- 序列埠監控速度：115200。
- 開發環境／dependency reproducibility：`platformio.ini` 固定 Espressif32 7.1.3、Arduino-ESP32 4.20017.260907+sha.dcc1105b、U8g2 2.36.18、ArduinoJson 6.21.5、NimBLE-Arduino 2.5.0；這些版本取自既有成功建置的環境。

## 新環境建置

1. 安裝 [Visual Studio Code](https://code.visualstudio.com/) 與 [PlatformIO IDE 擴充套件](https://platformio.org/install/ide?install=vscode)。若使用命令列，請確認 `pio` 可在終端機執行（PlatformIO Core 6.2.0 已用於本專案建置）。
2. 安裝 Git，然後 clone 專案：`git clone https://github.com/fuling0519/ESP32-Electronic-Pet.git`。
3. 在 VS Code 選擇「開啟資料夾」，開啟 clone 出來的 `ESP32-Electronic-Pet`。PlatformIO 會讀取 `platformio.ini`，在首次 build 時安裝指定的 ESP32 platform、Arduino framework 與 U8g2 library；首次下載需要網路。
4. 在專案根目錄執行下列指令。接好 NodeMCU-32S 後才執行 upload；如有多個序列埠，先選定正確的埠。

```text
pio run -e esp32dev
pio run -e esp32dev -t upload
pio device monitor -b 115200
```

只有要重新產生字圖素材時才需要 Python。建議安裝 Python 3.10 以上；`tools/generate_ui_font.py` 只用標準函式庫，`tools/generate_bird_sprite.py` 需要 Pillow 12.3.0。建議在專案根目錄先建立並啟用 `.venv`，再安裝 requirements：

```text
python -m venv .venv
```

Windows PowerShell 執行 `.venv\Scripts\Activate.ps1`；macOS／Linux 執行 `source .venv/bin/activate`。啟用後執行：

```text
python -m pip install -r requirements.txt
```

`.venv/` 已被 Git 忽略，不需提交。

ESP32 韌體資料使用 Preferences／NVS。MySQL、SQLite 都不是目前韌體的必要依賴；`docs/database-erd.md` 與 SQL schema 是未來規劃資料，不屬於此建置流程。

死亡動畫可用專用測試環境在約 30 秒內驗收；此環境開機後直接把測試寵物設為生病，正式 `esp32dev` 環境仍使用六小時生病、24 小時清醒疾病死亡門檻：

```text
pio run -e esp32dev_death_test
pio run -e esp32dev_death_test -t upload
```

墓碑、ABB 命名、重養及重開機保存請使用會保留測試存檔的快速生命週期環境。它使用獨立 NVS namespace：蛋約 15 秒孵化，孵化後自動生病並於約 30 秒後死亡；領養新蛋後可重複同一流程，重新開機也會載入該測試寵物與墓碑群：

```text
pio run -e esp32dev_memorial_test
pio run -e esp32dev_memorial_test -t upload
```

一般睡眠可用獨立的快速測試環境驗收。它每次開機建立心情 50 的成鳥，進入睡眠時才把飽食設為 0；睡眠時每 10 秒恢復 1 點心情，飽食為 0 持續 20 秒後生病，再過 30 秒會在睡夢中死亡。此環境使用獨立 NVS namespace，不影響正式寵物：

```text
pio run -e esp32dev_sleep_test
pio run -e esp32dev_sleep_test -t upload
```

手動深度睡眠測試使用 `esp32dev_deep_sleep_test`，詳細操作見 [深睡驗收步驟](docs/deep-sleep-test.md)。它使用獨立 NVS namespace `pet-deep`，首次建立成鳥、心情 50；資料會跨睡醒與重開機保留，不覆寫正式寵物。進入「休息」後，上下選擇一般睡眠或省電睡眠，短按執行、長按返回。測試版省電睡眠 60 秒 Timer 喚醒，心情每 10 秒 +1；正式版不啟用 Timer，只由 SW 喚醒，心情每 10 分鐘 +1；每次醒來最多結算 48 小時，超出的時間不補算。測試版也可按 SW 提早喚醒，醒來重新廣播 BLE。實作／驗收只見主進度。

```text
pio run -e esp32dev_deep_sleep_test
pio run -e esp32dev_deep_sleep_test -t upload
```

上板前核對 COM 埠、接線和供電；可從 VS Code 的 PlatformIO Build／Upload／Monitor 執行同樣工作。目前開機序列訊息以 `src/main.cpp` 為準，舊 README 所列的 `System Booting...` 範例已不適用。

## 硬體接線與目前操作

- OLED：SH1106，128×64，I2C 地址 `0x3C`，SDA GPIO21，SCL GPIO22。
- 搖桿：VRX GPIO34、VRY GPIO35、SW GPIO26；方向移動，短按確認，長按返回。獨立按鈕目前無接線或程式。
- 蜂鳴器：GPIO27；現有程式依無源蜂鳴器設計。
- 腳位與校正參數集中在 [HardwareConfig.h](include/HardwareConfig.h)。硬體代號在不同資料寫為 HW-504／B103348，接手時核對實物。ESP32 輸入應維持 3.3V 規格。
- BH1750、溫濕度與 MPU6050 尚未加入。電池、充電、保護與整機續航也尚未實測。

## 功能與進度入口

- 猜拳操作：主選單「陪玩」→短按開始，左右選招、短按出拳；揭曉後短按繼續，長按返回。固定三回合，整場贏／和／輸的心情獎勵為 +15／+10／+5；第三回合揭曉時結算，提早退出不發獎勵。蛋與生病寵物不可開始。規則、素材與畫面見 [猜拳企劃](docs/rps-game-plan.md)，上板檢查見 [猜拳驗收](docs/rps-game-test.md)。
- 治療操作：生病且清醒的幼鳥／成鳥在主選單選「治療」後直接倒藥，倒完才恢復健康，再播放恢復閃光；全程約 3.4 秒。期間汙點隱藏、按鍵鎖定，結束依當下清潔度重畫。座標與實際程式預覽見 [治療動畫](docs/ui-layout-guide.md#治療動畫)。
- [主企畫：進度、驗證證據與下一步](docs/project-plan.md#progress)。
- [產品規則](docs/project-plan.md#4-功能規劃與規則)：養成、疾病、孵化與睡眠。
- [BLE 共用協定](esp32_pet_app/PROTOCOL.md)：裝置 ID、命令與資料格式。
- [Web 需求及兩組分工](docs/web-phase-plan.md)。

## 專案結構與修改邊界

- `src/main.cpp`：Application 持有唯一 `PetData`，串接輸入、UI、音效、保存。
- `src/hardware/`：Display 封裝 U8g2／I2C、Input 封裝 GPIO／搖桿事件、Sound 封裝 LEDC 蜂鳴器。
- `src/ui/`：畫面切換與圖示；`include/ui/`：狀態頁使用的 U8g2 精簡中文字型與先前的字圖素材。
- `src/pet/`：寵物資料、衍生需求狀態及版本 1 寵物快照。
- `src/storage/`：固定欄位序列化、CRC32 與 Preferences／NVS A／B 槽保存。
- `src/ble/`：唯讀 BLE v1 的 GATT service、分片 JSON 命令與狀態通知。
- `esp32_pet_app/`：Flutter 狀態檢視 App；與 PlatformIO 韌體分開建置。
- `include/HardwareConfig.h`：集中腳位及硬體參數。
- `docs/project-plan.md`：需求、決策、進度與驗收；`docs/architecture.md`：早期架構說明。

素材分類、命名規則與舊檔對應見 [美術素材說明](assets/README.md)。

正式寵物的主畫面美術以 **64×44 單色像素**為標準畫布，放在 `(32, 6)`；主畫面中央安全區域約為 `x=20..108`、`y=0..54`。蛋、裂蛋、幼鳥與成鳥均使用上下兩格的 64×44 使用者手繪素材；目前幼鳥與成鳥分別取自 [baby/idle.png](assets/pets/bird/baby/idle.png) 和 [adult/idle.png](assets/pets/bird/adult/idle.png)，`tools/generate_bird_sprite.py` 逐像素產生韌體點陣資料及閉眼睡眠版本。成鳥第二格讓呆毛變化、身體略為下沉，畫面依序顯示 700／400 毫秒；幼鳥死亡時也會以其自身圖案溶解。畫面驗收範圍見主進度台帳。死亡動畫共用靈魂以 [soul.png](assets/shared/effects/soul.png) 為準：圖檔 30×56，從上到下是兩格 30×28 畫面，白色身體、X 眼、光環和兩種翅膀位置皆逐像素保留。兩格約每 300 毫秒交替，靈魂畫布從 `x=64` 開始（中心 `x≈79`)。

UI 不應直接操作硬體腳位。主迴圈目前以非阻塞方式更新輸入與音效；增加養成計時、保存與睡眠時維持這個方向。修改現有程式前先讀實際檔案，避免依據已過時的文件覆蓋工作目錄變更。

## 接手與保存注意事項

- 修改前檢查工作目錄差異；不要清除既有變更。
- 改存檔格式需定義版本與遷移。正常重開機測試不能代替寫入中斷或損毀測試。
- Deep Sleep 的需求、SW／Timer 喚醒、中心校正與計時邊界見主企畫第 6 節；BLE 斷線／重連約定見共用協定。
- 不要提交 `.pio/`、本機設定、輸出檔或憑證。

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

狀態頁共有三頁，可用右／下前進、左／上返回。第 3 頁顯示年齡，第一行「年齡　200 天」，第二行「15 時 30 分」，數值靠右對齊。包含已結算的睡眠時間，斷電時間不計入；不足一分鐘顯示 0 分，超過 999999 天顯示「>999999 天」。

