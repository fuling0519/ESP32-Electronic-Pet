"""Copy only the files web-preview/index.html needs into a GitHub Pages folder.

Run from any directory: python tools/build_pages.py [output_dir] [--png]   (default: _site)
Starting at index.html, follows quoted .js/.css/image paths in HTML, JS and CSS.
Bare file names built at runtime (e.g. 'assets/pets/wyvern/'+name) are resolved
under any 'assets/.../'+ prefix found in the scanned code; a name that matches
no file, or more than one, stops the build so nothing is silently left out.

PNG art is published as WebP (quality 90, alpha kept exact) with the copied
code pointing at the new names, cutting the phone download about sevenfold;
the PNGs in web-preview stay the source. --png publishes the PNGs unchanged.
Needs Pillow unless --png is given.
"""
from pathlib import Path
import posixpath
import re
import shutil
import sys

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'web-preview'
REFERENCE = re.compile(
    r"""["'`(]((?:\./)?[A-Za-z0-9_\-./]+\.(?:js|css|png|jpe?g|gif|webp|json|svg|mp3|wav|ogg|woff2?))(?:\?[^"'`)]*)?["'`)]""")
PREFIX = re.compile(r"""["'`](assets/[A-Za-z0-9_\-./]+/)["'`]\s*\+""")
# A literal that starts at the extension ('.png') means the name is glued together at
# runtime, which the scan cannot follow; spell the full paths out in the code instead.
GLUED = re.compile(r"""\+\s*["'`]\.(?:png|jpe?g|gif|webp|js|css|json)\b""")
TEXT = ('.html', '.js', '.css')


def collect(source):
    """Return the set of relative paths under source reachable from index.html."""
    found, pending, unresolved, prefixes = set(), ['index.html'], set(), set()
    while pending:
        name = pending.pop()
        if name in found:
            continue
        found.add(name)
        if not name.endswith(TEXT):
            continue
        text = (source / name).read_text(encoding='utf-8', errors='ignore')
        if GLUED.search(text):
            sys.exit(f'{name}: file name built from pieces ending in an extension; write the full paths out')
        prefixes.update(PREFIX.findall(text))
        base = Path(name).parent
        for ref in REFERENCE.findall(text):
            ref = ref.removeprefix('./')
            for candidate in (ref, posixpath.normpath((base / ref).as_posix())):
                if (source / candidate).is_file():
                    pending.append(candidate)
                    break
            else:
                unresolved.add(ref)
    errors = []
    for ref in sorted(unresolved):
        matches = {path.relative_to(source).as_posix()
                   for prefix in prefixes for path in (source / prefix).rglob(ref)
                   if path.is_file() and path.relative_to(source / prefix).as_posix().endswith(ref)}
        if len(matches) == 1:
            found.update(matches)
        else:
            errors.append(f'{ref}: {len(matches)} matches {sorted(matches)}')
    if errors:
        sys.exit('Unresolved references:\n  ' + '\n  '.join(errors))
    return found


def main():
    args = [arg for arg in sys.argv[1:] if not arg.startswith('--')]
    webp = '--png' not in sys.argv[1:]
    out = Path(args[0]) if args else ROOT / '_site'
    files = sorted(collect(SOURCE))
    pngs = [name for name in files if name.endswith('.png')] if webp else []
    if pngs:
        from PIL import Image
        names = sorted({Path(name).name for name in pngs}, key=len, reverse=True)
        rename = re.compile(r"(?<=[/'\"`(])(" + '|'.join(map(re.escape, names)) + r")(?=[?'\"`)])")
    if out.exists():
        shutil.rmtree(out)
    for name in files:
        target = out / name
        target.parent.mkdir(parents=True, exist_ok=True)
        if name in pngs:
            with Image.open(SOURCE / name) as image:
                image.save(target.with_suffix('.webp'), 'WEBP', quality=90, method=6, alpha_quality=100)
        elif pngs and name.endswith(TEXT):
            text = (SOURCE / name).read_text(encoding='utf-8')
            target.write_text(rename.sub(lambda m: m[1][:-4] + '.webp', text), encoding='utf-8', newline='')
        else:
            shutil.copy2(SOURCE / name, target)
    (out / '.nojekyll').touch()
    published = sorted(path.relative_to(out).as_posix() for path in out.rglob('*')
                       if path.is_file() and path.name != '.nojekyll')
    stray = [name for name in published if name.endswith('.png')] if webp else []
    if collect(out) != set(published) or stray:
        sys.exit(f'Published files and references disagree; leftover PNG: {stray}')
    before = sum((SOURCE / name).stat().st_size for name in files)
    after = sum((out / name).stat().st_size for name in published)
    print(f'{len(published)} files, {before / 1e6:.1f} MB source -> {after / 1e6:.1f} MB in {out}')


if __name__ == '__main__':
    main()
