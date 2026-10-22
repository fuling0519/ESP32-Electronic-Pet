# 正式UI觸摸動畫驗證

2026-10-20建立，2026-10-22配合焊接後校正重新核對並更新來源雜湊；來源是實際UiController／PetIcons／TouchSprite經U8g2渲染的host framebuffer，
不是實體OLED錄影。43個摸頭幀與來源PNG及已確認V9的整個128×64畫面逐像素一致，
八個戳戳姿勢及兩個幼寵復原姿勢與來源原圖一致。HUD／EXP／原點不變。

- [幼鳥5 FPS](bird-baby-happy-512x256.gif)：14個手繪幀。
- [成鳥5 FPS](bird-adult-happy-512x256.gif)：13個手繪幀。
- [幼龍7 FPS](wyvern-baby-happy-512x256.gif)：8個手繪幀。
- [成龍7 FPS](wyvern-adult-happy-512x256.gif)：8個手繪幀。

本目錄PNG為實際UI逐幀輸出；GIF由這些輸出組成。7 FPS在GIF內以140／150ms累積捨入，
連續七輪56格／8秒，與第一階段V9一樣平均7 FPS。韌體使用millis推進幀號，沒有GIF的重複幀。
[validation.json](validation.json)包含來源／實作檔案雜湊與逐角色驗證結果。

重建：先執行`tools/test_rps.ps1`（已包含`test_touch.ps1`），
再用有Pillow的Python執行`tools/verify_touch_frames.py`。
`tools/generate_touch_sprite.py --check`另檢查全部49個打包素材幀。

原生回歸與各日期建置證據見主台帳；MODE 1正式版／Deep Sleep建置已通過，仍有原有的host snprintf截斷警告，
沒有為觸摸修改相關文案或字型。提交前以目前素材及同工作目錄的C++輸出重新核對上述像素／GIF，並更新validation.json的來源雜湊。
使用者曾在部分畫面實機成功觸發摸頭與戳戳；2026-10-22已完成焊接並提供新讀值，MODE 1門檻更新為670／690，完整互動驗收仍未完成。
待補驗新門檻穩定性、四角色及其他流程；詳見[觸摸驗收](../../testing/touch-test.md)。Agent未執行燒錄或清NVS。
