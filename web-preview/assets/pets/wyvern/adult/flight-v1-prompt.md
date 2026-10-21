# 成年小飛龍六格飛行初稿 v1

2026-10-21。使用內建 image_gen，主要參考 `flight-three-quarter-preview-v1.png`。

使用者已確認以下動作提示詞並授權繪製；生成圖的造型、六格連續性及動畫自然度尚待確認。當次範圍為製圖，尚未切格對齊或接入 Web 移動流程。

圖源：`flight-source-v1.png`，1536×1024，8-bit RGBA；Alpha 範圍 0–254，860215 個像素完全透明。保留生成原始像素，尚未正規化圖集；不能直接假定等寬切格不會切到輪廓。

## 生成提示詞

Use case: stylized-concept.
Asset type: six-frame transparent flight animation sprite sheet for an existing adult pet dragon.
The attached image is the sole PRIMARY CHARACTER DESIGN REFERENCE, not a sheet to repeat unchanged. Generate the SAME character in six successive wingbeat poses. Preserve the reference's right-facing three-quarter view and complete body.

以附上的成年小飛龍飛行參考圖為主要造型依據，製作同一角色的六格連續飛行精靈圖。
保留紫色圓臉、小豆眼與白色高光、小嘴、粉色腮紅、奶油色雙角、額前突起、粉紫耳內、奶油色分節腹部、短小四肢、紫色翼骨與奶油色翼膜，以及向後延伸的彎尾。維持參考圖的頭身比例、彩色像素風格、深紫描邊、柔和明暗與光源；不放大眼睛、不增加黃色口鼻面罩，不增減肢體。
小飛龍面朝右方，保持參考圖的四分之三側面與微微前傾的飛行姿勢，像在花園低空悠閒巡遊。神情放鬆，小嘴維持原本的可愛表情。雙翼有清楚但不吃力的拍動，不呈現急速衝刺或奮力起飛。
動作以雙翼為主：翼根帶動翅膀，翼尖和翼膜稍微延遲跟隨；遠側翅膀保持正確透視，與近側翅膀協調拍動。尾巴末端只有輕微跟隨擺動。
六格依序呈現一個完整拍翼循環：
1. 上拍轉折：雙翼向背部上方舒展，翼尖自然彎曲，準備向下拍。
2. 開始下拍：雙翼從上一格向身體兩側展開，翼膜逐漸張開，翼尖稍微落後。
3. 下拍後段：雙翼繼續向腹部兩側下方掃動，翼膜充分展開；保持臉部清楚可見。
4. 下拍轉折：雙翼到達本次拍動最低處，翼尖柔和回彈，準備向上收回。
5. 向上回收：雙翼從上一格抬起，略微收攏翼膜，呈現與下拍不同的回收姿態。
6. 接近上拍完成：雙翼繼續向背部上方舒展，逐漸張開翼膜，姿勢自然銜接第一格，不突然跳動。
每格保持相同視角、角色大小與身體中心定位。頭部與軀幹位置穩定，五官、角、耳朵及腹部分節不隨拍翼變形；前爪自然收在胸腹旁，後腳微微向後收起，維持參考姿勢，不做跑步動作。不要靠整隻角色平移、旋轉、拉伸或縮放製造飛行感。
圖集排列為三欄兩列，從左到右、由上到下依序放置六格。每格畫布等大，預留足夠透明空間，完整容納雙翼、角、腳和尾巴，不裁切輪廓。輸出真正帶 Alpha 透明背景的 PNG，不畫背景、地面、投影、速度線、文字、編號或格線。

## 透明背景修正提示詞

Edit the attached six-frame dragon sprite sheet. BACKGROUND REMOVAL ONLY. Remove the entire purple/black gradient background and all purple glow between and around the six dragons. Replace it with actual fully transparent Alpha=0 pixels. Retain all six complete purple dragons exactly, including their purple outlines, purple skin, cream wing membranes and belly, eyes, feet, tail and tiny edges. Preserve their six distinct wingbeat poses, right-facing three-quarter camera, 3 columns by 2 rows, existing layout, character proportions, colors, soft pixel art shading. Do not redraw the characters, do not add halos, shadows, background colors, checkerboard patterns, text or gridlines. Output a PNG with a genuinely transparent background and crisp preserved character edges.

背景修正仍由 image_gen 處理。背景 RGB 殘留色不等於可見背景；透明性依實際 Alpha 檢查。
