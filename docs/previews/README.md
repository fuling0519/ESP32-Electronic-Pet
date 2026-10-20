# 畫面與產物索引

圖片與 GIF 依功能收納；原始美術在 `assets/`。原生 framebuffer 證據不是實體 OLED 錄影，也不能代表觸摸、蜂鳴器、BLE、保存或睡眠已完成實機驗收。

- [清潔畫面](cleaning/cleaning-preview.png)與[清潔驗收](../testing/cleaning-test.md)。
- [治療畫面](treatment/treatment-preview.png)與[治療驗收](../testing/treatment-test.md)。
- [EXP／升級](exp-level/exp-level-preview.png)與[功能規則](../features/exp-level-plan.md)。
- [猜拳](rps/rps-game-preview.png)與[功能規則](../features/rps-game-plan.md)。
- [記憶音符](memory-notes/memory-notes-preview.png)與[功能規則](../features/memory-notes-plan.md)。
- [傷心提醒](sad-state/sad-state-preview.png)與[驗收方法](../testing/sad-state-test.md)。
- [送別與紀念冊](farewell/farewell-preview.png)與[驗收方法](../testing/farewell-test.md)。該目錄的 `*-proposal.png` 含保留的設計與像素比對基準，並非全部可刪的暫存圖。
- 聲音：[核准六狀態提案](sound/sound-focus-proposal.png)、[實際 UI](sound/sound-focus-ui-preview.png)、[操作與驗收](../testing/volume-test.md)。六張單幀 `sound-focus-*-proposal.png` 是回歸比對輸入，即使與實作預覽相同仍需保留。
- 觸摸：[V3 戳戳／V9 摸頭與歷史索引](touch-proposal/README.md)。V4 是 V9 的底圖來源；V5～V8 與 V8 Piskel 已於 2026-10-20 退役。
- 小飛龍：[原始設計](wyvern-proposal/plan.md)、[實際 UI](wyvern-implemented/ui-preview.png)、[驗收方法](../testing/wyvern-test.md)。

舊清潔提案保留於 `cleaning-proposal/`，與目前程式輸出的 `cleaning/` 分開。保留的驗證 JSON 是產生當日快照，其中來源路徑／雜湊不因本次分類而改寫；最新操作入口以上方連結與工具為準。
