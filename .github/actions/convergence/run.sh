#!/usr/bin/env bash
set -euo pipefail

# Stage2 comes from the bootstrap path. The self-host fixed point begins at
# Stage3, so convergence requires Stage3 and Stage4 to be byte-identical.

fail() {
  echo "$*" >&2
  exit 1
}

sha256_file() {
  local path="$1"

  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$path" | awk '{print $1}'
  else
    shasum -a 256 "$path" | awk '{print $1}'
  fi
}

write_summary() {
  local stage2_sha="$1"
  local stage3_sha="$2"
  local stage4_sha="$3"
  local fixed_point_result="$4"

  [[ -n "${GITHUB_STEP_SUMMARY:-}" ]] || return 0

  {
    echo "### SultanC self-host convergence"
    printf -- '- Target: `%s`\n' "$target"
    printf -- '- Bootstrap-produced Stage2 SHA: `%s`\n' "$stage2_sha"
    printf -- '- Self-host Stage3 SHA: `%s`\n' "$stage3_sha"
    printf -- '- Self-host Stage4 SHA: `%s`\n' "$stage4_sha"
    printf -- '- Fixed point (Stage3 == Stage4): `%s`\n' "$fixed_point_result"
  } >> "$GITHUB_STEP_SUMMARY"
}

working_directory="${SULTANC_WORKING_DIRECTORY:?working directory is required}"
target="${SULTANC_TARGET:?target is required}"
compiler="${SULTANC_COMPILER:?compiler is required}"

cd -- "$working_directory"
[[ -x "$compiler" ]] || fail "Stage2 compiler is not executable: $compiler"

convergence_dir="${RUNNER_TEMP:-${TMPDIR:-/tmp}}/sultanc-ci/convergence/$target"
rm -rf "$convergence_dir"
mkdir -p "$convergence_dir"

stage3="$convergence_dir/stage3"
stage4="$convergence_dir/stage4"
stage2_sha="$(sha256_file "$compiler")"
stage3_sha="unavailable"
stage4_sha="unavailable"

printf 'Bootstrap-produced Stage2 SHA: %s\n' "$stage2_sha"

if ! "$compiler" \
  --target="$target" \
  -o "$stage3" \
  compiler/main.sn \
  >"$convergence_dir/stage3.stdout" \
  2>"$convergence_dir/stage3.stderr"; then
  write_summary "$stage2_sha" "$stage3_sha" "$stage4_sha" \
    "FAIL — Stage2 failed while producing Stage3"
  fail "Stage2 failed while producing Stage3"
fi

if [[ ! -x "$stage3" ]]; then
  write_summary "$stage2_sha" "$stage3_sha" "$stage4_sha" \
    "FAIL — Stage3 compiler is not executable"
  fail "Stage3 compiler is not executable"
fi

stage3_sha="$(sha256_file "$stage3")"
printf 'Self-host Stage3 SHA: %s\n' "$stage3_sha"

if ! "$stage3" \
  --target="$target" \
  -o "$stage4" \
  compiler/main.sn \
  >"$convergence_dir/stage4.stdout" \
  2>"$convergence_dir/stage4.stderr"; then
  write_summary "$stage2_sha" "$stage3_sha" "$stage4_sha" \
    "FAIL — Stage3 failed while producing Stage4"
  fail "Stage3 failed while producing Stage4"
fi

if [[ ! -x "$stage4" ]]; then
  write_summary "$stage2_sha" "$stage3_sha" "$stage4_sha" \
    "FAIL — Stage4 compiler is not executable"
  fail "Stage4 compiler is not executable"
fi

stage4_sha="$(sha256_file "$stage4")"
printf 'Self-host Stage4 SHA: %s\n' "$stage4_sha"

if ! cmp -s "$stage3" "$stage4"; then
  write_summary "$stage2_sha" "$stage3_sha" "$stage4_sha" "FAIL"
  fail "Stage3 and Stage4 differ; self-host fixed point was not reached"
fi

write_summary "$stage2_sha" "$stage3_sha" "$stage4_sha" "PASS"
echo "Self-host fixed point reached: Stage3 == Stage4"
