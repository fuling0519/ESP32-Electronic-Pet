# 幼龍睡覺 v2 提示詞：只重畫一格（2026-10-23 草稿）

圖集 C（`sheet-c-source-v1.png`）的換腳走路格與死亡已採用並接入。睡覺問題：
- 尾巴末端是圓的，像貓尾巴；小龍的尾巴應該尖尖的（見成龍睡姿與 A 的站姿）。
- 兩格「呼吸」頭的大小、角與耳朵位置都不同，交替播放時整隻抖動。生成工具做不到「只改背部」的兩格，所以改成**單格靜態睡姿**，靠既有的 Zzz 特效表現睡著。

附圖（兩張）：
1. `sheet-c-source-v1.png`：幼龍造型與這次要沿用的睡姿（右上那格）
2. `../adult/sleep-00.png`：尾巴形狀參考（尖端）

輸出：1024×1024 正方形，單一角色。

```
Image 1 shows the baby purple wyvern. Redraw ONLY its sleeping pose (top-right of image 1) as a single character, at the same size, colors, dark plum outline and pixel-art style: lying on its belly, front view, chin resting on its two front paws, eyes closed as gentle arcs, small "w" mouth, blush, two tiny cream horn bumps, small ears, tiny wings folded on the back, hind legs tucked under the body.

Fix only the tail: it is a dragon tail, not a cat tail. It comes out from behind the right side of the body, lies on the ground curling toward the front, and TAPERS TO A POINTED TIP like the adult's tail in image 2 (image 2 is only a reference for the tail shape; do not copy the adult's long horns or big wings). The tail has a cream underside and is clearly outlined and separate from the paws.

One character centered with empty space around it. No text, no ground, no shadows. Background: one flat solid pure green (#00FF00) everywhere outside the character, no gradient, no texture. Crisp pixel edges.
```
