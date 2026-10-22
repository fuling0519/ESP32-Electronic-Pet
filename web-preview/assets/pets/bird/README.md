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

## 睡眠與受驚連續圖格 v1
- white-sleep-sheet-v1.png 與 white-startle-sheet-v1.png 由確認過的 white-idle-v2.png 原圖逐格合成。每格保留同一張鳥身圖，只在眼部／嘴部有限區域疊加表情；圖集尺寸固定1254px每格，沿用共同播放器腳底錨點與原本縮小後顯示比例。
- 睡眠三格：眼皮落下、平直微斜閉眼線的過渡格；週期180／260／2400ms。以大眼原有範圍為基準，只在中央畫細直線，不使用彎弧笑眼。閉眼後停留並以全身約1%的緩慢呼吸起伏呈現安穩；減少動態固定閉眼格且不呼吸。
- 受驚四格：張眼、驚嚇加深、受驚停格、落地後微皺眉。戳碰呈現先縮身、向鳥喙反方向（右側）跳開、落地回彈；不改寵物數值、存檔、BLE或養成規則。
- white-bird-view.js 為完整 Web、睡眠狀態及展示反應共用的圖集播放器；index.html 載入兩份圖格資料。待使用者在完整頁確認表情與動作。
- 圖集由 Windows System.Drawing 從原始 PNG 複合產生；這些是程式合成圖格，並非另外生成的手繪角色來源。white-sleep-data-v1.js/json、white-startle-data-v1.js/json 記錄播放節奏和固定定位。

## 五格睡眠呼吸 v2
- white-sleep-sheet-v2.png 以 v1 第三格（穩定閉眼姿勢）複製成五個同尺寸影格；五格共用同一角色圖、透明背景、腳底錨點與比例，避免生成新畫面造成角色外觀漂移。
- white-sleep-data-v2.js/json 設定穩定、吸氣、隆起、吐氣、回復五段節奏；播放器依五段呼吸幅度輕微縮放整張角色圖，腳底錨點保持固定。減少動態固定原始姿勢。
- 目前隆起以整體極小幅度縮放近似，尚未局部變形腹部；圖集五格角色像素相同，動態差異由播放器呈現。圖片尺寸 6270×1254，未作 PNG 最佳化，需留意下載大小。

## 使用者提供的五格睡眠呼吸 v3
- `white-sleep-sheet-v3.png` 由使用者提供的五格圖切格、透明背景清理並對齊腳底產生；保留原始格間造型差異，不逐格縮放。圖集為 5×434px 方格，沿用同一繪製錨點與可見高度。
- 分鏡由左至右：穩定睡姿、輕微吸氣、吸氣高點、輕微吐氣、回到原始姿勢。每格停留 1200／900／1000／1200／1200ms，最後一格銜接第一格循環。
- `white-sleep-data-v3.js/json` 記錄格尺寸、定位、時間及步驟；`white-bird-view.js` 播放附件圖格，不再另外整體縮放角色。只更新 Web 素材與播放器，韌體未修改。

## 驚嚇四格 v2

使用者提供的 `white-startle-source-v2.png` 為 2×2 四格圖。轉換為 `white-startle-sheet-v2.png` 時依序裁切四格、移除連通至畫格邊界的淺色／棋盤背景、清除零散背景像素，並以腳底與角色範圍中心對齊；每格 512×512，未重繪或縮放角色。`white-startle-data-v2.json/js` 使用錨點 `[256,480]` 與 150／180／300／570ms 節奏（共1.2秒）。

`white-bird-view.js` 與完整 Web 以此圖集播放原地驚嚇反應，不再額外平移或跳開；約 1.1 秒後接回正常角色圖。驚嚇外觀只供展示，不更改數值或存檔。已目視檢查透明圖集；完整瀏覽器播放與使用者對表情自然度的確認仍待完成。
