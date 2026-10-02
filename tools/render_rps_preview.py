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

names = [f"level-up-{i}" for i in range(16)] + [f"baby-level-up-{i}" for i in range(16)]
names += ["status-health-stage-baby", "status-health-stage-adult",
          "exp-progress", "exp-max", "summary-multi-level", "summary-max",
          "status-age-fourth", "max-replay"]
sheet = Image.new("RGB", (4 * 276, ((len(names) + 3) // 4) * 174), "#202020")
draw = ImageDraw.Draw(sheet)
for index, name in enumerate(names):
    frame = ImageOps.invert(Image.open(ROOT / ".pio/rps-preview" / (name + ".pbm")).convert("L"))
    frame.save(ROOT / ".pio/rps-preview" / (name + ".png"))
    x, y = (index % 4) * 276 + 10, (index // 4) * 174 + 8
    draw.text((x, y), name, fill="white")
    sheet.paste(frame.resize((256, 128), Image.Resampling.NEAREST), (x, y + 24))
sheet.save(ROOT / "docs/exp-level-preview.png")

frames = []
for index in range(16):
    frame = ImageOps.invert(Image.open(ROOT / f".pio/rps-preview/level-up-{index}.pbm").convert("L"))
    frames.append(frame.resize((512, 256), Image.Resampling.NEAREST))
frames[0].save(ROOT / "docs/exp-level-up-preview.gif", save_all=True,
               append_images=frames[1:], duration=125, loop=0)

home_names = ("home-exp-0", "home-exp-half", "home-exp-near",
              "home-exp-level-reset", "home-exp-max", "home-exp-egg")
sheet = Image.new("RGB", (3 * 276, 2 * 174), "#202020")
draw = ImageDraw.Draw(sheet)
for index, name in enumerate(home_names):
    frame = ImageOps.invert(Image.open(ROOT / ".pio/rps-preview" / (name + ".pbm")).convert("L"))
    x, y = (index % 3) * 276 + 10, (index // 3) * 174 + 8
    draw.text((x, y), name, fill="white")
    sheet.paste(frame.resize((256, 128), Image.Resampling.NEAREST), (x, y + 24))
sheet.save(ROOT / "docs/exp-home-preview.png")

status_names = ("status-health-stage-baby", "exp-progress",
                "status-health-stage-adult", "exp-max")
sheet = Image.new("RGB", (2 * 276, 2 * 174), "#202020")
draw = ImageDraw.Draw(sheet)
for index, name in enumerate(status_names):
    frame = ImageOps.invert(Image.open(ROOT / ".pio/rps-preview" / (name + ".pbm")).convert("L"))
    x, y = (index % 2) * 276 + 10, (index // 2) * 174 + 8
    draw.text((x, y), name, fill="white")
    sheet.paste(frame.resize((256, 128), Image.Resampling.NEAREST), (x, y + 24))
sheet.save(ROOT / "docs/exp-status-preview.png")
