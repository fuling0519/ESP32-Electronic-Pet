# ESP32 Electronic Pet／神秘蛋電子寵物 2026/09/28

這是以 ESP32 NodeMCU-32S 製作的離線電子寵物。完整需求、已完成項目與驗收條件見 [專題企劃與進度追蹤](docs/project-plan.md)。此 README 是新開發者與 Agent 的接手入口；目前進度是 **A3 寵物核心進行中**，不是已完成的遊戲。

## 快速接手

1. 先讀本 README，再讀 [企劃](docs/project-plan.md) 第 2、3、4、6、8、10、11 節；[架構筆記](docs/architecture.md) 可補充模組邊界，但其中部分 UI 描述較舊，需與目前程式核對。
2. 先看 `git status --short`；工作目錄可能有尚未提交的 UI、顯示及字圖修改，勿直接重置或覆蓋。
3. 打開 [專案設定](platformio.ini)，確認本機 PlatformIO 已安裝 ESP32 平台與 U8g2，再編譯。
4. 需要實機驗證的項目請列出操作步驟、預期結果及需人類回報的現象。Agent 負責程式和可推導的細節，人類主要提供想法、硬體現況與實機測試結果。
5. 完成一項工作後，同步更新企劃核取項、本 README 的現況，以及實測／未實測註記；文件中的「已實作」不代表上板驗收通過。
6. 修改畫面、字型或操作文案前必讀 [OLED 排版引導手冊](docs/ui-layout-guide.md)：先規劃元素範圍、留白與動態內容，再修改繪圖；編譯成功不能代替視覺檢查。

## 開發環境與建置

- 語言：C++。
- 開發工具：Visual Studio Code + PlatformIO，或 PlatformIO CLI。
- 框架：Arduino for ESP32；PlatformIO 平台為 `espressif32`，環境 `esp32dev`，板型 `nodemcu-32s`。
- 顯示依賴：`olikraus/U8g2`，由 `platformio.ini` 的 `lib_deps` 安裝。沒有自建字型產生器或其他已宣告第三方依賴。
- 序列埠監控速度：115200。
- 尚未固定 `espressif32` 與 U8g2 的具體版本；2026/09/25 建置實際解析為 Espressif32 7.1.3、Arduino-ESP32 4.20017.260907、U8g2 2.36.18，若後續 API 不一致再處理版本釘選。

```text
pio run -e esp32dev
pio run -e esp32dev -t upload
pio device monitor -b 115200
```

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

上板前核對 COM 埠、接線和供電；可從 VS Code 的 PlatformIO Build／Upload／Monitor 執行同樣工作。目前開機序列訊息以 `src/main.cpp` 為準，舊 README 所列的 `System Booting...` 範例已不適用。

## 硬體接線與目前操作

- OLED：SH1106，128×64，I2C 地址 `0x3C`，SDA GPIO21，SCL GPIO22。
- 搖桿：VRX GPIO34、VRY GPIO35、SW GPIO26；方向移動，短按確認，長按返回。獨立按鈕目前無接線或程式。
- 蜂鳴器：GPIO27；現有程式依無源蜂鳴器設計。
- 腳位與校正參數集中在 [HardwareConfig.h](include/HardwareConfig.h)。硬體代號在不同資料寫為 HW-504／B103348，接手時核對實物。ESP32 輸入應維持 3.3V 規格。
- BH1750、溫濕度與 MPU6050 尚未加入。電池、充電、保護與整機續航也尚未實測。

## 真實功能現況

- 已有 OLED 驅動、輸入事件、蜂鳴器音序、開機／主畫面／選單／兩頁中文詳細狀態及死亡告別畫面。
- `PetData` 目前有寵物 ID、自動名字、小鳥種類、生命階段、飽食度、心情、清潔度、等級、經驗及健康狀態；`PetSnapshotV1` 會保存年齡、三項衰減餘數、疾病進度、可信時間與睡眠預留欄位，並以固定版本格式寫入 NVS。新蛋使用六字母 ABB 音節名字，例如 `Tamama`，建立後不因重開機或成長而重抽。
- 主選單 Feed／Clean／Treat／Play 有操作頁：進入後短按執行，長按返回；Play 目前是心情 +15 的基礎陪玩，Rest 仍是 `Not implemented`。主畫面狀態卡與 Status 都進入同一個兩頁詳細狀態，兩頁標題均顯示寵物名字。中文選單與照顧操作已由使用者上板驗收。
- 主選單第 3 頁的「紀錄」會進入墓碑群；方向鍵翻頁、短按進入刪除確認、長按返回。墓園採左側窄墓碑、右側名字與有效年齡的版面，當前剛死亡的寵物墓碑不可刪除。刪除確認以左右選擇 `Yes／No`，長按仍可直接返回。死亡告別頁把名字、RIP 與年齡放在墓碑內，短按後進入「接下來……？」，可選擇探望墓園或領養新蛋。
- 清醒時飽食或清潔連續為 0 六小時會生病；Treat 可治療，若需求持續為 0，六小時後可再次生病。生病後累計 24 小時清醒時間會死亡；死亡後停止照顧與時間推進，播放告別音效、山雀像素溶解及共用的白色 X 眼靈魂升空動畫，最後停在墓碑與年齡畫面。
- `Save` 已用 Preferences／NVS A／B 槽保存及載入當前寵物；墓碑群另有固定格式、序號、CRC32 與獨立 A／B 槽。死亡以 `petId` 去重，墓碑成功保存後才可領養新蛋。當前寵物互動後合併保存、自然進度每五分鐘保存，死亡立即保存。損毀槽不會覆蓋最後有效槽；兩槽皆無效時停止寫入以保留復原資料。
- 新存檔從蛋開始：4 分鐘顯示小裂痕、4 分 30 秒固定切換大裂痕，5 分鐘孵化成幼鳥，轉場會顯示「名字＋孵化了！」；裂痕只會單向增加，不循環回小裂痕。幼鳥再經 60 分鐘有效運行時間成為成鳥，生病時暫緩長大。蛋不會產生需求或疾病，幼鳥的飽食／清潔／心情分別每 60／90／120 秒下降 1。一般睡眠與 Deep Sleep 仍未實作。
- MVP 已決定包含疾病、治療、死亡、墓碑群與新蛋重養；**不加入體力**。墓碑顯示名字與累計有效年齡（活了多久），不要求出生／死亡日曆日期或可信現實時鐘。具體首版規則見企劃第 4.3 節。
- 第一版暫定只有小鳥一種寵物，每顆蛋都孵出小鳥，暫不做種類抽選。不同個體仍需各自的 ID、名字與墓碑；正式寵物美術由使用者繪製。

## 專案結構與修改邊界

- `src/main.cpp`：Application 持有唯一 `PetData`，串接輸入、UI、音效、保存。
- `src/hardware/`：Display 封裝 U8g2／I2C、Input 封裝 GPIO／搖桿事件、Sound 封裝 LEDC 蜂鳴器。
- `src/ui/`：畫面切換與圖示；`include/ui/`：狀態頁使用的 U8g2 精簡中文字型與先前的字圖素材。
- `src/pet/`：寵物資料、衍生需求狀態及版本 1 寵物快照。
- `src/storage/`：固定欄位序列化、CRC32 與 Preferences／NVS A／B 槽保存。
- `include/HardwareConfig.h`：集中腳位及硬體參數。
- `docs/project-plan.md`：需求、決策、進度與驗收；`docs/architecture.md`：早期架構說明。

正式寵物的主畫面美術以 **64×44 單色像素**為標準畫布，放在 `(32, 6)`；主畫面中央安全區域約為 `x=20..108`、`y=0..54`。蛋、裂蛋、幼鳥與成鳥均使用上下兩格的 64×44 使用者手繪素材，`tools/generate_bird_sprite.py` 逐像素產生韌體點陣資料。成鳥第二格讓呆毛變化、身體略為下沉，畫面依序顯示 700／400 毫秒；幼鳥死亡時也會以其自身圖案溶解。生命階段畫面與死亡動畫已由使用者上板驗收。死亡動畫共用靈魂以 [GHOST2.png](assets/ghost/GHOST2.png) 為準：圖檔 30×56，從上到下是兩格 30×28 畫面，白色身體、X 眼、光環和兩種翅膀位置皆逐像素保留。兩格約每 300 毫秒交替，靈魂畫布從 `x=64` 開始（中心 `x≈79`)。

UI 不應直接操作硬體腳位。主迴圈目前以非阻塞方式更新輸入與音效；增加養成計時、保存與睡眠時維持這個方向。修改現有程式前先讀實際檔案，避免依據已過時的文件覆蓋工作目錄變更。

## 下一位開發者／Agent 的注意事項

- A3 核心、當前寵物 NVS 保存及蛋／幼鳥／成鳥邏輯已接上；蛋／幼鳥／成鳥畫面、中文選單、照顧操作、死亡動畫與正常重開機續玩已由使用者上板驗收。幼鳥需求速度等更細的數值平衡不包含在此次回報中。快速驗收可燒錄 `esp32dev_growth_test`：10 秒小裂痕、12.5 秒大裂痕、15 秒孵化、再 2 分鐘成鳥，使用獨立 NVS namespace，成鳥後下次重啟會重新從蛋開始。體力不在 MVP。
- 主選單已新增「紀錄」與「墓碑群」瀏覽；死亡畫面可查看墓碑或領養新蛋，刪除墓碑需要再次確認。新蛋使用新 ID 與 ABB 隨機名字。首版保留 32 筆；第 33 次死亡會先寫入暫時溢位位置，刪除一筆舊墓碑後才能領養。
- 首版不要求設定現實日期時間，也不需購買外接 RTC。完全斷電後從 NVS 繼續遊玩，不補算未知的離線時間；Deep Sleep 仍需使用有效的相對計時基準結算睡眠時長。
- Deep Sleep 使用 SW GPIO26 與 Timer 喚醒；OLED／蜂鳴器需停止，醒來重新初始化硬體。RTC Memory 只供睡眠恢復，Preferences／NVS 才供斷電保存。詳見企劃第 6 節。
- 程式的 `Input::init()` 目前會在開機採樣搖桿中心；睡醒時搖桿偏移會影響校正。休眠實作需處理按鍵仍按著、重複觸發與中心校正。
- 改存檔格式時加版本與遷移。正常重開機續玩已上板驗收；意外斷電／寫入中斷、墓碑持久化及睡眠結算仍需實機驗證。不要只以編譯成功勾選驗收。
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
