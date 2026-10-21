# Web 藍色飼料盆

使用者提供藍色貓臉食盆，要求像素飼料逐步吃完。內建 imagegen 製作 `blue-seed-bowl-source-v1.png`；提示要求保留藍盆、貓臉和白色爪印，移除原截圖註解及背景，生成 2×2 滿盆／部分／少量／空盆，僅飼料減少、盆體不變，真正 Alpha 透明。

`tools/build_blue_seed_bowl.cjs` 依原圖格距裁切主輪廓、整數平移對齊盆底及中心，保留範圍內 RGBA，不改色、不重畫、不逐格縮放，輸出固定 745×496 方格（非正方形）的圖集與定位資料；原生成的盆體細節仍有微小差異。

`blue-seed-bowl.js` 共用播放器每 0.75 秒切一格，2.25 秒起空盆，3 秒由原本餵食流程結算。完整 Web 的兩種寵物實際餵食皆使用此共用盆；原始韌體素材不改，圖片未載入時沿用原盆。只更動呈現，不改數值、結算或存檔。

獨立預覽 `blue-seed-bowl-preview.html` 使用同一播放器，比較遊戲 64px 寬與放大版本。測試 `node web-preview/blue-seed-bowl-tests.cjs`；重建需 sharp，執行 `node tools/build_blue_seed_bowl.cjs`。`tools/render_blue_seed_bowl.cjs` 產生四格合成檢查圖，非瀏覽器截圖。實作驗證與使用者驗收見 project-plan 第10節。
