#!/usr/bin/env bash
# Interpreter-vs-native differential sweep.
#
# For every program under examples/{ar,en} and every run or differential test
# under tests/{ar,en} (headers `// اختبار: تشغيل` / `تفاضلي`) that targets the
# host, this builds the native executable and also runs the same source with
# `sultanc run`, both from the repository root with the same stdin
# (/dev/null), arguments (none, plus `SWEEP_ARGS` when set) and environment
# (the test's `// بيئة:` assignment, if any). It compares standard output,
# standard error and the exit status, and writes one line per program:
#
#   <verdict>\t<path>\t<native seconds>\t<interpreted seconds>\t<detail>
#
# verdict: same | stdout | stderr | status | interp-timeout | native-timeout
#          | not-a-program (the native build fails, e.g. a module of a
#            multi-file example) | skipped (targets exclude the host)
#
# Usage (from the repository root):
#   SC=build/sultanc bash tools/tests/interp_sweep.sh [paths...]
# Environment: SC (compiler, default build/sultanc), JOBS (parallel jobs,
# default nproc), TIMEOUT (seconds per run, default 120), OUT (work
# directory, default build/interp-sweep). Exit status: 0 when every compared
# program agrees, 1 otherwise. A summary line is printed last:
#   interp-sweep: compared=N same=S differ=D timeout=T not-a-program=P skipped=K
set -u -o pipefail

SC=${SC:-build/sultanc}
JOBS=${JOBS:-$(nproc 2>/dev/null || echo 2)}
TIMEOUT=${TIMEOUT:-120}
OUT=${OUT:-build/interp-sweep}
case "$SC" in /*) ;; *) SC="$PWD/$SC" ;; esac
[ -x "$SC" ] || { echo "interp_sweep.sh: missing compiler $SC (set SC)" >&2; exit 2; }
mkdir -p "$OUT"
export SC TIMEOUT OUT

host_target() {
  case "$(uname -s)-$(uname -m)" in
    Linux-x86_64) echo x86_64-linux ;;
    Linux-aarch64) echo aarch64-linux-gnu ;;
    Darwin-arm64) echo arm64-darwin ;;
    *) echo unknown ;;
  esac
}
HOST=$(host_target)
export HOST

one() {
  local src=$1
  local key
  key=$(printf '%s' "$src" | md5sum | cut -c1-16)
  local d="$OUT/$key"
  mkdir -p "$d"
  local mode=program
  if head -c 4096 "$src" | grep -q '^// اختبار: '; then
    mode=$(head -c 4096 "$src" | sed -n 's|^// اختبار: \([^ ]*\).*|\1|p' | head -1)
    case "$mode" in تشغيل|تفاضلي) ;; *) return 0 ;; esac
    local targets
    targets=$(head -c 4096 "$src" | sed -n 's|^// أهداف: \(.*\)$|\1|p' | head -1)
    if [ -n "$targets" ] && ! printf ',%s,' "$targets" | grep -q ",$HOST,"; then
      printf 'skipped\t%s\t-\t-\ttargets %s\n' "$src" "$targets"
      return 0
    fi
  elif [ "${src#tests/}" != "$src" ]; then
    return 0
  fi
  local envset=()
  local e
  e=$(head -c 4096 "$src" | sed -n 's|^// بيئة: \(.*\)$|\1|p' | head -1)
  [ -n "$e" ] && envset=("$e")
  if ! "$SC" "$src" -o "$d/native" > "$d/build.log" 2>&1; then
    printf 'not-a-program\t%s\t-\t-\tnative build failed\n' "$src"
    return 0
  fi
  local t0 t1 t2 ns is rn ri
  t0=$(date +%s.%N)
  env "${envset[@]}" timeout "$TIMEOUT" "$d/native" ${SWEEP_ARGS:-} < /dev/null > "$d/n.out" 2> "$d/n.err"
  rn=$?
  t1=$(date +%s.%N)
  env "${envset[@]}" timeout "$TIMEOUT" "$SC" run "$src" -- ${SWEEP_ARGS:-} < /dev/null > "$d/i.out" 2> "$d/i.err"
  ri=$?
  t2=$(date +%s.%N)
  ns=$(echo "$t1 - $t0" | bc)
  is=$(echo "$t2 - $t1" | bc)
  local verdict=same detail=""
  if [ "$rn" = 124 ]; then verdict=native-timeout
  elif [ "$ri" = 124 ]; then verdict=interp-timeout
  elif [ "$rn" != "$ri" ]; then verdict=status; detail="native=$rn interp=$ri: $(head -c 300 "$d/i.err" | tr '\n\t' '  ')"
  elif ! cmp -s "$d/n.out" "$d/i.out"; then verdict=stdout; detail=$(diff "$d/n.out" "$d/i.out" | head -4 | tr '\n\t' '  ' | head -c 300)
  elif ! cmp -s "$d/n.err" "$d/i.err"; then verdict=stderr; detail=$(diff "$d/n.err" "$d/i.err" | head -4 | tr '\n\t' '  ' | head -c 300)
  fi
  printf '%s\t%s\t%s\t%s\t%s\n' "$verdict" "$src" "$ns" "$is" "$detail"
}
export -f one

if [ $# -gt 0 ]; then
  printf '%s\n' "$@"
else
  find examples/ar examples/en tests/ar tests/en -name '*.sn' \
    -not -path '*/ispeak/*' | sort
fi | xargs -P "$JOBS" -I{} bash -c 'one "$@"' _ {} | tee "$OUT/results.tsv.tmp"
sort -k2 "$OUT/results.tsv.tmp" > "$OUT/results.tsv"
rm -f "$OUT/results.tsv.tmp"

compared=0 same=0 differ=0 tmo=0 nap=0 skip=0
while IFS=$'\t' read -r v _; do
  case "$v" in
    same) compared=$((compared+1)); same=$((same+1)) ;;
    stdout|stderr|status) compared=$((compared+1)); differ=$((differ+1)) ;;
    interp-timeout|native-timeout) compared=$((compared+1)); tmo=$((tmo+1)) ;;
    not-a-program) nap=$((nap+1)) ;;
    skipped) skip=$((skip+1)) ;;
  esac
done < "$OUT/results.tsv"
echo "interp-sweep: compared=$compared same=$same differ=$differ timeout=$tmo not-a-program=$nap skipped=$skip"
[ "$differ" = 0 ] && [ "$tmo" = 0 ]
