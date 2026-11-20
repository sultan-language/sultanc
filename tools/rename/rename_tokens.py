#!/usr/bin/env python3
"""Lexer-based rewrite of renamed SultanC spellings (docs/en/reference/arabic-terminology.md).

    python3 tools/rename/rename_tokens.py --wave N [--check] [--segments] PATH...

Each PATH is a file or a directory (searched recursively, in sorted order) for
`.sn` sources and for Markdown files, whose ```sultanc / ```sultan / ```sn and
untagged code blocks are rewritten the same way. The tokens come from
sn_tokens.py, a faithful copy of the compiler's lexer driven by
tools/generate/registry/token_registry.def, so string literals («…», "…"),
character literals and comments are never touched, and a spelling inside a
longer identifier is a different token.

Waves (naming spec, Part A):
  1  keyword بينما -> طالما
  2  contextual Option type اختيار -> اختياري (in type position and as the
     constructor qualifier), constructor اختيار.بعض -> اختياري.موجود;
     اختيار.لا_شيء keeps its constructor
  3  keywords غير_موقع -> طبيعي and غ8/غ16/غ32/غ64 -> ط8/ط16/ط32/ط64;
     with --segments, instead: identifier segments that name these types:
     غير_موقع -> طبيعي (أصغر_غير_موقع -> أصغر_طبيعي), غير_موقعة -> طبيعية,
     لغير_موقع -> لطبيعي, and the widths غ8..غ64 -> ط8..ط64 (ألحق_غ32_كوف ->
     ألحق_ط32_كوف)
  7  (A7) the entry function "the castle" (ال+قلعة) -> الرئيسية: the identifier itself, not a
     field or constructor named so and not a longer identifier. It has no
     deprecated alias; comments and docs that name it are edited by hand.

--check rewrites nothing: it counts the old-spelling tokens that remain and
exits 1 when there is any. A file holding the marker `rename_tokens:
keep-deprecated` (a deprecation fixture) is skipped and listed; with
`keep-deprecated wave N[,M]` only for those waves.
The rewrite is deterministic and idempotent: a second run changes nothing.
"""

from __future__ import annotations

import argparse
from collections import Counter
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent))
from sn_tokens import ROOT, Token, identity, keyword_identity, significant, tokenize  # noqa: E402

KEEP_MARKER = "rename_tokens: keep-deprecated"
SKIP_DIRS = {".git", "build"}
MARKDOWN_FENCE = re.compile(r"^(\s*)```(\S*)\s*$")
MARKDOWN_CODE_TAGS = {"", "sultanc", "sultan", "sn"}

# Keyword renames by wave: old spelling -> new spelling (both are keyword tokens).
KEYWORD_RENAMES: dict[int, dict[str, str]] = {
    1: {"بينما": "طالما"},
    2: {},
    3: {"غير_موقع": "طبيعي", "غ8": "ط8", "غ16": "ط16", "غ32": "ط32", "غ64": "ط64"},
    7: {},
}

# Wave 7 (A7): the program entry function.
# The old name is spelled by code point, so the tree holds no copy of it.
ENTRY_OLD, ENTRY_NEW = "\u0627\u0644\u0642\u0644\u0639\u0629", "الرئيسية"

# Wave 2: the contextual Option type and its "has a value" constructor.
OPTION_OLD, OPTION_NEW = "اختيار", "اختياري"
OPTION_QUALIFIERS = {identity(OPTION_OLD), identity(OPTION_NEW), "option"}
SOME_OLD, SOME_NEW = "بعض", "موجود"

# Wave 3 --segments: the identifier segment pair that names the unsigned type.
# Wave 3 --segments: identifier segments that name the unsigned types.
# Two-segment forms (old pair -> new segment): the type, its feminine
# adjective form, and the form with the preposition ل.
SEGMENT_PAIRS = {
    ("غير", "موقع"): "طبيعي",
    ("غير", "موقعة"): "طبيعية",
    ("لغير", "موقع"): "لطبيعي",
}
# One-segment forms: the machine widths.
SEGMENT_SINGLES = {"غ8": "ط8", "غ16": "ط16", "غ32": "ط32", "غ64": "ط64"}


def keyword_edits(tokens: list[Token], renames: dict[str, str]) -> list[tuple[Token, str]]:
    table = {identity(old): new for old, new in renames.items()}
    edits = []
    for token in tokens:
        if token.kind == "keyword" and identity(token.text) in table:
            edits.append((token, table[identity(token.text)]))
    return edits


# Keywords that can begin a type (token_registry.def identities).
TYPE_START_KEYWORDS = {
    "ANY", "BOOL", "STRING", "CHAR", "INT", "UINT", "FLOAT", "VOID",
    "I8", "I16", "I32", "I64", "U8", "U16", "U32", "U64", "F32", "F64",
    "MUTABLE", "BOX_OPERATOR", "SLICE_OPERATOR", "FUNCTION_TYPE",
}


def _type_start(token: Token) -> bool:
    """Whether a token can begin the type after a contextual `اختيار`. A
    variant declared as `| اختيار بنوع T` or a value in `طابق اختيار مع` is
    followed by a keyword that cannot."""
    if token.kind == "ident":
        return True
    if token.kind == "keyword":
        return keyword_identity(token.text) in TYPE_START_KEYWORDS
    return token.kind == "operator" and token.text in ("&", "*", "(", "[")


def option_edits(tokens: list[Token]) -> list[tuple[Token, str]]:
    """Wave 2. `اختيار` is an ordinary identifier lexically; it is the Option
    type only in a type position or as a constructor qualifier, so only those
    tokens change. A field or constructor of another type named اختيار
    (`نوع_مقنن.اختيار`) and identifiers that merely contain the word
    (`سياق_استعلام_اختيار_خلفية`) are other tokens."""
    sig = significant(tokens)
    edits: list[tuple[Token, str]] = []

    def at(j: int) -> Token | None:
        return tokens[sig[j]] if 0 <= j < len(sig) else None

    for j, index in enumerate(sig):
        token = tokens[index]
        if token.kind != "ident":
            continue
        previous = at(j - 1)
        following = at(j + 1)
        after_dot = previous is not None and previous.kind == "operator" and previous.text == "."
        word = identity(token.text)
        if word == identity(OPTION_OLD) and not after_dot and following is not None:
            if following.kind == "operator" and following.text == ".":
                constructor = at(j + 2)
                if constructor is not None and constructor.kind in ("ident", "keyword"):
                    edits.append((token, OPTION_NEW))
            elif _type_start(following):
                edits.append((token, OPTION_NEW))
        elif word == identity(SOME_OLD) and after_dot:
            qualifier = at(j - 2)
            before = at(j - 3)
            qualifier_after_dot = before is not None and before.kind == "operator" and before.text == "."
            if qualifier is not None and qualifier.kind == "ident" and not qualifier_after_dot \
                    and identity(qualifier.text) in OPTION_QUALIFIERS:
                edits.append((token, SOME_NEW))
    return edits


_PAIRS = {(identity(a), identity(b)): new for (a, b), new in SEGMENT_PAIRS.items()}
_SINGLES = {identity(old): new for old, new in SEGMENT_SINGLES.items()}


def _segment_rename(text: str) -> str | None:
    parts = text.split("_")
    out: list[str] = []
    k = 0
    changed = False
    while k < len(parts):
        pair = (identity(parts[k]), identity(parts[k + 1])) if k + 1 < len(parts) else None
        if pair in _PAIRS:
            out.append(_PAIRS[pair])
            k += 2
            changed = True
        elif identity(parts[k]) in _SINGLES and len(parts) > 1:
            out.append(_SINGLES[identity(parts[k])])
            k += 1
            changed = True
        else:
            out.append(parts[k])
            k += 1
    return "_".join(out) if changed else None


def segment_edits(tokens: list[Token]) -> list[tuple[Token, str]]:
    """Wave 3 --segments: `_`-separated segments that name the unsigned
    types become طبيعي forms (SEGMENT_PAIRS, SEGMENT_SINGLES). The keywords
    themselves are wave 3 proper.
    The old-name alias table is kept: in `مرادف OLD = NEW;`, where NEW is
    OLD renamed, OLD stays (a public std name keeps it for one release)."""
    sig = significant(tokens)
    keep: set[int] = set()
    for j in range(len(sig) - 3):
        alias, name, equals, target = (tokens[sig[j + n]] for n in range(4))
        if alias.kind == "keyword" and keyword_identity(alias.text) == "ALIAS" and name.kind == "ident" \
                and equals.text == "=" and target.kind == "ident" \
                and _segment_rename(name.text) == target.text:
            keep.add(name.start)
    # The qualified form for a constructor: `مرادف T.OLD = T.NEW;`.
    for j in range(len(sig) - 7):
        a, t1, d1, name, equals, t2, d2, target = (tokens[sig[j + n]] for n in range(8))
        if a.kind == "keyword" and keyword_identity(a.text) == "ALIAS" and d1.text == "." and d2.text == "." \
                and equals.text == "=" and name.kind == "ident" and target.kind == "ident" \
                and identity(t1.text) == identity(t2.text) and _segment_rename(name.text) == target.text:
            keep.add(name.start)
    edits = []
    for token in tokens:
        if token.kind != "ident" or token.start in keep:
            continue
        renamed = _segment_rename(token.text)
        if renamed is not None:
            edits.append((token, renamed))
    return edits


def entry_edits(tokens: list[Token]) -> list[tuple[Token, str]]:
    sig = significant(tokens)
    edits = []
    for j, index in enumerate(sig):
        token = tokens[index]
        if token.kind == "ident" and identity(token.text) == identity(ENTRY_OLD):
            previous = tokens[sig[j - 1]] if j else None
            if previous is None or not (previous.kind == "operator" and previous.text == "."):
                edits.append((token, ENTRY_NEW))
    return edits


def edits_for(source: str, wave: int, segments: bool) -> list[tuple[Token, str]]:
    tokens = tokenize(source)
    if segments:
        return segment_edits(tokens)
    edits = keyword_edits(tokens, KEYWORD_RENAMES[wave])
    if wave == 2:
        edits += option_edits(tokens)
    if wave == 7:
        edits += entry_edits(tokens)
    return sorted(edits, key=lambda edit: edit[0].start)


def apply(source: str, edits: list[tuple[Token, str]]) -> str:
    pieces = []
    cursor = 0
    for token, new in edits:
        pieces.append(source[cursor:token.start])
        pieces.append(new)
        cursor = token.end
    pieces.append(source[cursor:])
    return "".join(pieces)


def rewrite_markdown(text: str, wave: int, segments: bool) -> tuple[str, int]:
    """Rewrite only the SultanC code blocks of a Markdown file."""
    lines = text.splitlines(keepends=True)
    out: list[str] = []
    count = 0
    k = 0
    while k < len(lines):
        match = MARKDOWN_FENCE.match(lines[k].rstrip("\n"))
        if not match:
            out.append(lines[k])
            k += 1
            continue
        indent, tag = match.groups()
        close = k + 1
        while close < len(lines) and not re.match(rf"^{re.escape(indent)}```\s*$", lines[close].rstrip("\n")):
            close += 1
        block = "".join(lines[k + 1:close])
        if tag in MARKDOWN_CODE_TAGS:
            edits = edits_for(block, wave, segments)
            block = apply(block, edits)
            count += len(edits)
        out.append(lines[k])
        out.append(block)
        if close < len(lines):
            out.append(lines[close])
        k = close + 1
    return "".join(out), count


def keeps_wave(text: str, wave: int) -> bool:
    """Whether a deprecation fixture keeps this wave's old spellings."""
    at = text.find(KEEP_MARKER)
    if at < 0:
        return False
    rest = text[at + len(KEEP_MARKER):].split("\n", 1)[0]
    match = re.match(r"\s*wave\s+([0-9,\s]+)", rest)
    if not match:
        return True
    return str(wave) in re.split(r"[,\s]+", match.group(1).strip())


def files_under(paths: list[str]) -> list[Path]:
    found: set[Path] = set()
    for raw in paths:
        path = Path(raw)
        if path.is_file():
            found.add(path)
        elif path.is_dir():
            for candidate in path.rglob("*"):
                if candidate.suffix in (".sn", ".md") and candidate.is_file() \
                        and not SKIP_DIRS.intersection(candidate.parts):
                    found.add(candidate)
        else:
            raise SystemExit(f"rename_tokens: no such file or directory: {raw}")
    return sorted(found, key=lambda p: p.as_posix())


def area(path: Path) -> str:
    try:
        relative = path.resolve().relative_to(ROOT)
    except ValueError:
        relative = path
    parts = relative.parts
    if len(parts) >= 2 and parts[0] in ("tests", "examples", "docs", "library"):
        return "/".join(parts[:2])
    return parts[0]


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--wave", type=int, required=True, choices=sorted(KEYWORD_RENAMES))
    parser.add_argument("--check", action="store_true", help="count remaining old spellings; rewrite nothing")
    parser.add_argument("--segments", action="store_true", help="wave 3 only: rename identifier segments")
    parser.add_argument("--quiet", action="store_true", help="print only the per-directory summary")
    parser.add_argument("paths", nargs="+")
    args = parser.parse_args()
    if args.segments and args.wave != 3:
        parser.error("--segments belongs to wave 3")

    per_area: Counter[str] = Counter()
    skipped: list[Path] = []
    total = 0
    for path in files_under(args.paths):
        text = path.read_text(encoding="utf-8")
        if keeps_wave(text, args.wave):
            skipped.append(path)
            continue
        if path.suffix == ".md":
            new_text, count = rewrite_markdown(text, args.wave, args.segments)
        else:
            edits = edits_for(text, args.wave, args.segments)
            count = len(edits)
            new_text = apply(text, edits)
        if count == 0:
            continue
        total += count
        per_area[area(path)] += count
        if not args.quiet:
            print(f"{count:6d}  {path.as_posix()}")
        if not args.check and new_text != text:
            path.write_text(new_text, encoding="utf-8")

    label = "remaining" if args.check else "rewritten"
    kind = f"wave {args.wave}{' segments' if args.segments else ''}"
    print(f"rename_tokens: {kind}: {total} token(s) {label}")
    for name in sorted(per_area):
        print(f"{per_area[name]:8d}  {name}")
    for path in skipped:
        print(f"  skipped (keeps deprecated spellings): {path.as_posix()}")
    return 1 if args.check and total else 0


if __name__ == "__main__":
    raise SystemExit(main())
