# 第三顆蛋：水彩暈染蛋 v1 提示詞（2026-10-24 草稿）

取代三選一圖中右邊的波浪紋蛋，因為它太像復活節彩蛋。三顆蛋會依 petId 隨機出現，物種跟花紋無關，所以這顆要繽紛，但不能比另外兩顆高級。不加星星、光暈、金屬或寶石感。

附圖：`egg-designs-source-v1.png`（GPT 三蛋 v1）。只拿來對齊外形、大小、描邊和光源，不沿用其中任何一顆的花紋。

輸出：1024×1024，單顆蛋。

```
Use the attached image of three pixel-art eggs as the reference for shape and style. Draw ONE new egg in exactly the same style, so that it could sit next to them as a fourth egg of the same set.

Keep identical to the reference eggs:
- The same upright oval silhouette, slightly wider at the bottom, at the same proportions.
- The same clean dark warm-brown outline thickness and the same chunky visible pixel size.
- The same soft lighting: a small white highlight near the upper left, gently darker toward the lower right and the bottom.
- It is an inanimate egg: no face, no eyes, no mouth, no blush, no limbs.

New shell pattern (this is the only thing that changes):
- The shell looks softly dyed with watercolor, like hand-dyed fabric. Four or five large irregular patches of soft pastel color: peach, butter yellow, mint, sky blue and lilac.
- Neighboring patches melt into each other through a few stepped pixel color bands, with no hard borders and no outlines between them. A little creamy shell color shows through in places.
- Colors stay soft and friendly, at the same brightness as the reference eggs, never dark or neon.
- The patches follow the curve of the shell and are darker on the shaded side, just like the rest of the egg.
- The egg looks cheerful and ordinary, not rare or magical: no stars, no sparkles, no glow, no rainbow shine, no metallic or pearly sheen, no gems, no gold.

Layout: a single egg centered in the image, upright, about 55% of the image height, with plenty of empty space around it. Transparent background. No nest, no grass, no ground, no shadow, no cracks, no text.
```

## 產出後檢查

- 外形、描邊粗細、像素顆粒和高光位置是否跟另外兩顆一致。
- 色塊有沒有暈開，不能是幾塊邊界清楚的色塊。
- 有沒有出現星星、光暈、珠光或寶石感。
- 背景是否真的透明。棋盤格外觀不算，要檢查 Alpha 通道。
