[العربية](../../ar/history/byte-literal-proposal.md) | **English**

# Proposal: byte character literals

> **Historical record.** This page documents the project as it was when it was written. For the current
> design, see the [documentation index](../index.md).

Status: **proposal, not implemented**. This needs an explicit language decision. No compiler or Stage0 change has been made.

## Why

SultanC has two related types:

- `char`: a Unicode scalar value, 32-bit unsigned. Written `'a'`.
- `u8`: an unsigned byte.

There is no literal of type `u8` for an ASCII character. Byte-oriented code therefore compares and appends characters as numbers: `b == 0x2F` stands for `/`, and `append(output, 0x3A)` stands for `:`.

A scan of `compiler/` and `library/` (tests excluded) finds 997 single-character hexadecimal byte expressions in printable ASCII range:

- 786 are textual characters in parsers and formatters: TOML, JSON, URI, HTTP, dates, paths, the lexer, base64, CSV, semantic versions. This text is concealed only because no byte literal exists.
- 211 are protocol or binary-format bytes, which stay numeric. Examples: `ar` member headers, ELF/Mach-O/Wasm writers, WASI and Win32 structures, UUID text layout.

Multi-character readable text written as byte codes has already been replaced with text literals through `std.text.from_bytes` (`bytes_from_text`, `nul_terminated_bytes_from_text`, `append_text_bytes`).

## Mature precedent

Rust keeps the two meanings apart with separate literal forms:

- `'a'` has type `char`, a Unicode scalar value.
- `b'a'` has type `u8`, accepts only ASCII or `\xNN` escapes, and has no implicit conversion between the two.

Go writes `'/'` as an untyped constant that converts to `byte` only when the value fits.

The directive excludes an implicit `char` → `u8` conversion. So the Rust-style explicit form is the candidate.

## Candidate grammar

```
byte-char-literal ::= "ب'" byte-char "'"
byte-char         ::= ASCII printable except "'" and "\"
                    | "\n" | "\r" | "\t" | "\\" | "\'" | "\0" | "\x" hex hex
```

- **Prefix.** `ب'/'` (Arabic surface) with the English alias `b'/'`. The canonical spelling is Arabic, as for every other surface form.
- **Typing.** Always `u8`. A non-ASCII character (code point > 0x7F) is a lexical error, not a UTF-8 sequence. `\xNN` admits any byte value.
- **Escapes.** Exactly the subset above. `\u{…}` is not allowed in a byte literal.
- **Compatibility.** Currently `ب` followed directly by `'` is an identifier `ب` followed by a character literal, which is a syntax error. Giving it meaning breaks no valid program. Stage0 (`bootstrap/src/frontend/lexer`) must accept the same token before compiler sources may use it.

## Sites that depend on this decision

The per-file counts come from the classification script used for the report. The largest are:

| Count | File |
|---:|---|
| 120 | compiler/driver/toml.sn |
| 84 | library/std/time/date.sn |
| 52 | library/std/uri/uri.sn |
| 48 | library/std/json/parse.sn |
| 42 | library/std/http/multipart.sn |
| 39 | library/std/http/http1/common.sn |
| 34 | library/std/time/timezone.sn |
| 33 | library/std/json/stringify.sn |
| 26 | library/std/encoding/base64.sn |
| 24 | library/std/chars/ascii.sn |
| 23 | library/std/fs/path.sn |
| 21 | library/std/version/semver.sn |
