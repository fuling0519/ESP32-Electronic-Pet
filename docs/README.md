# 文件導覽與更新方式

整理日期：2026-10-20；合併後核對：2026-10-21。產品需求與唯一開發進度仍在 [主企畫](project-plan.md)，所有實作、自動驗證及實機驗收狀態只維護於[主台帳](project-plan.md#progress)。

## 從這裡開始

- 要生成／修改 Web 寵物精靈圖、動畫或接入角色素材：先讀 [精靈圖製作規則](web-sprite-workflow.md)，再看目標角色的素材說明。通用流程與小龍專屬設定分開維護。
- 要知道完成了什麼、還缺什麼：看 [主進度台帳](project-plan.md#progress)。各項都有固定編號、實作狀態、驗證範圍與下一步。
- 要開始建置或接線：看 [根 README](../README.md)；Flutter 啟動看 [App README](../esp32_pet_app/README.md)。
- 韌體與 Web 要對接：看 [BLE 共用協定](../esp32_pet_app/PROTOCOL.md)；Web 功能與兩組分工看 [Web 計畫](features/web-phase-plan.md)。
- 閱讀程式：[架構與模組邊界](reference/architecture.md)。
- 修改 OLED 畫面：[排版規則與核准座標](reference/ui-layout-guide.md)。
- 執行驗收：下方「測試與驗收」；自動測試通過不代表實機驗收通過。
- 準備提交：[提交前企畫核對](commit-plan-review.md)。

## 資料夾用途

- [web-sprite-workflow.md](web-sprite-workflow.md)：Web 寵物精靈圖的通用美術、生成、整理、預覽及整合流程，以及獨立的角色專屬設定；相關 agent 任務開始前閱讀。實作與驗收進度仍回主台帳記錄。
- [project-plan.md](project-plan.md)：產品需求＋唯一進度台帳。完成實作或得到測試回報時，只更新第 10 節對應項目；玩法改變才改需求段落。
- [根 README](../README.md)：環境、建置、接線、素材與接手入口。只有操作方式或依賴改變時修改。
- [App README](../esp32_pet_app/README.md)：Flutter 啟動及平台準備，不維護容量或完成清單。
- [PROTOCOL.md](../esp32_pet_app/PROTOCOL.md)：UUID、命令、欄位、分片及相容性要求。兩組修改介面時共同更新；規格要求不代表驗收通過。
- [Web 計畫](features/web-phase-plan.md)：W0～W8 工作範圍、依賴及驗收條件。進度在主台帳，改需求／分工才改本檔。
- [Web 陪玩 BLE 規則](features/web-game-ble-plan.md)：Web 小遊戲經 BLE 由韌體結算、雙端互鎖的設計理由與分工；介面以 PROTOCOL.md「Web 陪玩」節為準。
- [裝置與 Web 養成同步](features/device-sync-ble-plan.md)：照顧、睡眠、送別、領養與紀念冊雙向同步的設計理由、使用者決定 D1～D4、Web 對帳與分階段分工；介面以 PROTOCOL.md「養成同步」節為準。
- [架構](reference/architecture.md)：原始碼責任與資料流，模組邊界改變時更新。
- [觸摸第一階段方案](features/touch-interaction-phase1.md)：保存來源分析與已確認美術；觸摸功能已實作並有部分實機觸發回報，2026-10-22焊接完成後已更新MODE 1校正，完整互動驗收待補，最新結果見主台帳及[觸摸驗收](testing/touch-test.md)。
- [OLED 排版](reference/ui-layout-guide.md)：OLED 排版規則與已接受的座標基準，版面規格改變時更新；驗收結果回主台帳。
- [髒污預覽](guides/dirty-preview.md)：髒污預覽操作與素材產生方法，操作或素材配置改變時更新。
- [送別驗收](testing/farewell-test.md)：送別、紀念冊、存檔升級及中斷恢復驗收；含實際 UI 動畫預覽。
- [聲音設定驗收](testing/volume-test.md)：靜音／有聲音、舊四級偏好相容、實際 UI 預覽、自動測試與上板步驟；進度只在 A5-SOUND。
- [清潔驗收](testing/cleaning-test.md)：首頁清潔動畫的自動檢查、實際畫面預覽與實機操作步驟；進度及驗收回報仍在主台帳清潔動畫增量（A5-CARE／A5-VIS）。
- [小飛龍驗收](testing/wyvern-test.md)：小飛龍種類抽選、動畫、存檔相容、固定龍測試版與實際 UI 預覽；實機回報仍在主台帳 A5-PET。
- [存檔修復](guides/storage-recovery.md)：持續存檔失敗的退避、容量提示、限定舊測試資料清理與備份；進度只在主台帳 A6-SAVE。
- [經驗與升級](features/exp-level-plan.md)：EXP、升級、滿等與畫面規則，以及 1.4.0 驗證證據及上板步驟；進度仍在 A3-EXP。
- [猜拳規則](features/rps-game-plan.md)：猜拳規則、畫面座標及素材分工；[猜拳驗收](testing/rps-game-test.md) 保存可重跑的測試方法與本次證據，進度仍在 A5-GAME。
- [記憶音符](features/memory-notes-plan.md)：記憶音符五回合玩法、四方向／回中操作、獎勵、像素座標與上板驗收；進度仍在 A5-GAME。
- [邏輯 ERD](reference/database-erd.md)：邏輯模型提案，含未來 Session 模型；不能當作 NVS 格式或已部署雲端資料庫。SQL 草案亦只供參考。
- [App 分工入口](../esp32_pet_app/docs/project-plan.md)：導向主台帳、Web 需求及協定，不複製另一套階段狀態。
- [App 實作定位](../esp32_pet_app/docs/implementation-notes.md)：Flutter 程式閱讀指南，程式結構改變才更新。
- `reference/`：架構、OLED 排版與邏輯資料模型。修改對應規格時更新。
- `features/`：功能規則、互動設計與分工；完成狀態回主台帳。
- `testing/`：可重跑的自動檢查、燒錄、操作預期與實機驗收邊界。
- `guides/`：素材預覽操作與存檔維護。
- `history/`：日期化決策、過往方案與建置量測；保留當時條件，不改寫成最新數字。
- `previews/`：依功能收納圖片、GIF 與 framebuffer 驗證產物；原始美術仍在 `assets/`。

## 參考規格與功能設計

- [架構](reference/architecture.md)；[OLED 排版](reference/ui-layout-guide.md)。
- [EXP／升級](features/exp-level-plan.md)；[猜拳](features/rps-game-plan.md)；[記憶音符](features/memory-notes-plan.md)。
- [觸摸第一階段方案與保留歷史](features/touch-interaction-phase1.md)：現行美術採 V3 戳戳／V9 摸頭；目前實作與硬體待驗收狀態見[觸摸驗收](testing/touch-test.md)。
- [Web 計畫與分工](features/web-phase-plan.md)。
- [邏輯 ERD](reference/database-erd.md)與 [SQLite schema 草案](reference/database-schema.sql)：規劃參考，並非現行 NVS 格式或已部署雲端資料庫。
- [App 分工入口](../esp32_pet_app/docs/project-plan.md)；[App 程式閱讀指南](../esp32_pet_app/docs/implementation-notes.md)。

## 測試與驗收

- 照顧與狀態：[清潔](testing/cleaning-test.md)、[治療](testing/treatment-test.md)、[心情](testing/mood-test.md)、[傷心提醒](testing/sad-state-test.md)。
- 功能與硬體：[聲音設定](testing/volume-test.md)、[觸摸](testing/touch-test.md)、[深度睡眠](testing/deep-sleep-test.md)。
- 種類與生命週期：[小飛龍](testing/wyvern-test.md)、[送別與紀念冊](testing/farewell-test.md)。
- 小遊戲：[猜拳驗收](testing/rps-game-test.md)；記憶音符與 EXP 的驗收方法各保留在功能文件內。
- 韌體建置：平常只編正式版（`pio run -e esp32dev`）並跑 `tools/test_rps.ps1`。改到測試環境開關會影響的檔案（`platformio.ini`、`src/`／`include/` 的標頭檔，或使用 `PET_*_TEST` 等開關的原始檔）時，提交前用 `tools/build_all.ps1` 同時編 4 個環境、一次看完全部 18 個結果；加 `-Auto` 會依 git 變更自動決定要全編還是只編正式版。

## 操作、歷史與預覽

- [存檔修復與維護](guides/storage-recovery.md)；[髒污預覽操作](guides/dirty-preview.md)。
- [開發與驗收歷史](history/development-history.md)。
- [容量改善原方案](history/firmware-capacity-plan.md)；[2026-10-01 容量交接與建置證據](history/firmware-capacity-handoff.md)。其中「下一步／待驗收」為當日狀態，最新結果只看主台帳。
- [BLE Stage 0～2 歷史](../INTEGRATION_PROGRESS.md)：2026-09-30 紀錄，92.8% 是舊版量測。
- 預覽入口：[畫面與產物索引](previews/README.md)。真實程式 framebuffer、設計提案與實體 OLED 驗收分開看待。

## 文件維護規則

1. 功能實作或取得測試回報時，更新主台帳的實作／驗證／下一步；玩法、架構或介面改變時也核對主企畫相關段落。
2. 專項文件保存詳細規則與操作方法，不複製另一套進度清單。BLE 規格只維護於共用協定。
3. 量測另留有日期的證據，主台帳連到它；實機回報附日期、來源版本、操作與未測範圍。
4. 新圖片依功能放入 `previews/`，工具輸出也使用相同路徑，不再寫到 `docs/` 根目錄。
5. 搬移檔案時同步更新文件連結與產生／驗證工具；執行 `python tools/check_docs_links.py` 檢查本機 Markdown 連結。

驗證紀錄格式：`日期｜版本或工作目錄｜操作／命令｜結果｜未驗證範圍｜證據連結`。尚未提交時明寫「未提交工作目錄」，不只依韌體版本字串辨識來源。

本次保留 `project-plan.md` 與 `commit-plan-review.md` 固定路徑。移除 22 張舊聲音圖片、觸摸 V5～V8 的 1,333 個歷史產物及七支退役工具；保留 V3、V4、V9 與現行 framebuffer 比對基準。歷史檔案可由 Git 查閱；整理不代表新增功能驗收。
