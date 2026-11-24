#!/usr/bin/env bash
# Runs every iSpeak test (tests/{ar,en}/ispeak) on one target, using the same
# headers the compiler runner reads: اختبار (تشغيل|رفض), أهداف, خروج, تشخيص,
# خرج-قياسي, خرج-تشخيص-عربي, خرج-تشخيص-إنجليزي.
#
# A run test passes on its exit code (each in-process assertion returns its own
# code) and, with `// خرج-قياسي: <text>`, on its exact standard output. A reject
# test passes when `check` fails with the expected code and, with
# `// خرج-تشخيص-عربي: <file>` / `// خرج-تشخيص-إنجليزي: <file>`, when the human
# diagnostic output in that language is byte-for-byte the file beside the test.
# Run from the sultanc root, after a compiler build:
#
#   bash tools/tests/ispeak_test.sh                        # x86_64-linux, all tests
#   bash tools/tests/ispeak_test.sh check_registry         # only files whose path contains this
#   TARGET=arm64-darwin bash tools/tests/ispeak_test.sh    # another target
#   SC=/path/to/sultanc bash tools/tests/ispeak_test.sh    # another compiler binary
#
# LLVM columns run only when `$SC version` lists the llvm backend; otherwise
# they read "skip". Executables run only when TARGET is the host target; for
# another target the test is built as an object file (--emit=obj) and the run
# columns read "-".
#
# Works with macOS bash 3.2; needs no `timeout`.
set -u
SC=${SC:-./build/sultanc}
TARGET=${TARGET:-x86_64-linux}
FILTER=${1:-}
OUT=build/ispeak-tests-$TARGET
mkdir -p "$OUT"
[ -x "$SC" ] || { echo "missing $SC — build the compiler first or set SC"; exit 2; }

case "$(uname -s)-$(uname -m)" in
  Linux-x86_64) HOST=x86_64-linux ;;
  Darwin-arm64) HOST=arm64-darwin ;;
  *) HOST=unknown ;;
esac
if [ "$TARGET" = "$HOST" ]; then EMIT=exe; else EMIT=obj; fi

BACKENDS=native
if "$SC" version 2>/dev/null | grep -qi 'llvm'; then BACKENDS="native llvm"; fi

row() { printf '%-55s %-6s %-6s %-8s %-8s %-8s %-8s\n' "$@"; }
row TEST KIND CHECK NAT_BLD NAT_RUN LLVM_BLD LLVM_RUN | tee "$OUT/summary.txt"

find tests/ar/ispeak tests/en/ispeak -name '*.sn' | sort | grep -- "$FILTER" | while read -r t; do
  n=$(echo "$t" | sed 's|^tests/||; s|/|__|g; s|\.sn$||')
  kind=$(sed -n 's|^// اختبار: ||p' "$t" | head -1)

  if [ "$kind" = "رفض" ]; then
    want=$(sed -n 's|^// تشخيص: ||p' "$t" | head -1)
    if ! "$SC" check --lang=en "$t" > "$OUT/$n.log" 2>&1 && grep -q "\[$want\]" "$OUT/$n.log"; then r=PASS; else r=FAIL; fi
    # Exact diagnostic output, per language, when the test names a file for it.
    for pair in "ar:خرج-تشخيص-عربي" "en:خرج-تشخيص-إنجليزي"; do
      lang=${pair%%:*}; header=${pair#*:}
      file=$(sed -n "s|^// $header: ||p" "$t" | head -1)
      [ -n "$file" ] || continue
      expected="$(dirname "$t")/$file"
      "$SC" --lang "$lang" --color never check "$t" > /dev/null 2> "$OUT/$n.$lang.stderr"
      if [ ! -s "$expected" ] || ! cmp -s "$expected" "$OUT/$n.$lang.stderr"; then
        r=FAIL
        diff "$expected" "$OUT/$n.$lang.stderr" > "$OUT/$n.$lang.diff" 2>&1
      fi
    done
    row "$n" reject "$r" - - - - | tee -a "$OUT/summary.txt"
    continue
  fi

  targets=$(sed -n 's|^// أهداف: ||p' "$t" | head -1)
  if [ -n "$targets" ] && ! echo ",$targets," | grep -q ",$TARGET,"; then
    row "$n" run skip skip skip skip skip | tee -a "$OUT/summary.txt"
    continue
  fi

  want=$(sed -n 's|^// خروج: ||p' "$t" | head -1); want=${want:-0}
  c=FAIL; nb=FAIL; nr=-; lb=skip; lr=skip
  "$SC" check --lang=en "$t" > "$OUT/$n.check.log" 2>&1 && c=PASS
  for be in $BACKENDS; do
    exe="$OUT/$n.$be"
    if "$SC" "$t" --backend=$be --target="$TARGET" --emit=$EMIT -o "$exe" > "$OUT/$n.$be.log" 2>&1; then
      b=PASS
      if [ "$EMIT" = exe ]; then
        "$exe" > "$OUT/$n.$be.run" 2> "$OUT/$n.$be.err"; rc=$?
        if [ $rc -eq "$want" ]; then x=PASS; else x="rc=$rc"; fi
        # With a خرج-قياسي header the exact standard output must match too.
        if [ $x = PASS ] && grep -q '^// خرج-قياسي: ' "$t"; then
          stdout_want=$(sed -n 's|^// خرج-قياسي: ||p' "$t" | head -1)
          [ "$stdout_want" = "<فارغ>" ] && stdout_want=""
          [ "$(cat "$OUT/$n.$be.run")" = "$stdout_want" ] || x=FAIL
        fi
      else x=-; fi
    else b=FAIL; x=-; fi
    rm -f "$exe"
    if [ $be = native ]; then nb=$b; nr=$x; else lb=$b; lr=$x; fi
  done
  row "$n" run "$c" "$nb" "$nr" "$lb" "$lr" | tee -a "$OUT/summary.txt"
done

total=$(($(wc -l < "$OUT/summary.txt") - 1))
bad=$(tail -n +2 "$OUT/summary.txt" | grep -Ec 'FAIL|rc=')
echo
case "$BACKENDS" in *llvm*) ;; *) echo "note: $SC has no llvm backend; LLVM columns skipped" ;; esac
[ "$EMIT" = obj ] && echo "note: $TARGET is not the host ($HOST); built objects only, nothing run"
echo "$TARGET: $((total - bad))/$total tests pass — logs and summary in $OUT/"
[ "$bad" -eq 0 ]
