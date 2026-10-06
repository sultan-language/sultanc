#!/usr/bin/env bash
set -euo pipefail

# Rebuild the self-hosted compiler twice and require byte-for-byte convergence.

fail() {
  echo "$*" >&2
  exit 1
}

working_directory="${SULTANC_WORKING_DIRECTORY:?working directory is required}"
target="${SULTANC_TARGET:?target is required}"
compiler="${SULTANC_COMPILER:?compiler is required}"

cd -- "$working_directory"
[[ -x "$compiler" ]] || fail "self-host compiler is not executable: $compiler"

convergence_dir="${RUNNER_TEMP:-${TMPDIR:-/tmp}}/sultanc-ci/convergence/$target"
rm -rf "$convergence_dir"
mkdir -p "$convergence_dir"

stage3="$convergence_dir/stage3"
stage4="$convergence_dir/stage4"

"$compiler" \
  --target="$target" \
  -o "$stage3" \
  compiler/main.sn \
  >"$convergence_dir/stage3.stdout" \
  2>"$convergence_dir/stage3.stderr"

"$stage3" \
  --target="$target" \
  -o "$stage4" \
  compiler/main.sn \
  >"$convergence_dir/stage4.stdout" \
  2>"$convergence_dir/stage4.stderr"

[[ -x "$stage3" ]] || fail "Stage3 compiler is not executable"
[[ -x "$stage4" ]] || fail "Stage4 compiler is not executable"

if command -v sha256sum >/dev/null 2>&1; then
  sha256sum "$compiler" "$stage3" "$stage4"
else
  shasum -a 256 "$compiler" "$stage3" "$stage4"
fi

cmp "$compiler" "$stage3"
cmp "$stage3" "$stage4"

{
  echo "### SultanC self-host convergence"
  printf -- '- Target: `%s`\n' "$target"
  echo '- Stage2 = Stage3 = Stage4: `PASS`'
} >> "$GITHUB_STEP_SUMMARY"
