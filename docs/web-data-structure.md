# Web 端資料結構與硬體隔離規格

- 版本：1.0
- 日期：2026-09-30
- 狀態：Web domain model 與 BLE adapter 已建立；登入、API、雲端 DB 尚未實作。

## 1. 邊界原則

韌體自己的資料結構留在韌體：`PetData`、`PetSnapshotV1`、NVS 編碼和 BLE v1 JSON 都屬於硬體端。Web 自己定義 `WebDevice`、`WebPetSnapshot`、`WebPetKey`、`WebPetEvent`、`WebDeviceOwnership` 等資料；Web model 不直接 include、複製或依賴韌體 C++ struct。

資料只透過明確的 adapter 和版本化介面跨界：

```text
韌體 PetData / PetSnapshot
        ↓ 韌體 BLE encoder
BLE v1 wire JSON (硬體協定 DTO)
        ↓ BleSnapshotAdapter
Web domain model (網站內部語意)
        ↓ Web API serializer / database repository
Web REST DTO / PostgreSQL schema
```

畫面和網站服務只依賴 `lib/domain/models/`；BLE 欄位解析只依賴 `lib/data/ble/`。未來改用 Wi-Fi、原生 App、本地模擬裝置或不同後端時，替換 transport／repository adapter，不改畫面 domain model。反過來，增加 Web 收藏標籤、顯示偏好、裝置暱稱或帳號資料，不修改 BLE v1 或韌體存檔格式。

## 2. 目前已建立的 Web model

程式位置：

- `esp32_pet_app/lib/domain/models/web_pet.dart`
- `esp32_pet_app/lib/data/ble/ble_snapshot_adapter.dart`
- 使用 domain model 的首頁：`esp32_pet_app/lib/main.dart`

Web domain types：

- `WebDeviceId`：驗證後的公開裝置 ID 型別。
- `WebDevice`：Web 裝置檔案，含暱稱、韌體／協定版本及最後同步時間。
- `WebDeviceOwnership`：帳號與裝置的 Web 業務關係。
- `WebPetKey`：裝置 ID、存檔世代及個體 ID 組成的雲端個體鍵。舊韌體未提供裝置 ID／世代時相應欄位暫為 null，狀態仍可顯示，但不可用於雲端綁定或保證唯一的長期寵物歷史。
- `WebPetSnapshot`：型別化的 Web 寵物最新狀態，需求欄位以百分比命名、年齡採 `Duration`、生命階段／健康／睡眠採 enum；保留收到時間與可空 revision。
- `WebPetEvent`：Web 事件識別、個體鍵、有效年齡、可能缺省的可信發生時間及收到時間。

這些型別是 Web domain 結構，不是對韌體記憶體 layout 或 C++ struct 的鏡像。`WebPetSnapshot.toCloudJson()` 提供獨立的 Web API 表示範例；實際 server API 上線時可另建嚴格 DTO，不要求資料庫照 Flutter 類別名稱或欄位排序建表。

## 3. BLE v1 對 Web domain 的單向映射

BLE v1 wire 欄位保持原名及編碼；只在 `BleSnapshotAdapter` 轉換：

| BLE v1 欄位 | Web domain 欄位 | 轉換規則 |
|---|---|---|
| root `device_id` | `WebPetKey.deviceId` | 新韌體需驗證；舊韌體缺欄位時可為 null 並只供本地顯示，格式錯誤的非空值拒收 |
| `pet.id` | `WebPetKey.petId` | 十進位字串原樣保存，不能轉成 JS number |
| `pet.save_generation`（未來可選） | `WebPetKey.saveGeneration` | 目前韌體未傳；缺少時不宣稱長期唯一 |
| `pet.name` | `WebPetSnapshot.name` | 非空 UTF-8 名稱 |
| `pet.life_stage` | `PetLifeStage` | `egg/baby/adult` → Web enum |
| `pet.satiety` | `satietyPercent` | 0–100 整數 |
| `pet.mood` | `moodPercent` | 0–100 整數 |
| `pet.cleanliness` | `cleanlinessPercent` | 0–100 整數 |
| `pet.age_seconds` | `Duration age` | 十進位字串解析為秒；拒絕超出 Dart 整數範圍值 |
| `pet.health` | `PetHealthState` | `healthy/sick/dead` → Web enum |
| `pet.is_dead` | `isDead` | 必須與 health 狀態一致 |
| `pet.sleep` | `PetSleepState` | `awake/normal` → `awake/sleeping`；未知／不支援值拒收 |
| Web 收到完整狀態時間 | `receivedAt` | 瀏覽器接收時間；不是寵物事件發生時間 |

BLE DTO 驗證失敗時整筆 snapshot 拒收，不用預設值遮掩韌體或協定不相容。Wire 欄位改名時應新增協定版本或相容解析，不應要求 Web 畫面更改。

## 4. Web API / 持久化模型

雲端只接收 Web domain serializer／REST DTO；不要直接把 BLE 原始 JSON 作為資料庫 schema。建議 PostgreSQL 表：

- `users`：登入供應商 subject、狀態、建立／刪除時間。登入密碼及供應商 token 不進裝置資料表。
- `devices`：`device_id`、Web 暱稱、首次註冊、韌體／BLE 協定版本、最後收到快照時間。
- `device_ownerships`：帳號 ID、裝置 ID、綁定及解除時間；由後端授權，任何裝置輸入都不能指定擁有者。
- `pairing_sessions`：短效配對流程、已 hash 的一次性碼／驗證材料、到期、嘗試次數和使用狀態。
- `pets`：Web pet key、展示名稱、種類、生命階段及最新完整 Web snapshot。
- `pet_events`：穩定 event ID、Web pet key、事件種類、`age_seconds`、nullable `occurred_at`、`received_at`、payload version、來源。
- `commands`：Web command ID、帳號、裝置、目標 Web pet key、種類、效期、回覆和稽核資料。
- `user_preferences`：網站主題、通知偏好、畫面顯示名稱等 Web-only 欄位，完全不下傳韌體。

pet 唯一鍵建議 `(device_id, save_generation, pet_id)`。在 `save_generation` 尚未由韌體協定提供前，只存目前快照或標記 legacy key；不要用 `petId` 單獨合併不同裝置，也不要保證跨 NVS 重建後的歷史不碰撞。

後端 API 可用 `camelCase` 的 Web 欄位（例如 `deviceId`, `lifeStage`, `ageSeconds`）；DB 可使用 `snake_case`，由 server repository 映射。BLE 的 `life_stage`、`age_seconds` 僅留在 BLE adapter。Web-only 欄位可以獨立新增 migration；不因 schema 欄位增加就改動裝置協定。

## 5. 隔離規則與相容策略

1. 韌體 C++ 結構、NVS 存檔版本、BLE wire 協定和 Web domain／REST／DB schema 使用各自的 version；不要求數字相等。
2. BLE wire 相容性透過 BLE `protocol_version` 管理。非破壞性新增欄位可先設 optional；更名、重解釋、移除欄位或命令語意改變需升 protocol version，並在 adapter 中保留相容分支或拒絕不支援版本。
3. Web REST API 自行版本化，例如 `/v1`；Flutter Web 更新和後端部署可採先向後相容再切換的 expand/migrate/contract 流程。
4. DB schema 只由 server migrations 管理；不要由 ESP32 知道 SQL 表名、資料庫欄位或登入帳號欄位。
5. Firmware 不接收網站 display preferences、收藏、登入資料或 DB revision。需要實際改變養成的操作才經 BLE command contract 傳輸。
6. Web 僅將裝置回報的寵物狀態視為權威快照；網站備註、標籤、帳號設定及使用者操作稽核屬 Web 自有資料。
7. 一次韌體變更不得偷偷依賴 Web DB；一次 Web feature 不得假設韌體 NVS 會同步永久保存它。
8. 任何需要韌體改動的欄位，先寫出 BLE DTO 與 adapter 遷移，再分別發布 Web 和韌體；任一端版本不支援時以明確狀態降級，不猜欄位。

## 6. 功能所有權界線

| 資料／規則 | 權威來源 | 複製／衍生位置 | 修改是否要動硬體 |
|---|---|---|---|
| 三項需求、健康、階段、有效年齡 | ESP32 PetData | WebPetSnapshot、雲端 latest snapshot | 遊戲規則變更要改硬體；呈現方式不必 |
| 裝置晶片身分 | ESP32 eFuse `device_id` | Web Device 記錄 | eFuse 來源變更需改硬體 adapter |
| 登入、owner、裝置暱稱 | Web 後端／帳號 | Web domain／DB | 否 |
| Web 顯示偏好、排序、標籤 | Web | Web user preferences | 否 |
| 事件時間線 | ESP32 或橋接 App 的真實事件 | WebPetEvent／DB | 若要補齊裝置本地離線事件，需增加裝置事件佇列與協定 |
| Web 頁面快取、載入狀態 | Web | 前端狀態 | 否 |
| 餵食等改變養成的指令 | ESP32 執行規則 | Web Command／結果稽核 | 新增命令需改硬體與 BLE contract |

## 7. 新增欄位流程

### 只改 Web

新增網站顯示偏好或裝置暱稱時，更新 domain model、Web API／DB migration 與 UI；不變動 BLE schema 或 NVS。

### 只改硬體內部

重構 `PetData`、畫面或 NVS 編碼時，只要 BLE v1 snapshot 語意保持一致，Web 不需修改。若原本內部資料不再能提供既有 wire 欄位，韌體需維持相容輸出或發布新 protocol version。

### 增加跨界欄位

先寫資料用途、權威來源、nullable／預設策略、範圍、version 和 fallback，再修改 BLE DTO、adapter、domain model、REST DTO 與 DB migration。確認舊韌體 + 新網站、新韌體 + 舊網站的行為後才移除舊格式。

## 8. 目前不涵蓋的部分

本次先建立 Flutter Web domain models 與 BLE-to-domain adapter，並以文件定義雲端資料邊界。帳號登入、配對驗證、後端 repository、資料庫 migration、事件上傳、BLE 命令及 `saveGeneration` 韌體欄位仍各自是後續階段；它們未完成前不把雲端同步或長期歷史描述為已具備。
