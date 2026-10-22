# Actual built-in imagegen prompts

## Drops initial

Use case: stylized-concept
Asset type: transparent PNG sprite sheet for a soft colorful pixel-art virtual pet bathing animation.
References: image 1 foam-sheet-v2 is only the style and blue-white color reference; image 2 sponge is only the matching soft pixel-art rendering density and upper-left lighting reference. Do NOT draw foam or sponge.
Primary request: exactly FIVE water drop/small splash animation frames, arranged in a regular THREE-COLUMN TWO-ROW grid with equally sized square cells. The final bottom-right sixth cell must be completely transparent/empty. Read order: left to right then top to bottom. One isolated sprite/effect per occupied cell; generous transparent margins. Consistent underlying drawing scale and same central anchor within each cell, fixed view, palette, upper-left light and pixel size. Do not fit/scale each effect separately.
Style: crisp stepped pixel contours, pastel pale-blue body, white-blue upper-left highlights, soft blue-grey minimal outlines and shadow. Simple small-scale game effects, no blur or gradient glow.
Frames: 1 a round water droplet; 2 the same falling droplet slightly elongated vertically, preserving its visual mass; 3 immediately after impact, a small light splash opening to left and right from a shared central impact point; 4 the splash separates into a few small scattered droplets just above and to either side of the same impact point; 5 those fragments diminish, leaving only a few tiny droplets about to vanish. The last three frames share a stable impact center. Keep all sprite shapes within their respective cells.
Constraints: true alpha transparent PNG background. No checkerboard pixels, background, ground, puddle, cast shadow, foam, showerhead, sponge, character, hands, text, labels, numbers, borders, grid lines, extra effects. Preserve transparent space in sixth cell.

## Drops targeted correction

Use case: precise-object-edit
Edit target: supplied water-drop and splash sheet. Keep the exact five sprite shapes, colors, relative sizes, positions, and 3 columns by 2 rows layout with bottom-right blank. Change ONLY the incorrect fuzzy blue ambient halos surrounding each sprite: remove ALL soft fuzzy glow and all mist. These should be plain solid-pixel game sprites with clean pixel contour edges directly touching fully transparent background. Do not retain any low-opacity haze or colored glow anywhere outside the crisp pixel silhouette. Make sprite bodies opaque, with true transparent alpha outside. No new shadow, extra outline, background, grid, text, checkerboard, or extra symbols. All sixth-cell pixels transparent. Output transparent PNG.

## Sparkle

Use case: stylized-concept
Asset type: small 2D pixel-art game animation sprite sheet, flat opaque pixels on transparent alpha.
Style reference: supplied sponge image is reference ONLY for its crisp stepped pixel edges, low saturated pastel coloring, and soft game illustration style. Do not draw sponge. Make a CLEAN star-shaped icon, like an opaque paper-cut pixel symbol. No illumination effects or soft alpha anywhere.
Primary request: one single four-point clean sparkle growing and shrinking over FIVE animation frames. Exactly 3 columns and 2 rows of equal square cells. Sixth bottom-right cell entirely empty and transparent. All five star shapes perfectly centered at exact cell centers. Fixed visual scale, fixed pixel-size and palette. Generous empty margins. Read order: frame 1 small bright diamond dot; frame 2 same center extends to a small four-point cross; frame 3 fully open elegant four-point starburst, vertical ray slightly longer than horizontal; frame 4 returns to small cross; frame 5 returns to tiny diamond dot. Each occupied cell contains exactly ONE connected pixel shape. No satellites, dots around star or additional stars.
Palette: opaque warm ivory center with pastel pale-gold contour, simple two or three flat colors, softly compatible with the reference. Crisp hard stepped edges, no blur, NO GLOW, NO HALO, no bloom, no shadow, no background, no checkerboard, no grid lines, no labels, numbers or text. Truly transparent PNG alpha outside the solid pixel icon.

