# 共用清潔・使用者參考收腰海綿 v4

2026-10-22：使用者提供收腰海綿圖，指出原先期待兩端圓胖、中間內凹的通用輪廓。v3 的不規則橢圓未獲採用。v4 直接以 [使用者參考](user-shape-reference.png) 作主要造型與角度參考，前版僅作像素風格和配色參考。

- 內建 imagegen 產生 `sponge-source-v4.png`；裁切主輪廓留白為 `sponge-sample-v4.png`，不改色、不重畫、不拉伸。裁切內 RGBA 無損、真 Alpha 已檢查；原圖與雜湊／裁切座標見 [metadata.json](metadata.json)，完整提示見 [prompts.md](prompts.md)。
- 兩端圓胖、兩側向內收腰，斜向呈現側邊厚度；黃色孔洞材質、柔和彩色像素風。所有寵物／成長階段的海綿皆 60 px 寬，幼年不縮小；畫布是 384×256。因斜角較高，定位略向右下避開臉部。泡沫沿用 v2 三種數量，未改畫。
- [目前預覽](../../../../cleaning-style-preview.html) 與合成圖共用繪圖函式，角色使用現有正式播放器。[v3 預覽](../../../../cleaning-style-preview-v3.html) 保留供比較。幼龍仍為原有遮罩版本。
- `scene-contact-v4.png`、`quantity-scenes-v4.png`、`size-contact-v4.png` 是相同播放器的日夜、泡沫數量與放大細節合成圖，並非瀏覽器截圖。重建：`node tools/render_cleaning_style_study.cjs v4`，需 `@napi-rs/canvas`。
- 已於本機瀏覽器查看白天／中量泡沫與四種角色，`browser-preview-v4.png` 為實際畫面；日夜合成圖另已檢視。只證明本輪靜態預覽已查看，不代表完整動畫或美術驗收。

本輪是靜態造型修正與獨立預覽，未接正式清潔動畫，未改韌體、養成數值或存檔。使用者於 2026-10-22 回覆「好可以，那接著做精靈圖吧」，確認 v4 收腰海綿造型及既定 60 px 基準；後續又確認六份提示詞並授權生成。此確認不延伸為新動畫格或完整播放已驗收；後續見 [清潔動畫素材](../animation-v1/README.md)。
