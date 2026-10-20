"""Check local file/directory links in repository Markdown; no network access.

Run from any directory: python tools/check_docs_links.py
URL fragments are retained by documents but this check validates paths only.
"""
from pathlib import Path
import re
import subprocess
from urllib.parse import unquote

ROOT = Path(__file__).resolve().parents[1]


def local_links(text):
    """Yield line number and target for inline links and reference definitions."""
    fence = None
    for number, line in enumerate(text.splitlines(), 1):
        marker = re.match(r"^\s{0,3}(`{3,}|~{3,})", line)
        if marker:
            token = marker[1]
            if fence is None:
                fence = token
            elif token[0] == fence[0] and len(token) >= len(fence):
                fence = None
            continue
        if fence:
            continue
        targets = re.findall(r"\]\(\s*(<[^>]+>|[^\s)]+)", line)
        definition = re.match(r"^\s{0,3}\[[^\]]+\]:\s*(<[^>]+>|\S+)", line)
        if definition:
            targets.append(definition[1])
        for target in targets:
            target = target.strip('<>')
            if target.startswith(('#', '//')) or re.match(r"[a-zA-Z][a-zA-Z0-9+.-]*:", target):
                continue
            path = unquote(target.split('#', 1)[0].split('?', 1)[0])
            if path:
                yield number, path


def main():
    raw = subprocess.check_output(
        ['git', 'ls-files', '-z', '--cached', '--others', '--exclude-standard'], cwd=ROOT
    )
    names = sorted({name.decode('utf-8') for name in raw.split(b'\0') if name})
    missing = []
    documents = checked = 0
    for name in names:
        source = ROOT / name
        if source.suffix.lower() != '.md' or not source.is_file():
            continue
        documents += 1
        for line, target in local_links(source.read_text(encoding='utf-8-sig')):
            checked += 1
            path = ROOT / target.lstrip('/') if target.startswith('/') else source.parent / target
            if not path.exists():
                missing.append(f'{name}:{line}: {target}')
    if missing:
        print('\n'.join(missing))
        print(f'FAIL: {len(missing)} missing paths; {checked} local links in {documents} Markdown files.')
        return 1
    print(f'PASS: {checked} local link paths in {documents} Markdown files (anchors/network not checked).')
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
