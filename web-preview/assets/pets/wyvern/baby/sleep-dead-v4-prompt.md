# 幼龍睡覺＋死亡 v4 提示詞：改用正面姿勢（2026-10-23 草稿，方向待使用者確認）

v3（`../adult/generated_images/睡覺與死亡.png`）兩格都不合格：
- 睡覺：眼睛畫成「∩」開心笑眼，又只有一顆頭加兩隻手、看不到身體，像一顆在笑的麻糬，不像睡著。
- 死亡：頭轉了約 60°，五官歪斜難讀；身體雖然變短，但整體仍怪。

觀察：正面站／坐姿（A、B 的 17 格）幾乎都一次就對；趴著、側躺這類橫向姿勢試了四次，每次都在耳朵、角、五官或身體長度上走樣。因此改用生成工具擅長的**正面姿勢**：
- 睡覺：**坐著打瞌睡**。屁股著地、短腿往前伸，頭微微歪向一側，閉眼是往下的弧線（不是開心的 ∩），小翅膀收好。與「生病閉眼」的區別：生病是站著、翅膀鬆垂、眉眼疲倦；睡覺是坐下、頭歪、表情放鬆，花園中另有 Zzz。
- 死亡：**正面整隻攤平趴在地上**。下巴貼地、前爪往兩側攤開、小翅膀和尾巴軟軟平貼地面，閉眼、嘴角微垂。與睡覺（坐著）完全不同，也沿用既有的淡出與靈魂流程。

附圖（一張）：`sheet-a-source-v2.png`

輸出：1536×1024 橫式，一列 2 格。

```
The attached sheet is the only character reference: the baby purple wyvern. Copy its head, face and proportions exactly from the standing poses: small rounded ears with pink insides set low on the sides of the head, two tiny round cream horn bumps, no forehead crest, tiny features set low on the face, big round head, very short bean-shaped body, tiny stubby legs, tiny wings, short tail with a small pointed tip. Same colors, dark plum outline and pixel-art style, same size as in the sheet. Both poses are FRONT VIEW, like the sheet.

Layout: one row of 2 separate poses on one image, each centered in its half with empty space around it. No grid lines, no text, no ground, no shadows.

1. Dozing off while sitting: sitting on its bottom with both short legs stretched forward (soles toward the viewer), body upright, head tilted a little to one side. Eyes closed as gentle DOWNWARD arcs like a sleeping face (not happy upside-down U arcs), small relaxed "w" mouth, blush. Both little arms resting on the belly, tiny wings folded, tail lying on the ground beside it.
2. Lifeless, flopped flat: front view, the whole baby lying flat on its belly on the ground, chin resting on the ground, both front paws splayed loosely out to the sides, tiny wings drooping flat, tail lying limp. Eyes calmly closed as short downward arcs, mouth slightly turned down, relaxed brows, no X eyes. The body stays short and round behind the head.

Background: one flat solid pure green (#00FF00) everywhere outside the characters, no gradient, no texture. Crisp pixel edges.
```
