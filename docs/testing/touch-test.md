# 電容觸摸互動：實作與實機驗收

2026-10-20接入觸摸，2026-10-21更新有限實機回報。使用者已在部分畫面成功觸發摸頭開心與戳戳驚嚇；完整驗收未完成，待硬體焊接完成後補驗。自動測試／建置與實機驗收分開記錄，最新結果以[主台帳](../project-plan.md#progress)為準；Agent未執行燒錄、清NVS、覆蓋裝置存檔或commit／push。

## 實作與可調參數

- `include/HardwareConfig.h`：Pins::Touch=32（經典ESP32 T9），目前採MODE 1暫定校正：MeasureCycles=40000（約5ms）、SleepCycles=4096、保留Core 2.0.17的0.5～2.7V量測電壓；低於820觸摸、高於830放開，等於門檻屬中間帶；Debug目前開啟。原1400／1450提案及預設0.5ms時的80／82門檻均不適用此量測設定。每30ms讀取一次，連續3筆確認；中間帶重置確認筆數。首版固定門檻，未加自動校正；完整機殼下的穩定性尚未驗收。
- `Hardware::TouchGesture`：純raw／millis狀態機。起點取連續低值的第一筆，短觸長度取第一筆放開高值，避免把三筆防抖時間算入。100～699ms放開發一次Poke，700ms未放開發一次PettingStarted，摸頭確認放開發一次PettingReleased；感測故障／長停頓發Cancelled，立即取消畫面。短暫訊號小於100ms略過。已有PettingStarted的手勢不再發Poke。
- 摸頭放開需連續3筆高值且距第一筆高值至少120ms；期間恢復低值沿用原事件與動畫。短觸只做三筆放開確認，以保留約100～150ms間隔的連戳。
- 啟動、取消、禁用頁面開始觸摸後，需穩定放開才重新接受。raw=0視為無效並鎖到放開；取樣／更新間隙超過150ms也取消，不虛構補樣本。這會保守丟棄遇到長阻塞的手勢，需在實機確認取樣穩定性。
- `Hardware::TouchInput`負責touchRead與取樣節奏，UI不讀GPIO。setup先做一次touchRead初始化，之後套用touchSetCycles，避免Core首次初始化覆寫量測參數，再啟動PetClock；runtime沒有新增delay或重設量測參數。Arduino驅動首次初始化本身可能短暫等待，同步touchRead／OLED／NVS仍可能影響實機延遲。30ms是軟體讀取節奏，硬體完整量測還包含RTC sleep，相鄰讀值不保證來自不同硬體週期；需在實機確認防抖與手勢延遲。
- Application在UI更新前取樣，更新後再次比對資格、UI情境修訂、petId、生命階段、搖桿輸入與UiAction。同輪即使離開再回Home，也不投遞舊事件。任何搖桿事件優先；取消後必須先放開。
- UI資格：健康、清醒、非傷心、Home的幼年／成年；餵食、清潔、治療、遊戲、成長、升級含待播、提示、Deep Sleep pending等流程不允許。睡眠、蛋、死亡／送別與其他頁面皆不接受。恢復心情／需求仍沿用原有遲滯。

## 動畫與音效

V3戳戳：同一shock反應幀右移1px維持160ms，回原點維持340ms，總長500ms。幼鳥／幼龍結束用shock第二幀復原，至下一次原待機換幀後接回正常動畫；成年直接接回原待機。新事件撞上反應／摸頭收尾時略過，不排隊、不重設起點。

V9摸頭：完整使用使用者happy圖集，幼鳥14幀／成鳥13幀為5 FPS，幼龍／成龍各8幀為7 FPS。身體與愛心共用使用者原幀，不額外疊加漂移。按millis算幀號，獨立於原待機FPS，保留原點(32,6)、兩側HUD、EXP、現有髒污。放開或開始反應滿5秒後，繼續目前動畫250ms再回待機；5秒後仍按住不重播，需先放開。

49個來源幀以17248 bytes的PROGMEM點陣存放於`src/ui/TouchSprite.h`。`tools/generate_touch_sprite.py`重建；`--check`核對49幀每一像素。原始PNG不修改，未更動BirdSprite／WyvernSprite。

Sound新增低優先`tryPlayPoke`（1047Hz 45ms→784Hz 55ms）、`tryPlayPetting`（659Hz 60ms→784Hz 80ms）。只有UI接受新反應時嘗試一次，忙碌略過且不補播。原高優先音序可接管；遵守全域靜音，靜音時仍非阻塞計時。實際聽感尚待驗收。

觸摸不呼叫照顧、EXP、保存或BLE通知；正常PetClock、既有保存與BLE更新仍照常執行。NVS／BLE格式及睡眠喚醒源均未修改。

## 在 VS Code 執行獨立原始讀值測試

不用另外建立專案，也不用替換`src/main.cpp`。`esp32dev_touch_read_test`沿用正式`esp32dev`的NodeMCU-32S、PlatformIO平台與Arduino ESP32 Core 2.0.17，只編譯`src/touch_read_test.cpp`，不執行電子雞Application或其存檔操作；沒有OLED／BLE／寵物動畫。程式直接呼叫`touchRead(32)`，每200ms輸出一次，不套用正式版的門檻或UI資格。

1. 用VS Code開啟專案根目錄，點左側PlatformIO圖示，展開`PROJECT TASKS`。
2. 展開`esp32dev_touch_read_test` → `General`，先點`Build`。
3. 要上板比較時，由使用者點同一環境的`Upload`。這會暫時替換板上執行的韌體；不用Erase Flash。
4. 點同一環境的`Monitor`，速率115200。若錯過開機訊息，按板上EN／Reset。
5. 應看到`Touch Sensor Test - esp32dev_touch_read_test`及`Arduino ESP32 Core: 2.0.17 | GPIO: 32 | Interval: 200 ms`，接著連續`Touch Value: ...`。保持相同供電、外殼與接線，記錄放開、持續觸摸各約10秒。
6. 測完關閉Monitor，再到`esp32dev` → `General` → `Upload`，恢復正式電子雞韌體。預設建置環境仍是`esp32dev`。

環境列表未更新時，在VS Code命令面板執行`Developer: Reload Window`。[PlatformIO Project Tasks操作說明](https://docs.platformio.org/en/stable/integration/ide/vscode.html#project-tasks)。

此測試用來區分Core差異與其他程式／硬體因素：使用者Arduino IDE測試快取為Core 3.3.11，正式PlatformIO為2.0.17，兩版觸摸驅動的預設量測時間與電壓設定不同，因此raw門檻不能直接共用。2026-10-21使用者回報獨立2.0.17測試也為80多，與正式版一致，支持Core預設量測參數造成主要量級差距；這不代表正式手勢已驗收。200ms是呼叫間隔，並不是底層量測窗口。

2026-10-21原始獨立環境建置成功（`.pio/touch-read-test-build.log`）；正式`esp32dev`也建置成功（`.pio/touch-read-production-build.log`），確認新增測試入口未造成重複setup／loop。之後使用者回報獨立版與正式版同為80多；此結果只比較原始讀值，不代表正式手勢、OLED或存檔驗收。

### 量測參數比較（2026-10-21增量）

使用者確認原始讀值後，獨立測試版新增串口`0`／`1`／`2`三種模式；重新Build／Upload `esp32dev_touch_read_test`後，在Monitor按數字並按Enter送出。開機預設0，Reset恢復0，不保存模式。每次切換會印出Mode、實際讀回的measure／sleep cycles與設定電壓，等下一個200ms輸出節點再讀值。

- `0`：原預設，measure=4096（約0.512ms），sleep=4096，電壓範圍0.5～2.7V。
- `1`：只延長量測，measure=40000（約5ms），sleep=4096，電壓仍為0.5～2.7V。
- `2`：measure=40000（約5ms），sleep=4096，縮窄電壓範圍至0.5～1.7V，接近Core 3.3.11的這兩項預設。舊SDK以2.7V上限減1.0V attenuation設定1.7V；未宣稱整個新舊驅動完全等效。

先以0記錄放開／觸摸各10秒，再分別以1、2重複；保持供電、電極、手勢與握持方式一致。點入VS Code Monitor終端機，輸入數字並按Enter；預設Monitor通常逐字送出，輸入的字元可能不回顯，程式接受單個數字並忽略換行。確認出現對應`Mode`訊息才開始記錄。回報每組未觸摸最小／最大與觸摸最小／最大；切換時不要拿上一模式的門檻判讀。

延長實際量測能增加計數解析度，但數字變大不保證雜訊與兩狀態重疊改善；縮窄電壓範圍也可能增加雜訊。選擇依據是多次觸摸、長時間放開及不同供電條件下的範圍間隔，而非是否回到1000多。官方[量測與雜訊說明](https://docs.espressif.com/projects/esp-idf/en/v4.4.7/esp32/api-reference/peripherals/touch_pad.html#optimization-of-measurements)。

設定在首次`touchRead`初始化之後套用，避開Core 2.0.17首次初始化覆寫參數的行為；沒有新增runtime delay、顯示倍率或自動基準。獨立版Build通過（`.pio/touch-read-modes-build.log`，Flash 276253、RAM 21976 bytes）。建立比較版當時尚未取得模式1／2實測、正式量測仍為預設與80／82；後續結果與暫定接入見下段。沒有由工具燒錄、清NVS、提交或推送。

### MODE 1 正式版暫定校正

使用者回報MODE 1摸住800～844、放開838～846；摸住大部分約800～816，但曾連續六筆837～844。MODE 2摸住1264～1506、放開1504～1523。兩組完整範圍皆有重疊；使用者表示「應該有稍微放開或移動過」，支持MODE 1高值可能來自動作變化，並未證實定點持續觸摸時完全不重疊。

因此正式版先採MODE 1量測＋820／830遲滯作為待驗收設定，保留原三筆確認、700ms摸頭與120ms摸頭釋放寬限。這是實作與校正候選，不是已驗收門檻；若定點摸住仍出現838以上連續高值，需再檢查電極貼合、接觸位置與供電，不能把該段直接刪成雜訊。原生測試改用目前量測尺度，並檢查初始化後才設定週期、runtime不重設週期。

後續完整補驗時，使用正式`esp32dev`，先完全放開至少1秒讓輸入解鎖；健康清醒Home應看到`allowed=1`。固定同一位置自然觸摸10秒／放開10秒，重複三輪並記錄raw範圍，再測約200～400ms短觸與1～3秒摸頭各10次。確認700ms後開始摸頭、放開不再戳戳、持續按住不重播；滑動或短暫放開與完全不動分開記錄。其他頁面仍不接受觸摸。使用者目前僅回報部分畫面成功觸發，尚未完成這組詳細驗收。

自動驗證：`tools/test_rps.ps1`完整原生回歸（含校正尺度的手勢、初始化順序及四角色實際UI）通過，紀錄`.pio/touch-mode1-native-tests.log`；保留既有host字串格式截斷警告。PlatformIO正式`esp32dev`與`esp32dev_deep_sleep_test`皆成功（`.pio/touch-mode1-builds.log`），Flash分別762625／762941 bytes、RAM皆55108 bytes。`git diff --check`通過；文件檢查仍有先前已記錄的19個失效路徑，未在本輪改動。未由工具燒錄、清NVS、commit／push；編譯通過不等於Deep Sleep或觸摸實機驗收。

## 自動驗證入口

在專案根目錄執行；以已安裝的PlatformIO及有Pillow的Python環境執行對應工具：

```powershell
python tools/generate_touch_sprite.py --check
./tools/test_rps.ps1
python tools/verify_touch_frames.py
pio run -e esp32dev
```

完整原生回歸已包含`tools/test_touch.ps1`；也可在U8g2 host objects已建好時單獨執行它。測試包括門檻等號、中間帶／三筆防抖、99／100／699／700ms、690ms開始放開、長按10秒、摸頭斷訊、連戳、取消到放開、故障值、缺樣、millis繞回及實際取樣器節奏。

真實UiController／PetIcons／U8g2測試包括四角色幀率邊界、五秒上限／250ms收尾、dirty flag、重複事件、HUD與髒污保留、養成時間、心情／EXP／需求不變，以及選單／狀態、照顧、遊戲、睡眠、傷心／生病、升級、成長、死亡／送別的拒絕／取消。

`verify_touch_frames.py`把真實C++ framebuffer與來源PNG及已確認V9逐幀比對，輸出[驗證JSON](../previews/touch-implemented/validation.json)、四組[正式UI GIF](../previews/touch-implemented)與PNG。這些不是實機錄影。

## 實機回報（2026-10-21：部分確認，完整驗收未完成）

- 使用者已在部分畫面實機成功觸發「摸頭開心」及「戳戳驚嚇」，記為兩種互動已觀察到成功觸發；未回報完整角色覆蓋或各項詳細測試結果。
- 硬體焊接尚未完成，感測讀值仍會變動，目前無法做詳細穩定性驗收。MODE 1與820／830保持暫定設定，不標記為最終校正或整體觸摸驗收通過。
- 待焊接完成、硬體配置固定後，再記錄觸摸／放開範圍並確認門檻，補驗下方完整流程。四角色、連戳／長按／放開、誤觸／漏判、頁面及特殊流程、音效、睡眠、存檔與BLE回歸皆未因這次有限回報視為通過。

本輪僅更新驗收紀錄，未改韌體、參數或素材，也未新增自動測試／編譯。先前自動驗證結果保留，不作為完整實機驗收的替代。

## 完整實機驗收流程（尚未完成，待硬體焊接後補驗）

1. 記錄本次程式版本／工作目錄、環境名、供電、外殼與銅箔配置，核對GPIO32未另接其他裝置。取得另外的燒錄指示後才上板；此流程不包含Erase Flash或清NVS。
2. 正式`esp32dev`讀取原存檔，不為了驗收重養或改正式寵物。四角色可使用既有獨立測試環境：成鳥`esp32dev_sad_test`、幼鳥`esp32dev_sad_baby_test`、成龍`esp32dev_wyvern_test`、幼龍`esp32dev_wyvern_baby_test`。它們使用各自測試NVS，會在冷啟動重建測試寵物；不要把正式存檔移入測試namespace。所有namespace仍共享NVS容量，容量不足時停止，不以清存檔當前提。
3. 固定角色版本啟動後，在115200串口送`0`，恢復健康並取消原有8秒後自動傷心，以便持續驗收。每次冷啟動重新送0。送1／2／3／4分別檢查低心情、低飽食、低清潔與生病時拒絕觸摸，再送0恢復。這些命令僅存在於既有測試版，正式版不提供。
4. A／B兩位使用者各先放開至少5分鐘，應無誤觸；各測20次約200～400ms短觸、10次1～3秒持續摸頭。記錄漏判／誤判與放開反應延遲，門檻附近滑動／輕觸不得連播。若需raw紀錄，可暫將Touch::Debug開啟，最多每秒一行；結束後關閉。測量取樣間隔需另外做限量儀表，不用每輪Serial干擾結果。
5. 四角色逐一確認：短觸只有一次500ms戳戳；長觸約700ms開始、放開不再戳戳；小鳥5 FPS、小飛龍7 FPS，表情／愛心／動作與V9一致。幼寵戳戳用第二幀復原；角、翅、HUD、EXP不裁切，稍髒時污漬仍在。
6. 長按10秒只播一次摸頭音，約5秒後接250ms收尾，繼續按住不重啟；先放開才可再摸。快速連戳不重設當前動畫，短暫滑開約100ms不重新啟動摸頭。
7. 動畫中移動搖桿或切到選單／狀態頁，立即取消。從選單、遊戲或照顧動畫中開始按住，持續到回Home仍不得突然觸發；先放開後才能重試。餵食、清潔、治療、成長、升級、死亡／送別保持原流程。
8. 一般睡眠／Deep Sleep都不被觸摸喚醒；SW仍可喚醒。醒來若手指持續觸摸，先放開才可互動。靜音／有聲各驗證一次，治療／升級／疾病提醒音不被觸摸音截斷。
9. 回歸原搖桿、兩個遊戲、存檔重啟、一般／Deep Sleep、BLE讀取與通知。觸摸本身不增加心情或EXP；比對數值時扣除自然養成時間與照顧／遊戲獎勵，不能把自然扣分誤認為觸摸扣分。

請回報各項通過／失敗、角色與環境、raw範圍及可重現操作。編譯、自動測試或GIF一致不代表真實觸摸、實物OLED、蜂鳴器、存檔或BLE驗收通過。
