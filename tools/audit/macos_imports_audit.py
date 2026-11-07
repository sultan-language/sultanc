#!/usr/bin/env python3
"""Check that every symbol a macOS executable imports exists for its architecture.

dyld refuses to start a binary that imports a symbol libSystem does not export
for the running architecture (for example `_stat$INODE64`, which exists only
on x86_64). This audit catches that without a Mac: it reads the macOS SDK's
text-based stubs (usr/lib/system/*.tbd) and checks each undefined symbol of the
given executables against the exports listed for <arch>-macos.

Usage:
  macos_imports_audit.py --sdk <MacOSX.sdk | dir of .tbd files> --arch arm64|x86_64 BINARY...
On macOS, --sdk defaults to `xcrun --show-sdk-path`. Needs `nm` (macOS) or
`llvm-nm`. Exit status 1 when any import is missing.
"""
import argparse
import glob
import os
import re
import shutil
import subprocess
import sys


def exports(tbd_dir, arch):
    syms = set()
    files = glob.glob(os.path.join(tbd_dir, "*.tbd"))
    if not files:
        files = glob.glob(os.path.join(tbd_dir, "usr/lib/system/*.tbd"))
    for f in files:
        text = open(f, encoding="utf-8", errors="replace").read()
        for block in re.split(r"\n\s*-\s*targets:\s*", text)[1:]:
            m = re.match(r"\[([^\]]*)\]", block)
            if not m:
                continue
            targets = [t.strip() for t in m.group(1).split(",")]
            if not any(t.startswith(arch + "-macos") for t in targets):
                continue
            for sm in re.finditer(r"(?:symbols|weak-symbols):\s*\[([^\]]*)\]", block):
                for s in sm.group(1).split(","):
                    s = s.strip().strip("'")
                    if s:
                        syms.add(s)
    return syms, len(files)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--sdk")
    ap.add_argument("--arch", required=True, choices=["arm64", "x86_64"])
    ap.add_argument("binaries", nargs="+")
    a = ap.parse_args()
    sdk = a.sdk
    if not sdk and shutil.which("xcrun"):
        sdk = subprocess.run(["xcrun", "--show-sdk-path"], capture_output=True, text=True).stdout.strip()
    if not sdk:
        print("macos_imports_audit: pass --sdk", file=sys.stderr)
        return 2
    syms, n = exports(sdk, a.arch)
    if not n:
        print(f"macos_imports_audit: no .tbd files under {sdk}", file=sys.stderr)
        return 2
    nm = shutil.which("llvm-nm") or shutil.which("nm")
    bad = 0
    for b in a.binaries:
        out = subprocess.run([nm, "-u", b], capture_output=True, text=True, check=True).stdout.split()
        missing = [s for s in out if s not in syms and s != "dyld_stub_binder"]
        state = "ok" if not missing else "MISSING " + " ".join(missing)
        print(f"{b}: {len(out)} imports for {a.arch}-macos: {state}")
        bad += len(missing)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
