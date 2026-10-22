# 共用清潔・配色與尺寸試樣 v1

2026-10-22：只供使用者確認配色、描邊、像素密度與道具比例；尚未確認，未製作完整清潔動畫、未接入正式清潔流程。未改韌體或養成規則。

- `soap-source-v1.png`、`foam-source-v1.png`：內建 imagegen 生成，透明 RGBA，1586×992。以成年小龍 `idle-preview-v1.png` 和白鳥 `white-idle-v2.png` 作風格參考；泡沫另參考本輪肥皂。完整提示見 [prompts.md](prompts.md)。
- `soap-sample-v1.png`、`foam-sample-v1.png`：以 alpha >=160 找主輪廓外框並裁切；保留外框內所有 RGBA，不改色、不重畫、不拉伸，不對 alpha 做二值化。極淡外圍可能被裁去。原圖 SHA-256、裁切座標及尺寸見 [metadata.json](metadata.json)。
- 生成目標配色：肥皂奶油白／杏色陰影／灰紫描邊；泡沫乳白／淺藍／灰藍輪廓。這是提示用目標色，未量化為固定色盤。
- 尺寸以 384×256 場景畫布計算：成年肥皂寬 32 px、單團泡沫寬 28 px；幼年乘 0.82 並取整，約 26／23 px。全部等比縮放，角色比例不變；不聲稱已建立固定原生像素網格。
- [v1 比較頁](../../../../cleaning-style-preview-v1.html) 可切日夜、調整道具尺寸及隱藏效果，與合成圖共用 `cleaning-style-study.js`；角色沿用正式 `WhiteBirdView`、`WyvernAdultView`、`WyvernView`。幼龍目前仍是原有遮罩版本，並非新製彩色幼龍。2026-10-22 使用者回饋肥皂 32 px 偏小、40 px 較正常，且 v1 肥皂與泡沫像放在地上；後續改為提起的擦拭角度與正面數量泡群，見 [v2 比較頁](../../../../cleaning-style-preview-v2.html)。v1 不作為已確認造型。
- `scene-contact-v1.png`：四種現有角色／階段的日夜合成，原尺寸；`size-contact-v1.png`：試樣原尺寸與 4 倍放大。由 `tools/render_cleaning_style_study.cjs` 使用正式角色播放器與同一試樣繪圖函式產生，並非瀏覽器截圖。
- 已在本機瀏覽器開啟比較頁，檢查日夜切換、肥皂寬 32／40 px，再還原預設 32／28 px；`browser-preview-v1.png` 是還原後的實際瀏覽器畫面。這是有限的靜態檢視，不代表完整動畫或所有尺寸已驗收。

重建合成圖需 Node.js、`@napi-rs/canvas`，從專案根執行 `node tools/render_cleaning_style_study.cjs`。

預覽不讀寫寵物存檔。確認範圍只包含靜態配色、大小與遮擋；不將本輪視覺確認視為動畫連貫性、全部物種適配或硬體驗收。
