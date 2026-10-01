"""Build a contact sheet from actual UI framebuffers emitted by test_rps.ps1."""
from pathlib import Path
from PIL import Image, ImageDraw, ImageOps

ROOT = Path(__file__).resolve().parents[1]
NAMES = (
    "ready", "select-scissors", "select-rock", "select-paper",
    "countdown-3", "countdown-2", "countdown-1", "reveal-win",
    "reveal-draw", "reveal-loss", "summary-win-cap", "summary-draw",
    "summary-loss", "replay-return", "replay-again", "blocked-sick",
    "blocked-egg",
)
sheet = Image.new("RGB", (4 * 276, 5 * 174), "#202020")
draw = ImageDraw.Draw(sheet)
for index, name in enumerate(NAMES):
    # PBM 1 means black; the emitter uses 1 for OLED pixels that are lit.
    frame = ImageOps.invert(Image.open(ROOT / ".pio/rps-preview" / (name + ".pbm")).convert("L"))
    frame.save(ROOT / ".pio/rps-preview" / (name + ".png"))
    x, y = (index % 4) * 276 + 10, (index // 4) * 174 + 8
    draw.text((x, y), name, fill="white")
    sheet.paste(frame.resize((256, 128), Image.Resampling.NEAREST), (x, y + 24))
sheet.save(ROOT / "docs/rps-game-preview.png")
