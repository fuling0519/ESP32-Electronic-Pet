# 2026-10-19 本地整合核對

## 實際執行

- offline-tests.cjs：23項通過。UI案例現在一起載入shared-sprites、shared-view、bird-view、growth-view、egg-view、sad-view與game-keys，使用已載入圖像尺寸替身並實際走新繪圖分支。
- bird-view-tests.cjs 2項、growth-view-tests.cjs 2項、egg-view-tests.cjs 1項、sad-reasons-tests.cjs 2項、game-keys-tests.cjs 2項、care-view-tests.cjs 2項，合計34項通過。
- 所有Web頂層JavaScript與HTML內嵌腳本語法、HTML本地script/img/link引用存在性檢查通過。

## 覆蓋

領養候選失敗重試不換個體、兩種類共用規則、蛋→幼年→成年及重載、需求與健康保存、疾病／死亡／送別終止、紀念冊保存失敗阻擋領養、遊戲取消／結算一次、治療1199／1200ms與恢復邊界、BLE唯讀／舊快照／非法封包、來源切換、傷心緩衝及鍵盤階段。

## 本輪修正

蛋的50倍演示計時遇到長延遲時，原本可能將超出孵化點的時間也以50倍算入幼年。現在快轉上限停在300秒，下一輪恢復正常計時。整合案例直接呼叫UI的間隔回呼，模擬10秒延遲後仍為剛孵化幼年、有效年齡300秒。

## 不能據此宣稱通過

測試使用VM／DOM／Canvas／LocalStorage替身，不是實際瀏覽器截圖、真正關閉程序後的磁碟持久化、手機觸控、音效聽感或BLE實機驗收。瀏覽器中的存檔也未從工具讀取。使用者先前逐項畫面回報另記台帳，不能把本文件當作全部人工驗收。

人工下一步依BROWSER-CHECK-2026-10-19.md，優先完成重新整理與關頁重開、整段照顧及紀念冊、手機窄螢幕。
