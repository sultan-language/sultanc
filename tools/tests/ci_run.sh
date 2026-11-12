#!/usr/bin/env bash
# CI entry for the SultanC test suites (run by .github/ci/linux.sh, macos.sh and windows.ps1).
# Builds and runs the SultanC-native runner (tests/runner) and the iSpeak suite
# (tools/tests/ispeak_test.sh), then writes build/tests-result.json:
#   {"executed": N, "passed": P, "failed": F, "skipped": S, "compile_only": C,
#    "target": T, "errors": [...], "suites": {"runner": {...}, "ispeak": {...}}}
#   executed      tests that ran (everything except skipped)
#   passed        executed tests that passed and were checked at run time where
#                 they have a run step (compile-only passes are not counted here)
#   compile_only  executed run tests that were only compiled, because the target
#                 is not the host; they are neither passed nor failed
#   skipped       tests not executed at all (wrong target, opt-in tests)
#   executed = passed + failed + compile_only
# A suite that does not build, exits non-zero, or leaves a missing or
# malformed summary counts as one more executed, failed test and is named in
# "errors" (a malformed summary contributes no other counts).
# Exit status: 0 only when failed == 0.
#
# Run from the repository root after the compiler is built (./build/sultanc,
# build/sultanc.exe on Windows). SC and TARGET override the compiler and the
# target; TARGET defaults to the host's target.
set -u -o pipefail

case "$(uname -s 2>/dev/null)" in
  MINGW*|MSYS*|CYGWIN*|Windows_NT) exe_suffix=.exe ;;
  *) exe_suffix= ;;
esac
SC=${SC:-./build/sultanc$exe_suffix}

host_target() {
  case "$(uname -s 2>/dev/null)-$(uname -m 2>/dev/null)" in
    Linux-x86_64|Linux-amd64) echo x86_64-linux ;;
    Linux-aarch64|Linux-arm64) echo aarch64-linux-gnu ;;
    Darwin-arm64|Darwin-aarch64) echo arm64-darwin ;;
    Darwin-x86_64) echo x86_64-darwin ;;
    MINGW*-x86_64|MSYS*-x86_64|CYGWIN*-x86_64) echo x86_64-windows-msvc ;;
    MINGW*-aarch64|MSYS*-aarch64|MINGW*-arm64|MSYS*-arm64) echo aarch64-windows-msvc ;;
    *) return 1 ;;
  esac
}
if [ -z "${TARGET:-}" ]; then
  TARGET=$(host_target) || {
    echo "ci_run.sh: no default target for $(uname -s)-$(uname -m); set TARGET" >&2
    exit 2
  }
fi
export TARGET

OUT=build/tests-ci
RESULT=build/tests-result.json
mkdir -p "$OUT"
rm -f "$RESULT"
[ -x "$SC" ] || { echo "ci_run.sh: missing compiler $SC (set SC)" >&2; exit 2; }

errors=()
error() { errors+=("$1"); echo "ci_run.sh: $1" >&2; }
is_count() { [[ "$1" =~ ^[0-9]+$ ]]; }

# 1. SultanC-native runner. Its last line is the Arabic summary:
#    اختبارات SultanC: الكلي=N ناجح=P فاشل=F متجاوز=S تجميع-فقط=C
#    ناجح includes the compile-only passes (C <= P).
runner_built=0
runner_rc=-1
r_total=0; r_pass=0; r_fail=0; r_skip=0; r_compile=0; r_error=0
if "$SC" tests/runner/main.sn -o "$OUT/runner$exe_suffix" > "$OUT/runner-build.log" 2>&1; then
  runner_built=1
  SULTANC_BIN="$SC" "$OUT/runner$exe_suffix" > "$OUT/runner.log" 2>&1
  runner_rc=$?
  line=$(grep 'الكلي=' "$OUT/runner.log" | tail -1)
  num() { printf '%s\n' "$line" | sed -n "s/.*$1=\([0-9][0-9]*\).*/\1/p"; }
  t=$(num 'الكلي'); p=$(num 'ناجح'); f=$(num 'فاشل'); s=$(num 'متجاوز'); c=$(num 'تجميع-فقط')
  if [ -z "$line" ]; then
    error "runner: no summary line in $OUT/runner.log (exit $runner_rc)"
    r_error=1
  elif ! { is_count "$t" && is_count "$p" && is_count "$f" && is_count "$s" && is_count "$c"; }; then
    error "runner: malformed summary: $line"
    r_error=1
  elif [ $((p + f + s)) -ne "$t" ] || [ "$c" -gt "$p" ]; then
    error "runner: inconsistent summary (total=$t passed=$p failed=$f skipped=$s compile-only=$c)"
    r_error=1
  else
    r_total=$t; r_pass=$((p - c)); r_fail=$f; r_skip=$s; r_compile=$c
  fi
  if [ "$r_error" -eq 0 ] && [ "$runner_rc" -ne 0 ] && [ "$r_fail" -eq 0 ]; then
    error "runner: exited with $runner_rc but reported no failures"
    r_error=1
  fi
else
  error "runner: tests/runner/main.sn did not build (see $OUT/runner-build.log)"
  r_error=1
fi
r_fail=$((r_fail + r_error))

# 2. iSpeak suite (tests/ar/ispeak, tests/en/ispeak). summary.txt has a header
#    row, then one row per test: NAME KIND CHECK NAT_BLD NAT_RUN LLVM_BLD LLVM_RUN.
#    A row fails when it contains FAIL or rc=; it is skipped when every column
#    after KIND is "skip", and compile-only when it built but nothing ran ("-").
ispeak_rc=-1
i_total=0; i_pass=0; i_fail=0; i_skip=0; i_compile=0; i_error=0
summary="build/ispeak-tests-$TARGET/summary.txt"
rm -f "$summary"
SC="$SC" TARGET="$TARGET" bash tools/tests/ispeak_test.sh > "$OUT/ispeak.log" 2>&1
ispeak_rc=$?
if [ ! -s "$summary" ]; then
  error "ispeak: summary $summary was not produced (exit $ispeak_rc)"
  i_error=1
elif ! head -1 "$summary" | grep -Eq '^TEST +KIND +CHECK +NAT_BLD +NAT_RUN +LLVM_BLD +LLVM_RUN *$'; then
  error "ispeak: $summary has no header row"
  i_error=1
else
  rows=$(tail -n +2 "$summary")
  i_total=$(printf '%s\n' "$rows" | grep -c .)
  bad_rows=$(printf '%s\n' "$rows" | grep . | grep -Evc '^[^ ]+ +(run|reject) +[^ ]+ +[^ ]+ +[^ ]+ +[^ ]+ +[^ ]+ *$')
  if [ "$i_total" -eq 0 ]; then
    error "ispeak: $summary lists no tests"
    i_error=1
  elif [ "$bad_rows" -ne 0 ]; then
    error "ispeak: $summary has $bad_rows malformed rows"
    i_error=1
    i_total=0
  else
    i_fail=$(printf '%s\n' "$rows" | grep -Ec 'FAIL|rc=')
    i_skip=$(printf '%s\n' "$rows" | grep -Ec '^[^ ]+ +run +skip +skip +skip +skip +skip *$')
    i_compile=$(printf '%s\n' "$rows" | grep -Ev 'FAIL|rc=' |
      grep -Ec '^[^ ]+ +run +PASS +PASS +- +[^ ]+ +[^ ]+ *$')
    i_pass=$((i_total - i_fail - i_skip - i_compile))
    if [ "$ispeak_rc" -ne 0 ] && [ "$i_fail" -eq 0 ]; then
      error "ispeak: ispeak_test.sh exited with $ispeak_rc but reported no failures"
      i_error=1
    fi
  fi
fi
i_fail=$((i_fail + i_error))

r_exec=$((r_total - r_skip + r_error))
i_exec=$((i_total - i_skip + i_error))
executed=$((r_exec + i_exec))
passed=$((r_pass + i_pass))
failed=$((r_fail + i_fail))
skipped=$((r_skip + i_skip))
compile_only=$((r_compile + i_compile))

json_string() { printf '"%s"' "$(printf '%s' "$1" | sed 's/\\/\\\\/g; s/"/\\"/g')"; }
errors_json=
for e in ${errors[@]+"${errors[@]}"}; do
  errors_json="${errors_json:+$errors_json, }$(json_string "$e")"
done
cat > "$RESULT" <<EOF
{"executed": $executed, "passed": $passed, "failed": $failed,
 "skipped": $skipped, "compile_only": $compile_only,
 "target": $(json_string "$TARGET"), "errors": [$errors_json],
 "suites": {"runner": {"built": $runner_built, "exit": $runner_rc, "executed": $r_exec, "passed": $r_pass, "failed": $r_fail, "skipped": $r_skip, "compile_only": $r_compile},
            "ispeak": {"exit": $ispeak_rc, "executed": $i_exec, "passed": $i_pass, "failed": $i_fail, "skipped": $i_skip, "compile_only": $i_compile}}}
EOF
cat "$RESULT"
[ "$failed" -eq 0 ]
