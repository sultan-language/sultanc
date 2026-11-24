"""Faithful SultanC lexer for source-rewriting tools.

Mirrors compiler/frontend/lexer (lexer.sn, text.sn, numbers.sn) and the
identifier policy (identifiers.sn, identifier_identity.sn): `//` line
comments, nesting «…» strings without escapes, "…" strings and '…'
characters with backslash escapes, ASCII-digit numbers, Arabic/Latin
identifiers, and the operator spellings of token_registry.def.
Keyword classification comes from token_registry.def itself.
"""

from __future__ import annotations

from dataclasses import dataclass
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
REGISTRY = ROOT / "tools/generate/registry/token_registry.def"

TATWEEL = 0x0640
HAMZA_FOLD = {0x0622: 0x0627, 0x0623: 0x0627, 0x0625: 0x0627}

_START_RANGES = (
    (0x0041, 0x005A), (0x0061, 0x007A), (0x0621, 0x063A), (0x0641, 0x064A),
    (0x066E, 0x066F), (0x0671, 0x06D3), (0x06E5, 0x06E6), (0x06EE, 0x06EF),
    (0x06FA, 0x06FC), (0x0750, 0x077F), (0x08A0, 0x08C9),
)
_START_SINGLE = {0x005F, 0x06D5, 0x06FF, TATWEEL}
_CONT_RANGES = (
    (0x0030, 0x0039), (0x064B, 0x065F), (0x0660, 0x0669), (0x06D6, 0x06DC),
    (0x06DF, 0x06E4), (0x06E7, 0x06E8), (0x06EA, 0x06ED), (0x06F0, 0x06F9),
    (0x08D3, 0x08FF),
)
_CONT_SINGLE = {0x0670}


def identifier_start(ch: str) -> bool:
    c = ord(ch)
    return c in _START_SINGLE or any(lo <= c <= hi for lo, hi in _START_RANGES)


def identifier_continue(ch: str) -> bool:
    c = ord(ch)
    if identifier_start(ch) or c in _CONT_SINGLE:
        return True
    return any(lo <= c <= hi for lo, hi in _CONT_RANGES)


def identity(text: str) -> str:
    """Semantic identifier identity: drop tatweel, fold hamza-on-alef forms."""
    return "".join(chr(HAMZA_FOLD.get(ord(ch), ord(ch))) for ch in text if ord(ch) != TATWEEL)


# Non-ASCII separators (lexer.sn طول_فاصل_يونيكود).
_SEPARATORS = {
    " ", "؜", " ", " ", " ", " ", "　", "﻿",
    "‎", "‏",
} | {chr(c) for c in range(0x2000, 0x200B)}


@dataclass(frozen=True)
class Token:
    kind: str  # ident | keyword | string | char | number | operator | index | comment | space | error
    start: int
    end: int
    text: str


def _load_registry() -> tuple[dict[str, str], list[str]]:
    keyword_re = re.compile(r'^SULTANC__KEYWORD__\(([A-Z0-9_]+),\s*"([^"]+)",\s*"([^"]+)"\)$')
    deprecated_re = re.compile(r'^SULTANC__KEYWORD_DEPRECATED__\(([A-Z0-9_]+),\s*"([^"]+)",\s*"([^"]+)"\)$')
    bool_re = re.compile(r'^SULTANC__BOOL_KEYWORD__\("([^"]+)",\s*"([^"]+)",\s*([01])\)$')
    op_re = re.compile(r'^SULTANC__OPERATOR__\(([A-Z0-9_]+),\s*"([^"]+)"\)$')
    keywords: dict[str, str] = {}
    operators: list[str] = []
    for raw in REGISTRY.read_text(encoding="utf-8").splitlines():
        line = raw.strip()
        if m := keyword_re.fullmatch(line):
            keywords[identity(m.group(2))] = m.group(1)
            keywords[identity(m.group(3))] = m.group(1)
        elif m := deprecated_re.fullmatch(line):
            keywords[identity(m.group(2))] = m.group(1)
            keywords[identity(m.group(3))] = m.group(1)
        elif m := bool_re.fullmatch(line):
            keywords[identity(m.group(1))] = "LIT_BOOL"
            keywords[identity(m.group(2))] = "LIT_BOOL"
        elif m := op_re.fullmatch(line):
            operators.append(m.group(2))
    return keywords, operators


KEYWORDS, OPERATORS = _load_registry()


def keyword_identity(text: str) -> str | None:
    return KEYWORDS.get(identity(text))


def tokenize(source: str) -> list[Token]:
    """Every byte of `source` belongs to exactly one token (spaces and comments included)."""
    tokens: list[Token] = []
    n = len(source)
    i = 0
    while i < n:
        ch = source[i]
        start = i
        if ch in " \t\r\n\f\v" or ch in _SEPARATORS:
            while i < n and (source[i] in " \t\r\n\f\v" or source[i] in _SEPARATORS):
                i += 1
            tokens.append(Token("space", start, i, source[start:i]))
            continue
        if source.startswith("//", i):
            while i < n and source[i] != "\n":
                i += 1
            tokens.append(Token("comment", start, i, source[start:i]))
            continue
        if ch == "«":
            depth = 1
            i += 1
            while i < n and depth:
                if source[i] == "«":
                    depth += 1
                elif source[i] == "»":
                    depth -= 1
                i += 1
            tokens.append(Token("string", start, i, source[start:i]))
            continue
        if ch == '"':
            i += 1
            while i < n and source[i] != '"' and source[i] != "\n":
                i += 2 if source[i] == "\\" else 1
            i = min(i + 1, n)
            tokens.append(Token("string", start, i, source[start:i]))
            continue
        if ch == "'":
            i += 1
            while i < n and source[i] != "'" and source[i] != "\n":
                i += 2 if source[i] == "\\" else 1
            i = min(i + 1, n)
            tokens.append(Token("char", start, i, source[start:i]))
            continue
        if ch == "_" and i + 1 < n and source[i + 1].isascii() and source[i + 1].isdigit():
            i += 1
            while i < n and source[i].isascii() and source[i].isdigit():
                i += 1
            tokens.append(Token("index", start, i, source[start:i]))
            continue
        if ch == "_" and not (i + 1 < n and identifier_continue(source[i + 1])):
            tokens.append(Token("operator", start, i + 1, "_"))
            i += 1
            continue
        if ch.isascii() and ch.isdigit():
            i += 1
            while i < n and (
                (source[i].isascii() and (source[i].isalnum() or source[i] == "_"))
                or (source[i] == "." and i + 1 < n and source[i + 1].isascii() and source[i + 1].isdigit())
            ):
                i += 1
            tokens.append(Token("number", start, i, source[start:i]))
            continue
        if identifier_start(ch):
            i += 1
            while i < n and identifier_continue(source[i]):
                i += 1
            text = source[start:i]
            kind = "keyword" if keyword_identity(text) is not None else "ident"
            tokens.append(Token(kind, start, i, text))
            continue
        for op in OPERATORS:
            if source.startswith(op, i):
                i += len(op)
                tokens.append(Token("operator", start, i, op))
                break
        else:
            i += 1
            tokens.append(Token("error", start, i, source[start:i]))
    return tokens


def significant(tokens: list[Token]) -> list[int]:
    """Indexes of tokens the parser sees (no spaces or comments)."""
    return [k for k, t in enumerate(tokens) if t.kind not in ("space", "comment")]
