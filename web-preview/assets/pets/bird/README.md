# 白色小鳥 Web 素材

基準為使用者確認的 `white-idle-v2.png`。v1、v2 靜態圖均保留。

`white-idle-source-v3.png`：內建 imagegen 依 v2 製作四格待機來源。提示要求：相同白色圓身、大棕眼、粉紅腮紅、黑灰翅膀與長尾巴；2×2 四格，睜眼，只有翼尖微微放鬆與尾端細微收放，不走路、不飛、不改比例、不增加毛流、透明背景，不含字或道具。

`tools/build_white_bird_idle.cjs` 使用 sharp 切格、整數平移、補透明留白，產出 `white-idle-sheet-v3.png`、JSON 與 JS 定位資料。以 alpha >=160 的主輪廓量測裁切範圍，忽略範圍外極淡邊緣；保留範圍內 RGBA，不改色、不逐格縮放。頭部上方區域中心與腳底共同定位；原始生成格間的細微造型差異不能靠平移消除。

四格時間 220／160／220／320ms，一輪 920ms。兩階段暫共用素材，成年／幼年分別縮放；不代表已有獨立幼鳥美術。

`white-bird-view.js` 為共同播放器；完整 Web 與 `white-bird-idle-preview.html` 均使用它。健康清醒且無照顧動作時播放圖集；其他狀態先使用 v2 靜態圖與既有效果。減少動態顯示第一格。`white-idle-contact-v3.png` 是相同播放器產生的四格花園合成檢查圖，並非瀏覽器截圖。

重建：環境須能載入 sharp，執行 `node tools/build_white_bird_idle.cjs`。檢查：`node web-preview/white-bird-idle-tests.cjs`。實作、驗證與待確認事項統一見 project-plan 第 10 節。

## 尾巴加強 v4
使用者要求尾巴更明顯。內建 imagegen 以 v3 圖集為參考，只要求尾巴從原斜角抬高、返回、放低，保留白鳥造型、腳底、睜眼及配色；來源為 white-idle-source-v4.png，現行播放器讀取 v4 圖集與資料。重建使用 node tools/build_white_bird_idle.cjs v4；頭部錨點改量測頂部 15% 以避開抬高尾巴。v3 原素材保留。

## 散步與喝水 v1
white-activity-source-v1.png 使用內建 imagegen，參考白鳥 v2 製作 2×3 六格：左右腳走路、半低頭、低頭喝水、抬頭吞嚥、站立。提示要求固定白鳥特徵、折翼、完整尾巴、透明背景、禁止道具場景；沒有採用另外兩次重試來源。原圖透明區有深色 RGB，但檢查 alpha=0，實際花園合成無底色。tools/build_white_bird_activity.cjs 以 alpha>=160 主輪廓切格並平移腳底對齊，統一比例輸出圖集與資料。白鳥主體不同姿勢仍有生成差異。white-bird-activity.js 為展示用路徑與動作播放器：12 秒一輪，停留2秒、步行3秒、喝水3.5秒、返回3秒、停留0.5秒；未改養成規則。減少動態回到靜態待機，不移動；照顧、傷心、生病、睡眠不觸發。共同預覽 white-bird-activity-preview.html；檢查 node web-preview/white-bird-activity-tests.cjs；tools/render_white_bird_activity.cjs 可產生正式繪圖程式合成檢查圖，非瀏覽器截圖。

## 含淚傷心 v1
white-sad-source-v1.png 為內建 imagegen 依白鳥 v2 製作的橫排兩格：含淚睜眼、閉眼落淚。提示要求只改委屈眉眼與少量淚珠，固定體型、頭部傾斜、腳底、折翼與尾巴、原白灰配色及透明背景，不哭喊或誇張淚流。tools/build_white_bird_sad.cjs 裁切、整數平移與留白產生 879px 方格圖集及 JSON/JS 資料，未逐格縮放；腳底與頭頂區域中心對齊。white-bird-view.js 共用播放器採 2200/240ms，不改領域傷心門檻。其他狀態與照顧流程優先，減少動態停第一格。兩階段共用素材與原先顯示比例。預覽 white-bird-sad-preview.html；測試 node web-preview/white-bird-sad-tests.cjs；花園合成圖由 tools/render_white_bird_sad.cjs 產生，非瀏覽器截圖。尚待使用者確認哭泣自然度。

## 沒精神生病 v1
內建 imagegen 依白鳥 v2 製作 white-sick-source-v1.png，提示要求保留白灰體型、折翼和長尾，低垂站姿、沉重半閉眼／緩慢閉眼，不流淚、不改色、不畫冷汗或背景。node tools/build_white_bird_sad.cjs sick 整理為兩格；白鳥播放器採 1400/900ms。每10秒中的3至7秒有一次緩慢左右搖晃，以腳底支點旋轉整張圖，前後靜止且平滑接合，不切割肢體或縮放假裝動作。最大旋轉上限0.075弧度，實際包絡峰值更小。冷汗獨立疊在頭側，非眼下；治療停第一格且不搖、不顯示冷汗；睡眠優先。減少動態固定第一格不搖。white-bird-sick-preview.html 與完整 Web 共用繪圖程式；node tools/render_white_bird_sad.cjs sick 產生場景合成圖，非瀏覽器截圖；node web-preview/white-bird-sick-tests.cjs 驗證節奏與狀態優先序。

## 低頭啄食 v1
white-bird-feed.js 沿用已生成並整理好的 white-activity-sheet-v1.png 完整半低頭／低頭／抬頭姿勢，沒有再生成或分割頭部。餵食三秒內每0.75秒完成一次啄食共三次，2.25秒後抬頭吞嚥，對應藍色食盆四階段；不附帶喝水水珠。腳底固定201，依兩階段顯示比例將盆移到低頭喙尖下方，保留小龍原餵食呈現。減少動態顯示站姿與遞減食盆。完整 Web 實際餵食、外觀展示中的吃飯及 white-bird-feed-preview.html 共用播放器，不改數值／結算。測試 node web-preview/white-bird-feed-tests.cjs；合成檢查圖 tools/render_white_bird_feed.cjs。

## 長眠 v1
white-dead-source-v1.png 為使用者以 ChatGPT 依白鳥 v2 產出的綠底單張（提示詞見 white-dead-v1-prompt.md）：往一側倒下躺在地上、閉眼、腮紅、翅膀鬆開、尾巴平放、小腳收在肚子前。`python tools/prepare_white_bird_dead.py` 去綠底、移除小於 200 像素的雜點、裁切，輸出 white-dead-v1.png 與 white-dead-data-v1.js／.json；不縮放、不重畫。原圖與待機 v2 同為 1254 px 畫布，Web 以相同像素比例繪製，身體中心（裁切寬 42%）對準角色中心、底部貼齊腳底線。幼鳥與成鳥共用，按大小區分。`white-bird-view.js` 在 healthState 為 dead 時使用；`BirdView.still(…, closed=true)` 代表長眠外觀（死亡流程、紀念冊），展示工具的「長眠外觀」也改用它，睡眠仍是站姿。測試 node web-preview/white-bird-dead-tests.cjs。
