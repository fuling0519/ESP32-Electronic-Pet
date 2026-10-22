# Cleaning animation v1 — Group A

Generated with built-in `image_gen` on 2026-10-22 after the user approved the sprite prompts. This folder owns only sponge and foam assets; visual approval of these animation frames remains pending.

- `sponge-sheet-v1.png`: 3 horizontal 704×704 cells; fixed center anchor (352,352); 140/120/140 ms initial loop timing. Visible reference width is 602 source pixels. Draw with `60 / 602` scale for every pet age, including babies.
- `foam-sheet-v1.png`: 8 cells, 4 columns × 2 rows, each 512×512; center anchor (256,256). Reference width 391 pixels; suggested full-foam display width 64 px. Play 0→3 for gathering, alternate 3/4 for abundant foam, then 5→7 for rinsing. Hide the overlay after frame 7.
- `manifest.json`: consuming paths are relative to `animation-v1/`.
- `metadata.json`: complete crop, translation, anchor, source and output SHA-256, alpha range and frame bounds. Alpha ≥128 bounds are measurement only; the output alpha has not been thresholded.
- `*-source-v1.png`: unmodified generated originals.
- `*-prompt-v1.txt`: exact prompts submitted to the built-in tool.
- `prepare_assets.py`: repeatable crop/translation/padding only. There is no frame-by-frame fit, source resampling, repainting, recoloring or alpha cleanup.
- `foam-size-contact-v1.png`: approximate 64 px full-foam contact preview on a warm background, not a source asset.

Primary sponge reference: `../../style-study-v4/sponge-sample-v4.png`; supporting silhouette reference: `../../style-study-v4/user-shape-reference.png`. Foam palette/material reference: `../../style-study-v2/foam-sheet-v2.png`.

The sponge preserves the approved cream-yellow, muted-purple outline and pinched waist; its second frame compresses locally and third returns toward frame 1. The foam sequence changes quantity rather than uniformly scaling its entire cluster. Generative drawings still have small pore/bubble-position differences; these are not guaranteed pixel-identical topology between frames.

Both atlases have RGBA alpha with range 0–255 and transparent padding. Source low-alpha colored fringes have been preserved inside the crops; the saturated blue fringe pixels measured on the foam have alpha 1–8 (at most 3.14% opacity), and do not appear as blue outlines at the display-size contact check. The sources retain every original generated pixel. Atlas source scale and native pixels are preserved.

File and static visual checks are complete. Browser playback, assembled action and user animation acceptance are handled by the shared preview/integration work and are not established by these atlas checks.
