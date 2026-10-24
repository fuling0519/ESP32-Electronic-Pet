# 蛋造型三選一 v1 提示詞（2026-10-24 草稿）

附圖（兩張都附）：
1. `firmware-egg-reference.png`：韌體單色斑點蛋放大圖，只當外形與比例參考。韌體的斑點位置是配合硬體解析度畫的，Web 版不沿用，斑點交給 GPT 自由設計。
2. `../../garden-detailed-day.png`：Web 花園背景，只當配色、像素顆粒與光源參考。

不附小鳥或小龍圖：附角色圖時，生成結果容易帶入角色的五官、翅膀或尾巴。PixelLab 第一輪就發生過。

輸出：1536×1024 橫式，一列三顆蛋，每顆是一個候選造型，用來挑出正式的蛋。挑定後，裂痕和破殼另外製作。

```
Draw three pixel-art egg designs side by side for a cozy farm pet game. They are three alternative designs of the same egg, shown as three candidates to choose from.

References:
- The first attached image is a black-and-white low-resolution egg. Use it only for the egg's silhouette and proportions: an upright oval, slightly wider at the bottom, with a rounded top. Do not copy its spot pattern, its flat black fill or its low resolution.
- The second attached image is the game's background. Match its art style: soft bright pastel colors, clearly visible chunky pixels, gentle shading, and warm sunny light from the upper left.

Shared by all three eggs (keep identical):
- The egg is an inanimate object: no face, no eyes, no mouth, no blush, no limbs, no wings, no tail.
- The same silhouette, the same size, and the same upright angle.
- A creamy off-white shell with soft shading: a small bright highlight near the upper left, gently darker toward the lower right and the bottom.
- A clean dark warm-brown outline, about the same thickness on every egg.
- The surface pattern looks printed on the shell and follows its curve. It is not a sticker and is not raised.

The three designs, from left to right:
1. Spotted egg: a playful, decorative spotted pattern in warm browns and caramel tones. Freely mix spot sizes and shapes, from big organic blotches to tiny specks, and feel free to add subtle accents such as lighter rims or soft color variation inside the spots. Place them however looks most charming, as long as they do not form a face. The spots are slightly darker on the shaded side.
2. Pastel dot egg: eight to ten small round dots of different sizes in soft pink and sage green, scattered unevenly over the shell. They do not line up in rows and do not form a face.
3. Zigzag band egg: one soft peach zigzag band wrapping around the middle of the shell, with a thin lighter stripe along its upper edge. The band bends slightly with the curve of the egg.

Layout: one horizontal row of three eggs with equal spacing. Each egg is upright, about 55% of the image height, and the bottoms of all three eggs sit on the same invisible baseline. Leave plenty of empty space around each egg. Transparent background. No nest, no grass, no ground, no shadow, no cracks, no sparkles, no text, no numbers, no grid lines.
```

## 產出後檢查

- 三顆蛋的外形、大小、底線和描邊是否一致。
- 有沒有被畫成角色（長出臉、腮紅、手腳）。
- 斑點有沒有排成像臉的圖案。
- 背景是否真的透明。棋盤格外觀不算，要檢查 Alpha 通道。
