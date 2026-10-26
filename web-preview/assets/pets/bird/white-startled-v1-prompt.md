# 白鳥嚇一跳 v1 提示詞（2026-10-26 草稿）

白鳥目前沒有驚嚇圖：戳戳時畫的是待機圖加驚嘆號、上下小彈。成龍／幼龍都有專用驚嚇靜態圖，白鳥補一張對齊。
要的是「被戳到嚇一跳」，不是生氣；驚嘆號由程式畫，圖裡不要有。幼鳥與成鳥共用，只畫一張。

附圖：`white-idle-v2.png`（使用者確認的白鳥基準）。

輸出：1024×1024 正方形，單一角色。

```
The attached image is the character: a round white fluffy bird with big brown eyes, pink blush cheeks, a tiny dark beak, dark grey-black folded wings, a long black-and-white tail, small dark feet, pixel-art style with a dark outline. Draw ONLY this same bird, facing the same direction, at the same size and with the same colors, proportions and level of detail as in the image.

Draw ONE frame: the instant this bird is suddenly poked and startled. Show the surprise with the face, feathers, wing and tail, not by changing the body shape:
- The white feathers puff out all over, so the outline becomes a little spikier and rounder, but the body keeps the same size and round shape.
- The wing pops slightly away from the body, a little open, as if flinching.
- The tail flicks up higher than in the image.
- The body leans back very slightly; both feet stay planted on the same ground line.
- Eyes: the same big brown eyes, opened a little wider, with bright white highlights. Do NOT add white eyeballs, rings or eyebrows.
- Beak: slightly open in a small surprised "o".
- Pink blush still visible.
- The feeling is surprised, NOT angry: no frown, no furrowed brows.

One character centered with generous empty space around it; no part touches the image edge. No text, no exclamation mark, no sweat drop, no motion lines, no ground line, no shadow, no props. Background: one flat solid pure green (#00FF00) everywhere outside the character, no gradient, no texture. Crisp pixel edges.
```

## 若結果不理想的追加指示（擇一貼在後面）

- 身體變大或變形：
  `Keep the body exactly as round and as large as in the attached image; only the feathers fluff slightly.`
- 看起來在生氣：
  `Make it look startled and innocent, like a small bird that just got surprised.`
- 翅膀張太開像要飛走：
  `The wing only lifts a little away from the body; the bird is not flying or flapping.`

## 接入時

收到圖後由 Claude 整理：去綠底、裁切、以腳底和身體寬度對齊待機圖站位（避免戳下去時左右跳動），輸出 `white-startled-v1.png` 與定位資料，
再讓 `white-bird-view.js` 加一個 `startled` 動作，戳戳反應與展示工具「驚嚇／戳碰」一起換；驚嘆號與小彈動保留。
