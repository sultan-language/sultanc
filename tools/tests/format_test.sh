#!/bin/sh
# `sultanc fmt` end to end (docs/en/reference/formatter.md): in place, --check,
# standard input, a directory, and inputs that do not parse, in both surface
# languages.
#
#   SC=<sultanc> tools/tests/format_test.sh
set -eu
root=$(cd "$(dirname "$0")/../.." && pwd)
SC=${SC:-$root/build/sultanc}
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
failures=0
fail() { echo "FAIL: $*" >&2; failures=$((failures + 1)); }

if ! "$SC" fmt --check </dev/null >/dev/null 2>&1; then
    echo "format test: $SC has no fmt command" >&2
    exit 1
fi

mkdir -p "$work/tree/sub"
printf 'def f(  x: int ):int{\nreturn x+1;\n}\n' > "$work/tree/en.sn"
printf 'def f(x: int): int {\n    return x + 1;\n}\n' > "$work/en.expected"
printf 'عرف د(  س: عدد ):عدد{\nأرجع س+1; // تعليق\n}\n' > "$work/tree/sub/ar.sn"
printf 'عرف د(س: عدد): عدد {\n    أرجع س + 1; // تعليق\n}\n' > "$work/ar.expected"
printf 'def f( {\n' > "$work/broken.sn"
# A raw isolate control (U+2067) in a comment: the lexer rejects the file.
printf '// \342\201\247 isolate\ndef f(): int { return 0; }\n' > "$work/isolate.sn"
cp "$work/tree/en.sn" "$work/en.original"
cp "$work/tree/sub/ar.sn" "$work/ar.original"

# --check changes nothing, reports both files and exits 1.
status=0
"$SC" fmt --check --lang=en "$work/tree" > "$work/check.out" 2>&1 || status=$?
[ "$status" -eq 1 ] || fail "--check on unformatted files exits $status, not 1"
[ "$(grep -c 'SULTANC-FMT-UNFORMATTED' "$work/check.out")" -eq 2 ] || fail "--check does not report both files"
cmp -s "$work/tree/en.sn" "$work/en.original" || fail "--check changed en.sn"
cmp -s "$work/tree/sub/ar.sn" "$work/ar.original" || fail "--check changed ar.sn"

# Standard input to standard output, with no path and with `-`.
"$SC" fmt < "$work/tree/en.sn" > "$work/stdin.out" || fail "fmt from standard input fails"
cmp -s "$work/stdin.out" "$work/en.expected" || fail "fmt from standard input gives other text"
"$SC" fmt - < "$work/tree/sub/ar.sn" > "$work/stdin-dash.out" || fail "fmt - fails"
cmp -s "$work/stdin-dash.out" "$work/ar.expected" || fail "fmt - gives other text"

# A directory in place, then --check passes and a second run changes nothing.
"$SC" fmt "$work/tree" || fail "fmt on a directory fails"
cmp -s "$work/tree/en.sn" "$work/en.expected" || fail "en.sn is not formatted in place"
cmp -s "$work/tree/sub/ar.sn" "$work/ar.expected" || fail "ar.sn is not formatted in place"
"$SC" fmt --check "$work/tree" || fail "--check after formatting does not pass"
[ -z "$(ls "$work/tree" | grep fmt-tmp || true)" ] || fail "a temporary file is left behind"

# Inputs that do not parse: the existing diagnostics, status 1, no output, file unchanged.
for bad in broken isolate; do
    cp "$work/$bad.sn" "$work/$bad.original"
    status=0
    "$SC" fmt --lang=en "$work/$bad.sn" > "$work/$bad.out" 2>&1 || status=$?
    [ "$status" -eq 1 ] || fail "$bad.sn exits $status, not 1"
    grep -q 'SULTANC-\(PARSE\|LEX\)-' "$work/$bad.out" || fail "$bad.sn is not reported with a lexer or parser diagnostic"
    cmp -s "$work/$bad.sn" "$work/$bad.original" || fail "$bad.sn was changed"
    status=0
    "$SC" fmt < "$work/$bad.sn" > "$work/$bad.stdout" 2>/dev/null || status=$?
    [ "$status" -eq 1 ] || fail "$bad.sn from standard input exits $status, not 1"
    [ ! -s "$work/$bad.stdout" ] || fail "$bad.sn from standard input writes partial output"
done

# A missing file.
status=0
"$SC" fmt --lang=en "$work/missing.sn" > "$work/missing.out" 2>&1 || status=$?
[ "$status" -eq 3 ] || fail "a missing file exits $status, not 3"
grep -q 'SULTANC-IO-INPUT-NOT-FOUND' "$work/missing.out" || fail "a missing file is not reported"

if [ "$failures" -ne 0 ]; then
    echo "format test: $failures failures" >&2
    exit 1
fi
echo "format test: OK"
