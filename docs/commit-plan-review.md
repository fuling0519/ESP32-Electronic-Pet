# 每次提交前核對專案企劃書

此專案將 `docs/project-plan.md` 視為產品需求與唯一進度台帳。
核對全文中與本次提交相關的描述，而非只追加一行進度。

## 提交前的五項要求

1. 對照待提交的差異，核對企劃書全文的相關描述，包括功能規則、數值與架構。
2. 更新過時的功能規則、完成狀態、驗證結果與下一步。
3. 明確區分「已實作」「自動測試通過」「實機驗收完成」；未測試／未驗收必須明寫，不能推定完成。
4. 文件仍正確時，記錄核對章節及無須修改的原因，不為了通過檢查刻意修改企劃書。
5. 將必要的文件更新與功能一起暫存、核對並提交，保留其他任務的修改。

## 在 Codex 中使用

平常直接說「請 commit」或「請 commit＋push」。專案根目錄的 `AGENTS.md`
要求代理先完成以上五點，再建立核對紀錄；你不必每次重複提醒。
hook 不會替你授權 commit 或 push，提交仍依照使用者要求。

Codex 設定在 `.codex/hooks.json`，以 `PreToolUse` 攔截提交指令。
首次使用或修改 hook 定義後，需要在 Codex 的 hook 管理介面檢視並信任這個
專案 hook；CLI 可使用 `/hooks`。若目前聊天沒有載入新設定，重開專案聊天後再檢查。
不要使用跳過 hook 信任檢查的選項。

另外已提供 Git `pre-commit` 檢查，啟用方式（每個新 clone 各執行一次）：

```powershell
git config --local core.hooksPath .githooks
```

啟用前應先確認 `git config --get core.hooksPath` 與 `.git/hooks/pre-commit`
沒有其他檢查；有的話先整合，避免覆蓋。此專案本次安裝前已確認沒有現有設定或腳本。
Git 檢查即使尚未信任 Codex hook，也能在實際提交時攔住缺少核對紀錄的提交。

## 手動核對與提交

1. 使用 `git diff --cached` 查看本次內容，對照企劃書相關章節，必要時更新文件。
2. 使用明確的檔案路徑暫存這次功能與文件；不要把無關修改一起提交。
3. 建立 `.plan-review/` 目錄，將 `tools/plan-review-report.example.json` 複製到 `.plan-review/report.json`，
   填寫實際核對結果。範例文字不是核對證據。`plan_status` 為 `updated` 或
   `unchanged`，必須與暫存的企劃書差異一致。
4. 在專案根目錄執行：

```powershell
python tools/plan_review.py record --report .plan-review/report.json
python tools/plan_review.py check
git commit -m "你的提交訊息"
```

`.plan-review/` 是忽略的本機核對紀錄，不會放入 commit。
紀錄綁定完整 Git 暫存內容、HEAD 與企劃書內容。改動暫存內容、提交後再提交，
或企劃書出現未暫存修改，都需要重新核對並建立紀錄。
核對後若只改了未暫存的其他檔案，本次 commit 的內容未變，紀錄仍有效。
請將暫存與提交分成不同指令，讓 Codex 攔截時能看見最後的暫存內容。

## 能保證的範圍

腳本檢查紀錄是否存在、欄位是否齊全、企劃書狀態與暫存差異是否一致，以及
核對後待提交內容是否改變；企劃書文字是否正確仍由代理或人實際審查。
不會自動呼叫付費 API，也不會自動執行韌體測試或把未驗收功能標記完成。
Git hook 是本機工作流程保護，使用者可停用或略過，並非不可繞過的安全邊界。
未安裝 Git hook 的其他 clone 不會自動受到保護。
執行環境需可使用 `git` 與 `python`；本專案的 Windows 環境已確認兩者可用。

測試：`python tools/test_plan_review.py`（使用臨時 Git 倉庫，不改動本專案暫存區）。

Codex 格式依據：[官方 Hooks 文件](https://learn.chatgpt.com/docs/hooks)。
