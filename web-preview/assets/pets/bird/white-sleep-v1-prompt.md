# 白鳥睡眠 v1 提示詞（2026-10-26 草稿）

白鳥目前沒有睡眠圖：一般睡眠時畫的是睜眼站著的 `white-idle-v2.png` 加 Zzz。
姿勢依長眠 v1 時的約定，要跟長眠（側倒在地）分得出來：蓬起來坐低、頭往胸前縮、閉眼。
幼鳥與成鳥共用同一張，按大小區分，所以只要畫一張；呼吸起伏由程式做（以腳底為支點輕微縮放），不需要第二格。

附圖：`white-idle-v2.png`（使用者確認的白鳥基準）。

輸出：1024×1024 正方形，單一角色。

```
The attached image is the character: a round white fluffy bird with big brown eyes, pink blush cheeks, a tiny dark beak, dark grey-black folded wings, a long black-and-white tail, small dark feet, pixel-art style with a dark outline. Draw ONLY this same bird, facing the same direction, at the same size and with the same colors, proportions and level of detail as in the image.

Pose: peacefully asleep, roosting.
- The bird has settled down low: the round body sinks onto its belly so the feet are almost hidden, only the toe tips show under the body.
- The feathers are puffed up a little, so the body looks even rounder and softer, but it is still the same bird and about the same size.
- The head is drawn down into the shoulders, the beak tucked slightly toward the chest.
- Both eyes closed as calm, gently curved lines (curving downward, a sleeping face, not a smile). Pink blush still visible.
- Wings folded snugly against the body, tail lowered and resting, pointing down-back at the same side as in the image.
- Not lying on its side, not collapsed, not fallen over.

One character centered with empty space around it. No text, no Zzz, no moon, no nest, no pillow, no ground line, no shadow, no props. Background: one flat solid pure green (#00FF00) everywhere outside the character, no gradient, no texture. Crisp pixel edges.
```

## 若結果不理想的追加指示（擇一貼在後面）

- 變得太扁、不像同一隻鳥：
  `Keep the body as round and as large as in the attached image; it only sits lower and fluffs up slightly.`
- 頭整個埋掉、看不到臉：
  `The face must stay visible from the front: both closed eyes, the blush and the beak can be seen.`
- 像在笑：
  `The closed eyes are relaxed sleeping lines, curving downward, not happy upward arcs.`

## 接入時

收到圖後由 Claude 整理：去綠底、以 alpha 主輪廓裁切、量身體大小對齊待機圖，輸出 `white-sleep-v1.png` 與定位資料，
再讓 `white-bird-view.js` 在 `sleepMode==='normal'` 時使用它，展示工具的「睡眠」一起換；Zzz 位置依新頭部位置調整。
