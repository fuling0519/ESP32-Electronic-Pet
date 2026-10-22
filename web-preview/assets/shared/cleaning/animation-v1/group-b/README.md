# Cleaning animation v1 — Group B

Built-in `image_gen` produced one static shower prop and six shower-water animation frames. Main style references were the approved v4 sponge and v2 blue-white foam. `prompts.json` stores every actual prompt; all five generated source images are retained.

## Outputs

- `shower-sheet-v1.png`: one 1073 × 1023 cell; reference silhouette width 1049, suggested display width 58 px. Handle extends upper-right. The oval spray face is angled down-left.
- `water-sheet-v1.png`: 3 × 2 atlas, six 640 × 640 cells. Reference maximum stable spray width 334, suggested display width 80 px. Frame 1 opens; frames 2–5 loop; frame 6 closes.
- `manifest.json`: shared-renderer descriptors, visual nozzle and water origin anchors.
- `metadata.json`: source paths, SHA-256 hashes, alpha ranges, source cells, translations and anchors.
- `alpha-contact-v1.png`: light/dark true-alpha composite for visual inspection only; output atlases retain original unscaled RGBA.

## Positioning

Attach the water anchor `[400,100]` to the transformed shower nozzle `[350,733]`. Spray falls downward and slightly left. Its approximate central direction is `[-0.29,0.96]`; its average end is around `[-110,350]` source pixels relative to its origin.

The maximum visible water height is about 385 source pixels: at width 80 / reference width 334 the visible height is approximately 92 px. The 640 px cell includes transparent padding and must not be interpreted as the intended visible size. A smaller pet may need the complete effect shifted above the body rather than shrinking the water; final positioning belongs to the shared-scene preview.

## Processing and validation

`prepare-atlases.py` only copies sources, crops, translates by integer offsets, and pads transparently. No source alpha alteration, recolor, stretching, or per-frame resizing was performed. Alpha >= 160 is used only to measure the principal visible bounds. Original RGB in alpha-zero pixels can look like a broad halo in raw tool previews; light/dark alpha compositing confirms it is absent from the actual composite.

The six generated frames have some natural variation in disconnected water endpoints and outer width, while the top origin is aligned. These are separate generated water dashes, not a rigid silhouette; final loop continuity still needs review at the actual playback speed. No browser playback or Web flow integration was performed by this group.
