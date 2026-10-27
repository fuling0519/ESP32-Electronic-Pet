"""Copy only the files web-preview/index.html needs into a GitHub Pages folder.

Run from any directory: python tools/build_pages.py [output_dir]   (default: _site)
Starting at index.html, follows quoted .js/.css/image paths in HTML, JS and CSS.
Bare file names built at runtime (e.g. 'assets/pets/wyvern/'+name) are resolved
under any 'assets/.../'+ prefix found in the scanned code; a name that matches
no file, or more than one, stops the build so nothing is silently left out.
"""
from pathlib import Path
import re
import shutil
import sys

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / 'web-preview'
REFERENCE = re.compile(
    r"""["'`(]((?:\./)?[A-Za-z0-9_\-./]+\.(?:js|css|png|jpe?g|gif|webp|json|svg|mp3|wav|ogg|woff2?))(?:\?[^"'`)]*)?["'`)]""")
PREFIX = re.compile(r"""["'`](assets/[A-Za-z0-9_\-./]+/)["'`]\s*\+""")


def collect():
    """Return the set of relative paths reachable from index.html."""
    found, pending, unresolved, prefixes = set(), ['index.html'], set(), set()
    while pending:
        name = pending.pop()
        if name in found:
            continue
        found.add(name)
        if not name.endswith(('.html', '.js', '.css')):
            continue
        text = (SOURCE / name).read_text(encoding='utf-8', errors='ignore')
        prefixes.update(PREFIX.findall(text))
        base = Path(name).parent
        for ref in REFERENCE.findall(text):
            ref = ref.removeprefix('./')
            for candidate in (ref, (base / ref).as_posix()):
                if (SOURCE / candidate).is_file():
                    pending.append(candidate)
                    break
            else:
                unresolved.add(ref)
    errors = []
    for ref in sorted(unresolved):
        matches = {path.relative_to(SOURCE).as_posix()
                   for prefix in prefixes for path in (SOURCE / prefix).rglob(ref)
                   if path.is_file() and path.relative_to(SOURCE / prefix).as_posix().endswith(ref)}
        if len(matches) == 1:
            found.update(matches)
        else:
            errors.append(f'{ref}: {len(matches)} matches {sorted(matches)}')
    if errors:
        sys.exit('Unresolved references:\n  ' + '\n  '.join(errors))
    return found


def main():
    out = Path(sys.argv[1]) if len(sys.argv) > 1 else ROOT / '_site'
    files = sorted(collect())
    if out.exists():
        shutil.rmtree(out)
    for name in files:
        target = out / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(SOURCE / name, target)
    (out / '.nojekyll').touch()
    size = sum((SOURCE / name).stat().st_size for name in files)
    print(f'{len(files)} files, {size / 1e6:.1f} MB -> {out}')


if __name__ == '__main__':
    main()
