# 蛋裂痕與破殼 v1 提示詞（2026-10-24 草稿）

## 做法：只請 GPT 畫一顆素色蛋，再用程式套到三顆蛋上

如果請 GPT 分別在三顆蛋上畫裂痕，它每次都會重畫整顆蛋，斑點、點點和波浪的位置、大小都會跑掉（規則第 5 節第 51 條）。這樣從完整蛋切到裂痕蛋的那一格，花紋會整個跳一下。

所以這次只產一張圖：一顆**沒有花紋的素色蛋**，依序畫出完整、小裂痕、大裂痕、破殼後的下半殼和上蓋。之後我用程式處理：
- **裂痕**：比對「完整」和「小／大裂痕」兩格，抽出裂痕線條，疊到三顆正式的蛋上。
- **破殼**：沿大裂痕的線把正式的蛋切成上下兩塊，所以殼外的花紋一定跟原本的蛋一樣。下半殼開口看到的內壁和斷面，取 GPT 畫的樣子。
- **飛散的小碎殼和裂縫透出的光**：用程式畫，不放進這張圖。

裂痕要從小到大連續長出來，大裂痕也要當破殼的切線，所以提示詞要求小裂痕是大裂痕的中間一段，大裂痕則要橫跨整顆蛋。

附圖：`egg-designs-v1.png`（三顆定案蛋）。只拿來對齊外形、描邊、像素顆粒和光源，不畫任何花紋。

輸出：1536×1024 橫式，3 欄 × 2 列，用 5 格，右下那格留空。

```
Use the attached image of three pixel-art eggs as the style reference. Draw the same egg in the same pixel-art style, but as a PLAIN egg with no pattern: a creamy off-white shell with no spots, dots or stripes.

Keep identical to the reference in every cell:
- The same upright oval silhouette, slightly wider at the bottom, the same size and the same position inside each cell.
- The same chunky pixel size, the same dark warm-brown outline, crisp pixel edges, no blur.
- The same shading: clearly darker toward the lower right and the bottom in a few visible pixel steps, and the same small white highlight streak near the upper left.
- It is an inanimate egg: no face, no eyes, no limbs.

Cracks are drawn as jagged, chunky pixel lines in the same dark warm-brown as the outline, one pixel wide, with a thin lighter cream pixel line along their lower edge so they look slightly sunken into the shell. They stay on the shell surface and never change the egg's silhouette.

Layout: a 3-column x 2-row grid on one image, the same egg in every used cell, egg bottoms on the same baseline in each row, plenty of empty space around each egg.

Row 1 - the egg cracking, step by step (the egg itself does not move or change size):
1. The intact plain egg, no cracks.
2. Small crack: one short jagged zigzag crack in the middle of the egg's width, at about 40% of the egg's height from the top. It is about one quarter of the egg's width long.
3. Large crack: the same crack has grown sideways into one long jagged zigzag line that runs across the whole width of the egg at about 40% of its height, from the left outline to the right outline. Two or three very short side branches split off from it. Everything else is the same as cell 2.

Row 2 - the egg after hatching, broken exactly along the large crack from cell 3, empty inside:
4. The bottom part of the shell still standing upright: its top edge is the same jagged zigzag line as the large crack. Through the open top we see the inside back wall of the shell, a slightly darker, plain warm cream, and the broken edge shows the shell's thin thickness as a light cream rim. Nothing is inside.
5. The top cap of the shell that broke off, shown on its own, upright, with the same jagged zigzag bottom edge so that it would fit exactly back onto cell 4. Its inside is not visible.
6. Leave this cell empty.

Transparent background. No chick or creature, no yolk, no light or glow coming out of the cracks, no flying shell fragments, no sparkles, no nest, no ground, no shadow, no text, no numbers, no grid lines.
```

## 產出後檢查

- 第 1–3 格的蛋形、大小、位置是否完全一樣（只差裂痕），不然抽不出乾淨的裂痕。
- 小裂痕是不是大裂痕中間的一段；大裂痕有沒有從左描邊連到右描邊。
- 第 4、5 格的斷口是否同一條鋸齒線，拼回去要剛好；第 4 格裡面要是空的。
- 有沒有被畫成角色，或從裂縫冒光。
- 背景是否真的透明。棋盤格外觀不算，要檢查 Alpha 通道。
