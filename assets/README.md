# 美術素材分類與命名

寵物素材依「種類 → 成長階段 → 表情／動作」分類；共用道具、效果、蛋、UI 與遊戲素材各自集中。新增素材或尋找圖片時以本文件為入口。

## 資料夾

- `pets/bird/baby/`、`pets/bird/adult/`：小鳥幼寵、成寵的正式素材，目前各有 `idle.png`、`eating.png`、`sad.png`。
- `pets/wyvern/baby/`、`pets/wyvern/adult/`：小飛龍幼寵、成寵正式素材，各有 `idle.png`、`eating.png`、`sad.png`、`sleep.png`。
- `shared/items/`：共用道具，目前有 `food_bowl.png` 與治療動畫圖集 `potion.png`。
- `shared/effects/`：共用效果，目前有 `soul.png`、`dirt.png`、`cleaning.png`、`sad_lines.png`。
- `shared/egg/`：目前共用的蛋，`idle.png` 為一般狀態，`hatching.png` 為破殼狀態。未來若種類有專屬蛋，再放到 `pets/<species>/egg/`。
- `ui/`：介面圖示的預留位置，目前尚無圖檔。
- `games/rps/`：猜拳正式圖集 `gestures.png`，三種手勢統一讀取這張已修改剪刀的圖；舊圖保留於 `archive/games/rps/gestures_v1.png`。
- `previews/`：概念或展示圖片；`previews/pets/bird/idle_concept.png` 為原 `tit_idle_preview.png`，不作為韌體正式素材。
- `archive/`：保留的舊版素材，不作為目前韌體生成來源。

## 命名規則

1. 資料夾、檔名使用小寫英文；多字用底線，例如 `food_bowl.png`。
2. 寵物固定使用 `pets/<species>/<stage>/<state>.png`。種類目前為 `bird`、`wyvern`；階段為 `baby`、`adult`。
3. 表情使用 `idle`、`happy`、`sad`、`sick`；動作使用 `eating`、`sleep`。只放實際完成的圖片，不建立假的圖片佔位。
4. 一張包含多幀的動畫圖集仍叫 `idle.png`；若未來採獨立幀檔案，命名 `idle_00.png`、`idle_01.png`。不同儲存方式需同步調整轉換工具。
5. 正式素材使用固定檔名。被替換的舊版放到 `archive/`，保留相同分類並加版本尾碼，例如 `archive/pets/bird/adult/idle_v1.png`；不要用 `idle2.png` 混淆動畫幀與版本。
6. 共用素材只保存一份。若以後出現專屬靈魂，放到 `pets/<species>/effects/soul.png`，避免複製共用檔案。

## 本次舊檔對應

- `BIRD/BIRD-normal2.png` → `pets/bird/adult/idle.png`
- `BIRD/BIRD-baby2.png` → `pets/bird/baby/idle.png`
- `BIRD/BIRD-adult-eat.png` → `pets/bird/adult/eating.png`
- `BIRD/BIRD-baby-eat.png` → `pets/bird/baby/eating.png`
- `BIRD/BIRD-normal.png` → `archive/pets/bird/adult/idle_v1.png`
- `BIRD/BIRD-baby.png` → `archive/pets/bird/baby/idle_v1.png`
- `EGG/Egg-normal.png` → `shared/egg/idle.png`
- `EGG/Egg-born.png` → `shared/egg/hatching.png`
- `FEED/bowl.png` → `shared/items/food_bowl.png`
- `dirty/dirty.png` → `shared/effects/dirt.png`
- `ghost/GHOST2.png` → `shared/effects/soul.png`
- `ghost/GHOST.png` → `archive/shared/effects/soul_v1.png`
- `GAME/Rock-Paper-Scissors.png` → `archive/games/rps/gestures_v1.png`
- `GAME/Rock-Paper-Scissors-2.png` → `games/rps/gestures.png`
- `tit/tit_idle_preview.png` → `previews/pets/bird/idle_concept.png`

## 韌體轉換與尺寸

- `tools/generate_bird_sprite.py`：讀取正式小鳥、蛋與飼料碗，產生 `src/ui/BirdSprite.h`；小鳥的睡眠圖目前由工具閉眼處理產生，沒有獨立的 `sleeping.png`。
- 小鳥、蛋圖集為 64×88，上下兩幀各 64×44；飼料碗為三幀 21×15。
- `tools/generate_wyvern_sprite.py`：產生 `src/ui/WyvernSprite.h`，保留原始像素；待機／傷心各為 64×88 上下兩格，餵食 128×88 為左上→右上→左下→右下四格，幼龍睡眠 64×88 兩格、成龍睡眠 64×44 單格，共 19 格、6688 bytes。龍原圖沒有手捧食物，餵食仍疊加共用碗。主畫面原點 (32,6)，待機 6 FPS、傷心與幼龍睡眠 1 FPS。實機步驟見 [小飛龍驗收](../docs/wyvern-test.md)。
- `tools/generate_dirty_sprite.ps1`：讀取 `shared/effects/dirt.png`，產生 `src/ui/DirtySprite.h`。
- `tools/generate_cleaning_sprite.py`：讀取 `shared/effects/cleaning.png`（196×114，左上／右上／左下／右下四格各 98×57），等比例以最近鄰縮成 84×49，產生 `src/ui/CleaningSprite.h`；右下全白格保留，沒有退水幀。
- `tools/generate_rps_icons.py`：讀取 `games/rps/`，產生 `src/ui/RpsIcons.h` 和 `docs/rps-icons-preview.png`。
- 治療藥水：`shared/items/potion.png` 為 42×63、每格 21×21 的 5 幀圖集，依左上、右上、左中、右中、左下播放；使用 `tools/generate_potion_sprite.py` 產生 `src/ui/PotionSprite.h`。
- `shared/effects/soul.png` 為 30×56，上下兩幀各 30×28；目前點陣資料保存在 `src/ui/PetIcons.cpp`，尚無獨立生成工具。
- `shared/effects/sad_lines.png` 為透明背景的 5×6 共用傷心效果，三條線各寬 1 像素、間隔 1 像素，長度由左到右為 5、5、6。依參考圖量測，疊加在 64×44 傷心幀的幼鳥座標 `(40,14)`、成鳥座標 `(39,9)`、幼龍 `(42,14)`、成龍 `(50,6)`；各幀使用相同位置。兩種寵物倒藥及空瓶期間都隱藏三條線。`tools/render_sad_effect_preview.py` 產生此圖示與 `previews/pets/bird/sad_lines_*` 預覽，保留原始鳥圖；`tools/generate_sad_effect.py` 由正式 PNG 生成 `src/ui/SadEffect.h`，由共用 `drawSadLines()` 繪製。`tools/render_sad_ui_preview.py` 檢查原生測試輸出的真實 UI 並產生 `docs/sad-state-preview.png`。

改動素材路徑時，要同步更新轉換工具及文件連結。`web-preview/assets/` 為網站展示自己的背景素材，與本目錄分開管理。
