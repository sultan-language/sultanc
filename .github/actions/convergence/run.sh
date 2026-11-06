#!/usr/bin/env bash
set -euo pipefail

# Self-host fixed point through the repository's own contract:
# `./build.sh verify` builds Stage3 and Stage4 from Stage2 and requires them to
# be byte-identical, plus the cross-backend check cmake/modules/Verify.cmake
# defines for this host and capability. CI does not re-implement the stages.

fail() {
  echo "$*" >&2
  exit 1
}

sha256_file() {
  if command -v sha256sum >/dev/null 2>&1; then
    sha256sum "$1" | awk '{print $1}'
  else
    shasum -a 256 "$1" | awk '{print $1}'
  fi
}

working_directory="${SULTANC_WORKING_DIRECTORY:?working directory is required}"
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"

SULTANC_BUILD_STAGE=verify "$script_dir/../build/run.sh"

cd -- "$working_directory"
verify_dir="build/bootstrap/cmake/bin/verify"
[[ -d "$verify_dir" ]] || fail "./build.sh verify produced no verify directory: $verify_dir"

converged=0
summary="${GITHUB_STEP_SUMMARY:-/dev/null}"
{
  echo "### SultanC self-host fixed point (./build.sh verify)"
  echo '| Product | SHA-256 |'
  echo '|---|---|'
} >> "$summary"
while IFS= read -r product; do
  printf '| `%s` | `%s` |\n' "${product#"$verify_dir"/}" "$(sha256_file "$product")" >> "$summary"
done < <(find "$verify_dir" -maxdepth 1 -type f -name 'sultanc-*' | LC_ALL=C sort)
while IFS= read -r stamp; do
  converged=$((converged + 1))
  printf -- '- `%s`: PASS\n' "$(basename -- "$stamp")" >> "$summary"
done < <(find "$verify_dir" -maxdepth 1 -type f -name '*.converged' | LC_ALL=C sort)

((converged > 0)) || fail "./build.sh verify finished without a convergence stamp in $verify_dir"
echo "Self-host fixed point reached ($converged convergence checks)"
