# 文件導覽與更新方式

核對日期：2026-10-02。開發進度只記在 [主企畫第 10 節](project-plan.md#progress)。

## 平常只需要看這幾份

- 要知道完成了什麼、還缺什麼：看 [主進度台帳](project-plan.md#progress)。各項都有固定編號、實作狀態、驗證範圍與下一步。
- 要開始建置或接線：看 [根 README](../README.md)；Flutter 啟動看 [App README](../esp32_pet_app/README.md)。
- 韌體與 Web 要對接：看 [BLE 共用協定](../esp32_pet_app/PROTOCOL.md)；Web 功能與兩組分工看 [Web 計畫](web-phase-plan.md)。

## 每份文件的責任

- [project-plan.md](project-plan.md)：產品需求＋唯一進度台帳。完成實作或得到測試回報時，只更新第 10 節對應項目；玩法改變才改需求段落。
- [根 README](../README.md)：環境、建置、接線、素材與接手入口。只有操作方式或依賴改變時修改。
- [App README](../esp32_pet_app/README.md)：Flutter 啟動及平台準備，不維護容量或完成清單。
- [PROTOCOL.md](../esp32_pet_app/PROTOCOL.md)：UUID、命令、欄位、分片及相容性要求。兩組修改介面時共同更新；規格要求不代表驗收通過。
- [web-phase-plan.md](web-phase-plan.md)：W0～W8 工作範圍、依賴及驗收條件。進度在主台帳，改需求／分工才改本檔。
- [architecture.md](architecture.md)：原始碼責任與資料流，模組邊界改變時更新。
- [ui-layout-guide.md](ui-layout-guide.md)：OLED 排版規則與已接受的座標基準，版面規格改變時更新；驗收結果回主台帳。
- [dirty-preview.md](dirty-preview.md)：髒污預覽操作與素材產生方法，操作或素材配置改變時更新。
- [farewell-test.md](farewell-test.md)：送別、紀念冊、存檔升級及中斷恢復驗收；含實際 UI 動畫預覽。
- [volume-test.md](volume-test.md)：靜音／有聲音、舊四級偏好相容、實際 UI 預覽、自動測試與上板步驟；進度只在 A5-SOUND。
- [cleaning-test.md](cleaning-test.md)：首頁清潔動畫的自動檢查、實際畫面預覽與實機操作步驟；進度及驗收回報仍在主台帳清潔動畫增量（A5-CARE／A5-VIS）。
- [wyvern-test.md](wyvern-test.md)：小飛龍種類抽選、動畫、存檔相容、固定龍測試版與實際 UI 預覽；實機回報仍在主台帳 A5-PET。
- [storage-recovery.md](storage-recovery.md)：持續存檔失敗的退避、容量提示、限定舊測試資料清理與備份；進度只在主台帳 A6-SAVE。
- [exp-level-plan.md](exp-level-plan.md)：EXP、升級、滿等與畫面規則，以及 1.4.0 驗證證據及上板步驟；進度仍在 A3-EXP。
- [rps-game-plan.md](rps-game-plan.md)：猜拳規則、畫面座標及素材分工；[rps-game-test.md](rps-game-test.md) 保存可重跑的測試方法與本次證據，進度仍在 A5-GAME。
- [memory-notes-plan.md](memory-notes-plan.md)：記憶音符五回合玩法、四方向／回中操作、獎勵、像素座標與上板驗收；進度仍在 A5-GAME。
- [database-erd.md](database-erd.md)：邏輯模型提案，含未來 Session 模型；不能當作 NVS 格式或已部署雲端資料庫。SQL 草案亦只供參考。
- [App 分工入口](../esp32_pet_app/docs/project-plan.md)：導向主台帳、Web 需求及協定，不複製另一套階段狀態。
- [App 實作定位](../esp32_pet_app/docs/implementation-notes.md)：Flutter 程式閱讀指南，程式結構改變才更新。

## 歷史與證據：有需要再讀

- [development-history.md](development-history.md)：從主企畫移出的日期化決策及使用者驗收回報；保留原本適用範圍。
- [INTEGRATION_PROGRESS.md](../INTEGRATION_PROGRESS.md)：2026-09-30 BLE Stage 0～2 的歷史，內文 92.8% 是舊版數字。
- [firmware-capacity-plan.md](firmware-capacity-plan.md)：容量改善前的原定方案；「下一步」是當時計畫，不是最新待辦。
- [firmware-capacity-handoff.md](firmware-capacity-handoff.md)：2026-10-01 容量建置結果與硬體驗收操作。最新驗收結果更新主台帳，歷史量測不改成新數字。

保留原有路徑，避免舊連結失效。新增一份建置報告後，只需將主台帳的證據連結指向新報告；不必在 README、Web 計畫和 App 文件逐份更新容量。

## 一次工作如何收尾

1. 在主台帳找到項目編號，例如 FW-CAP、A6-S 或 W3。
2. 更新「實作／驗證／下一步」。實機回報附日期、版本、操作與未測範圍；build 成功只算 build。
3. 若介面、玩法、建置、架構或 UI 規格有變，才更新上方對應文件。
4. 重大決策或完整量測另留有日期的歷史證據，主台帳連到它。
5. 檢查文件連結；搜尋舊數字／舊名詞時，先區分歷史紀錄與現行規格。

建議驗證紀錄格式：`日期｜版本或工作目錄｜操作／命令｜結果｜未驗證範圍｜證據連結`。如果沒有提交版本，明確寫「未提交工作目錄」，不要只以可重用的韌體版本字串當作唯一來源。

## 本次審核修正

- App README、App 企畫及實作說明仍稱目前容量 92.8%、要遷移 NimBLE：改為引用主台帳與當日報告。
- 架構仍描述史萊姆、無法點選的狀態卡及尚未接上的畫面更新：依目前程式重寫。
- 主企畫把手機連線與未做的感測器混為同一未完成項、把墓碑滿額程式與實機驗收混在一起：拆成獨立實作／驗證狀態。
- petId 被描述為永久不重用、蛋的 species 可空、疾病僅計清醒：依程式修正為存檔內識別、蛋固定 Bird、清醒與一般睡眠皆計時。
- 一般睡眠心情恢復餘數未進入 PetSnapshotV1：明列在 A5-SLEEP，沒有用「睡眠已驗收」涵蓋這個尚未確認的邊界。
- Web 計畫引用不存在的 web-data-structure.md：改為本節分層原則並明示獨立規格尚未建立；選型階段統一為 W0。
- 原生 App 尚未交付：修正為後續方向，避免把它當現成下載替代方案。
- 舊驗收及原生測試紀錄與最新 NimBLE 版分開，沒有新增或推定實機通過。

這次只整理文件；沒有變更韌體或 App 行為，也沒有重新燒錄或執行硬體驗收。
