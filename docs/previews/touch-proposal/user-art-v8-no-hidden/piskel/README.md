# 摸頭愛心 V8：Piskel 編輯檔

四個 .piskel 對應 V8 的四組連續循環 GIF，使用原生 128×64 像素。
原生畫面放大四倍後，與前次提供的 512×256 GIF 每幀像素完全一致。
角色、閉眼表情、愛心、UI 均未變更；由 GIF 轉換，所以只有一個合併圖層。

在 Piskel 的 Import / Browse .piskel files 選取檔案開啟。

## 時間與編輯注意事項

Piskel 專案只有一個全域 FPS，不能直接儲存每幀各自的時長。
因此這份轉換使用 100 FPS（每格 10ms），以重複幀完整保留原 GIF 時間軸，
沒有丟幀或四捨五入。幼鳥、成鳥各 8060 格／80.6 秒；幼龍、成龍各 3100 格／31 秒。
連續相同畫面是原動畫的停留時間，不是新增動作。

Piskel 網頁的 FPS 滑桿上限為 24，而檔案格式／控制器可讀取數值 100。
修改滑桿會改變整體播放速度；編輯器／瀏覽器也可能無法即時播放 100 FPS。
這些檔案以像素編輯與精確時間資料保存為主，觀賞節奏仍請比對原 GIF。
已完成檔案回讀、每一時間格 RGBA 像素及總時長比對；尚未在 Piskel UI 實際匯入驗收。

格式依據（Piskel 官方原始碼）：
- [Serializer](https://github.com/piskelapp/piskel/blob/master/src/js/utils/serialization/Serializer.js)
- [Deserializer](https://github.com/piskelapp/piskel/blob/master/src/js/utils/serialization/Deserializer.js)
- [FrameUtils / layout](https://github.com/piskelapp/piskel/blob/master/src/js/utils/FrameUtils.js)
- [FPS controller](https://github.com/piskelapp/piskel/blob/master/src/js/controller/piskel/PiskelController.js)
- [FPS slider](https://github.com/piskelapp/piskel/blob/master/src/templates/preview.html)

來源 GIF、使用者素材及正式韌體未修改。這次只做格式轉換。
