# 第三顆蛋：粉彩色塊蛋 v2 提示詞（2026-10-24 草稿）

v1（`egg-watercolor-source-v1-rejected.png`）跟另外兩顆畫風不一致：
- 「watercolor」讓色塊邊緣變成模糊的暈染，另外兩顆是邊緣清楚的像素色塊。
- 殼幾乎沒有明暗，看起來扁平；另外兩顆右下明顯較暗，有立體感。
- 色塊是方正的大格子，像拼布；高光也又大又亮，像上了亮漆。

v2 拿掉「水彩」這個詞，改成邊緣清楚、形狀圓潤的粉彩色塊，並把明暗和高光要求寫得跟參考圖一樣。用程式減色修 v1 也試過，結果變得髒灰，所以重產。

附圖：`egg-designs-source-v1.png`。

輸出：1024×1024，單顆蛋。

```
Use the attached image of three pixel-art eggs as the style reference. Draw ONE new egg that looks like it was drawn by the same artist in the same session, so it fits in as a fourth egg of that set. Look closely at the leftmost spotted egg and copy how it is rendered.

Copy exactly from the reference:
- The same upright oval silhouette, slightly wider at the bottom.
- The same chunky pixel size and the same dark warm-brown outline thickness.
- Crisp pixel art: every color area has hard, clean pixel edges. No blur, no airbrush, no soft gradients, no smudging.
- The same shell shading: creamy off-white, clearly darker toward the lower right and the bottom in two or three visible pixel steps, so the egg looks round.
- The same small white highlight streak near the upper left, the same size as in the reference, painted directly on the shell.
- It is an inanimate egg: no face, no eyes, no mouth, no blush, no limbs.

New shell pattern (the only thing that changes):
- Five or six rounded, organic pastel blobs of different sizes, like soft clouds or puddles, in peach, butter yellow, mint, sky blue and lilac. Some blobs wrap around the edge of the egg and are cut off by the outline.
- Each blob is a flat pastel color with a crisp pixel edge, plus at most one slightly lighter pixel row along its upper-left edge. Blobs do not blend into each other and have no outlines.
- Creamy shell color stays visible between the blobs.
- Blobs on the shaded lower-right side are darker, following the same shading steps as the shell.
- Cheerful and ordinary, not rare or magical: no stars, no sparkles, no glow, no glossy shine, no pearly or metallic sheen, no gems.

Layout: a single egg centered in the image, upright, about 55% of the image height, with plenty of empty space around it. Transparent background. No nest, no grass, no ground, no shadow, no cracks, no text.
```

## 產出後檢查

- 跟斑點蛋並排看：像素顆粒、描邊、明暗階數、高光大小是否一致。
- 色塊邊緣是否清楚，沒有模糊暈染。
- 有沒有出現亮漆感、星星或寶石感。
- 背景是否真的透明。棋盤格外觀不算，要檢查 Alpha 通道。
