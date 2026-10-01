# App 與 Web 分工入口

文件角色：需求導覽；不另維護完成清單。進度唯一來源為 [主企畫第 10 節](../../docs/project-plan.md#progress)，Web 里程碑需求為 [Web 計畫](../../docs/web-phase-plan.md)。

- ESP32 負責寵物、計時、疾病／死亡與 NVS；App 只顯示快照並標示資料新鮮度。
- App 關閉、失去網路或 BLE 時，ESP32 應可獨立養成。
- BLE 欄位、UUID、分片、device_id 與 Deep Sleep 連線約定只看 [共用協定](../PROTOCOL.md)。
- 先以前景 Chrome Web 驗收；原生平台是後續候選，不宣稱已有可安裝的替代 App。
- Web 帳號、配對、雲端同步與命令依 W0～W7 分階段進行；遠端操作需先通過唯讀實機及授權流程。
- 韌體容量由韌體組負責，Web 組接收量測與介面，不需重新遷移 BLE 底層。

操作及建置看 [App README](../README.md)，實作定位看 [implementation-notes](implementation-notes.md)。
