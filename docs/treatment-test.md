# 生病與治療快速測試

新增日期：2026-10-03。同日使用者總括確認本輪實機驗證均無問題，最終節奏為睜眼 1.6 秒／閉眼 1 秒；最新狀態見 [主台帳 A3-TREAT](project-plan.md#progress)。

同日後續 sad 素材已換成使用者更新的低姿勢版本，四格來源像素比對及三環境編譯通過；最新素材建置紀錄 `.pio/sad-update-build.log`。使用者之後已確認本輪實機驗證均無問題。

移除舊治療頁後，三個環境在相同工作目錄編譯通過；最新紀錄為 `.pio/treatment-menu-build.log`，產物位於 `.pio/verification-build/<環境>/firmware.bin`。RAM 均為 50,380 bytes（15.4%）；Flash 正式版 710,517 bytes、成鳥測試版 711,085 bytes、幼鳥測試版 711,149 bytes。此為當時的建置紀錄；後續使用者已總括回報本輪實機通過。

| 環境 | 開機角色 | NVS namespace |
| --- | --- | --- |
| `esp32dev_treatment_test` | 生病成鳥 | `pet-treat-a` |
| `esp32dev_treatment_baby_test` | 生病幼鳥 | `pet-treat-b` |

冷開機／按 RESET 建立新的測試小鳥：Lv1、EXP 0、飽食 60、心情 55、清潔 0，清醒且生病。正式版 `pet-save` 不會被覆寫。死亡與成長時長維持正式設定，沒有 30 秒死亡倒數；幼鳥約一小時後才到成長年齡。深睡喚醒沿用當次寵物，不重新建立，反覆測試請按 RESET。

## 編譯與燒錄

在專案根目錄 PowerShell 執行；`pio` 不在 PATH 時可用完整路徑：

```powershell
$petPio = 'C:\Users\fulin\.platformio\penv\Scripts\platformio.exe'
if (Test-Path '.pio/verification-packages') {
    $env:PLATFORMIO_PACKAGES_DIR = Join-Path (Get-Location) '.pio/verification-packages'
    $env:PLATFORMIO_BUILD_DIR = Join-Path (Get-Location) '.pio/verification-build'
}
& $petPio run -e esp32dev_treatment_test -t upload
```

幼鳥版：

```powershell
& $petPio run -e esp32dev_treatment_baby_test -t upload
```

多塊板子時加上 `--upload-port COM實際編號`。只編譯則拿掉 `-t upload`。

本機原框架目錄曾出現缺檔及寫入權限錯誤，驗證改用 `.pio/verification-packages` 下的框架副本（其他工具沿用原安裝）和獨立的 `.pio/verification-build` 建置目錄。上方條件設定只影響目前 PowerShell；新 checkout 沒有此目錄時使用一般 PlatformIO 路徑。這些目錄被 Git 忽略，不是專案依賴版本變更。

## 操作與預期結果

1. 開機確認角色顯示對應的新 `sad.png`，有生病十字和汙點。
2. 短按 SW 開選單，向下兩次選「治療」，短按執行。
3. 倒藥期間汙點隱藏、小鳥維持 sad。約 1.2 秒倒完藥時健康恢復、生病十字消失，接下來 200ms 空瓶停留仍維持 sad。
4. 約 1.4 秒閃光開始的同一刻換回正常 idle 臉；閃光持續約兩秒。
5. 約 3.4 秒結束後汙點回來、清潔仍為 0；健康正常、EXP 增加 5。動畫中連按／長按不應離開畫面或重複結算。
6. 治癒後再開選單選「治療」：停留在選單，原列顯示「不需治療」約 1.2 秒，再恢復「治療」；只有一般確認音，沒有失敗音或舊治療頁。連按可刷新提示；方向鍵／長按可立即操作。EXP 不應再增加。
7. 按 RESET 重新出現生病小鳥，可再次治療；換另一個測試版重做。

本測試在重啟時重建寵物，不能驗收治癒狀態跨冷開機保存。測完燒回正式版：

```powershell
& $petPio run -e esp32dev -t upload
```

## 韌體工具下載

PlatformIO 已安裝時，依專案設定下載工具，不需逐個手動安裝：

```powershell
& $petPio pkg install -e esp32dev
```

來源為 [PlatformIO Registry](https://registry.platformio.org/)；用法見 [官方 pkg install 文件](https://docs.platformio.org/en/latest/core/userguide/pkg/cmd_install.html)。平台、框架、編譯器與函式庫版本由 `platformio.ini` 指定。
