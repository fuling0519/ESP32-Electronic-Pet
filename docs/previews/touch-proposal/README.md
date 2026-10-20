# 電容觸摸第一階段預覽

目前美術基準：**V9摸頭＋V3戳戳**。正式觸摸輸入、UI反應及音效尚未實作。

- [V9摸頭](user-art-v9-authored-timing/README.md)：使用者重繪角色與愛心時序；小鳥5 FPS、小飛龍7 FPS。2026-10-20使用者確認本版觀感。每個角色有128×64／512×256 GIF及全部原尺寸PNG。
- [V3戳戳分鏡](user-art-v3/shock-storyboard.png)：500ms兩段水平反應；幼鳥／幼龍以shock第二幀復原。
- [整合方案、狀態機與驗收計畫](../../features/touch-interaction-phase1.md)。

## 歷史產物

本目錄根層為第一版草圖，user-art-v2／user-art-v3／user-art-v4 保留歷史產物；它們不代表目前採用的摸頭美術。V3 戳戳仍為現行基準，V4 的四角色 Home framebuffer 仍是 V9 預覽底圖。

2026-10-20 依使用者同意清除已被 V9 取代的 V5～V8 產物（含 V8 Piskel）與專用工具；歷史來源可由第一階段 Git 快照 `0a25af4` 查閱。現行預覽重建使用 `tools/render_touch_authored_timing_preview.py`。

部分舊工具需要當時單幀happy原圖，目前assets/pets已由使用者改成V9多幀圖集。
不要直接對目前素材執行這些舊工具期待重建舊版本；保存的舊產物與當時驗證結果仍可檢視。
V9工具使用保存的V4原生Home framebuffer作底圖，再完整貼入新版圖集，不依賴舊happy原圖。

這些是OLED framebuffer與像素圖集的預覽，不是實機錄影。自動像素／時長檢查、
未加入觸摸功能的基線Build，以及真實感測／OLED／音效驗收分開記錄。
