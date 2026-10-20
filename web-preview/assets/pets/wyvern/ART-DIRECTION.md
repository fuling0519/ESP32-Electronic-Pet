# Web 小飛龍美術提案 v1（2026-10-20）

> 最新整合狀態見 [成年素材說明](adult/README.md)。成年正面、飛行側面、四格待機及六格表情已確認；待機、表情、蜷縮睡眠與死亡靜態現已接入 Web，飛行靜態造型可預覽。[全部狀態總覽](../../../wyvern-states-preview.html) 使用相同繪製程式。整合後的美術比例與瀏覽器畫面待確認；韌體未替換。以下為整合前的歷史設計記錄，其中「尚未替換」均描述當時狀態，不代表目前進度。

> **後續回饋：幼年 v1 方向未採用；成年靜態美術方向已確認。** 使用者偏好原本較簡單的可愛造型，提供成年龍手繪參考後，新增 `adult/idle-preview-v1.png`：保留小豆眼、小嘴、紫色完整臉部、奶油角與腹部、短翼與小尾巴，減少大眼、亮面與過強立體感。2026-10-20 使用者回覆「嗯嗯，就這個吧」，確認此成年靜態圖作為後續美術基準。此確認不代表幼年版本、動畫、固定網格與色盤或 Web 整合已完成／驗收；正式 Web 角色尚未替換。下文保留第一張幼年稿的設計與技術記錄，不代表目前已接受的方向。

早期 Phase 1 記錄：當時只有一張幼年靜態預覽，未接入正式角色渲染；該幼年方向後續未採用。

## 現況與原因

- `web-preview/wyvern-sprites.js` 使用來自 `src/ui/WyvernSprite.h` 的 19 張原始 64×44 單色遮罩。
- `wyvern-view.js` 的 `paint()` 以封閉區域填色、位置規則判斷腹部，再描深色外框。頭、翅膀與尾巴大多是單一紫色，腹部是黃色和深色橫帶。
- 這種方式保留了 OLED 輪廓，也保留了低解析度的粗大階梯、細節限制與平面色塊；提高 Canvas 輸出尺寸不會增加美術資訊。
- 現行背景由 `app.js` 載入 `garden-detailed-day.png`／`garden-detailed-night.png`，樹冠、建物與草地具有多階色彩、體積及材質。角色與背景之間的問題主要是繪製方法與細節密度。

## 新版設計

- **配色**：保留薰衣草紫身體與奶油黃腹部；亮面向淡紫偏暖，暗面向冷紫偏移，翅膜採柔和金杏色。避免純黑粗框壓過柔和環境。
- **光影**：以左上暖光統一額頭、臉頰、腹部與翅骨；下顎、翼根及尾巴內側加入冷紫遮蔽陰影。腹部分節以暖色暗面區分，取代粗黑切帶。
- **角色辨識**：保留大頭短身、幼態圓臉、大眼、小角、側耳、兩側小翼、短腳與捲尾。雙翼作為前肢，不額外新增一對手臂。
- **場景協調**：以重新繪製的像素色塊、階梯輪廓與材質層次接近現有彩色花園；暖黃呼應花朵及乾草，紫色與綠色草地形成清楚但柔和的主體分離。
- **獨立來源**：圖像生成工具依角色與花園參考重新繪製，未將原 OLED 遮罩放大填色。參考圖是 `wyvern-color-preview.png` 左側幼龍及現有日／夜花園。

## 資產與預覽

- 靜態 PNG：`baby/idle-preview-v1.png`，1254×1254，RGBA，856,822 bytes。
- 真正透明背景，不含草地、方格底或地面陰影。原圖生成後直接保存，未進行縮圖、去背或色盤轉換。
- 從 `web-preview/wyvern-art-preview.html` 比較舊版與新版，並在既有日／夜背景上試放同一張 PNG。方格與接地陰影由檢視頁 CSS 提供。
- 檢視頁刻意只對背景套用現行頁面的 `brightness(1.10) saturate(1.08)`；角色原色保持一致，便於看出夜間亮度差異。

## 驗證與邊界

- 已讀取 PNG 像素確認尺寸與 32-bit RGBA／Alpha 通道，含 908,411 個全透明像素，並非實色底或烘焙方格。
- 生成稿有半透明邊緣，主體常見 Alpha 值為 252～254；這是美術候選稿，尚非固定網格、固定色盤且只含 0／255 Alpha 的最終動畫 sprite。
- 已目視檢查單圖的五官、雙翼、尾巴與完整構圖。保留辨識度、精緻度與日夜場景融合仍需使用者美術確認，不據此宣稱驗收通過。
- 獨立 HTML 的本地引用與腳本語法已檢查。瀏覽器安全政策不允許工具開啟 `file:` 網址，本輪未完成實際瀏覽器渲染驗證；日夜試放頁可由使用者本機開啟查看。
- 未更動 `assets/pets/wyvern/` 韌體素材、`src/ui/WyvernSprite.h`、既有 19 張遮罩、成年外觀或正式 Web 養成流程。不涉及硬體驗收。

## 後續整合建議（Phase 2，方向確認後才執行）

1. 先定稿幼年中性造型，再決定實際顯示尺寸、像素網格與小尺寸清晰度。不要將 1254px 生成輸出誤稱為原生 192px sprite。
2. 角色資產延續 `web-preview/assets/pets/wyvern/baby/`，定稿可命名為 `idle-00.png`、`idle-01.png`、`sleep-00.png`、`sad-00.png`、`sick-00.png`；目前只有 `idle-preview-v1.png` 存在。
3. 所有狀態統一透明畫布、頭身比例、腳底錨點與受光方向。Web 與韌體路徑分離；成年素材另設 `adult/`，不覆蓋現有韌體來源。
4. 經確認後再替換 Web 的角色呈現層，沿用既有狀態判定、養成規則與動作時序。載入未完成或狀態資產尚未齊備的處理也應先定義。
5. 先規劃待機的微呼吸／眨眼、睡眠的閉眼／收翼、生病與傷心的表情；尚未製作，也未把它們列為已完成。

## 生成設計重點

High-quality colorful pixel art baby wyvern; redraw from scratch, never trace or recolor the 64×44 OLED mask. Oversized round head, tiny pear body, small lilac horn nubs, dark plum eyes, sweet smiling muzzle, two little bat wing arms, two hind feet, curling purple tail. Lavender skin, cream segmented belly, golden-peach membranes, warm upper-left light and cool violet shadows. Coherent crisp pixel clusters, selective plum outlines, cozy indie creature-raising game quality matching the existing sunny pixel garden. One neutral full-body pose on genuine transparent RGBA, no ground, shadow, text or extra subjects.
