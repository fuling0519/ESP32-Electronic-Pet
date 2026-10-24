# 白鳥長眠 v1 提示詞（2026-10-24 草稿）

白鳥目前沒有長眠圖，Web 長眠流程裡是睜眼站著的。這張只用在倒下的那一刻，以及身體化成光點的來源；
靈魂仍由待機圖產生，所以這張不需要配合靈魂。幼鳥與成鳥共用同一張圖，按大小區分，所以只要畫一張。

姿勢與兩隻小龍刻意不同：成龍側躺、幼龍正面攤平，白鳥採「往一側輕輕倒下、靠在地上」。
不畫腳朝天，避免太戲謔；也要跟之後的睡眠圖（蓬起來坐著、頭埋進羽毛）分得出來。

附圖：`white-idle-v2.png`（使用者確認的白鳥基準）。

輸出：1536×1024 橫式，單一角色。

```
The attached image is the character: a round white fluffy bird with big brown eyes, pink blush cheeks, a tiny dark beak, dark grey-black folded wings, a long black-and-white tail, small dark feet, pixel-art style with a dark outline. Draw ONLY this same bird, at the same size and with the same colors, proportions and level of detail as in the image.

Pose: lifeless, peaceful rest. The bird has gently tipped over onto its side and lies on the ground, facing the viewer's left.
- The round body rests on its side, slightly flattened where it touches the ground; the head rests on the ground too, tilted.
- Both eyes calmly closed as short downward arcs, blush still visible, beak slightly open or relaxed. No X eyes, no tears, no sweat.
- The upper wing lies loose and a little open along the body, not folded tightly.
- The tail lies straight out on the ground behind the body, relaxed, not raised.
- The two small feet are tucked loosely in front of the belly, pointing sideways, NOT sticking up into the air.
- Not sitting, not fluffed up, not tucking the head into the feathers like sleeping.

One character centered with empty space around it. No text, no ground line, no shadow, no flowers, no props. Background: one flat solid pure green (#00FF00) everywhere outside the character, no gradient, no texture. Crisp pixel edges.
```

## 若結果不理想的追加指示（擇一貼在後面）

- 身體變形太多、不像同一隻鳥：
  `Keep the body exactly as round and as large as in the attached image; only rotate it onto its side.`
- 看起來像在睡覺：
  `It must read as lying collapsed on the ground, not resting on its belly. The body is clearly on its side.`
- 腳的位置怪：
  `Hide the feet almost completely under the body; at most the tips of the toes show.`

## 接入時

收到圖後由 Claude 整理：去綠底、以 alpha 主輪廓裁切、量兩眼間距對齊待機圖的大小，輸出 `white-dead-sheet-v1.png` 與定位資料，
再讓 `BirdView` 在長眠狀態使用它（展示工具「長眠外觀」、死亡流程與紀念冊都會一起換）。
