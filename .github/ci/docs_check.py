#!/usr/bin/env python3
"""The documentation check of CI (ci.yml, docs job).

Every relative Markdown link in the repository's .md files must name a file
or directory that exists (links inside fenced code blocks, URLs with a
scheme and same-page anchors are not checked). Argument: the repository root
(default: the current directory). Exit status 1 when a link is broken, with
one ::error annotation per broken link. Standard library only.
"""

import os
import re
import sys
from urllib.parse import unquote

LINK = re.compile(r"\]\(\s*<?([^)\s>]+)>?(?:\s+\"[^\"]*\")?\s*\)")
FENCE = re.compile(r"^(```|~~~).*?^\1", re.S | re.M)
SCHEME = re.compile(r"^[A-Za-z][A-Za-z0-9+.-]*:")
SKIP = {".git", "build", "node_modules"}


def main():
    root = os.path.abspath(sys.argv[1] if len(sys.argv) > 1 else ".")
    checked = 0
    broken = []
    for directory, subdirectories, files in os.walk(root):
        subdirectories[:] = sorted(d for d in subdirectories if d not in SKIP)
        for name in sorted(files):
            if not name.endswith(".md"):
                continue
            path = os.path.join(directory, name)
            with open(path, encoding="utf-8", errors="replace") as f:
                text = FENCE.sub("", f.read())
            for match in LINK.finditer(text):
                target = match.group(1)
                if target.startswith("#") or SCHEME.match(target):
                    continue
                target = unquote(target.split("#", 1)[0].split("?", 1)[0])
                if not target:
                    continue
                checked += 1
                if target.startswith("/"):
                    resolved = os.path.join(root, target.lstrip("/"))
                else:
                    resolved = os.path.join(directory, target)
                if not os.path.exists(os.path.normpath(resolved)):
                    broken.append((os.path.relpath(path, root), match.group(1)))
    for path, target in broken:
        print(f"::error file={path},title=broken link::{target}")
    print(f"docs check: {checked} relative links, {len(broken)} broken")
    return 1 if broken else 0


if __name__ == "__main__":
    sys.exit(main())
