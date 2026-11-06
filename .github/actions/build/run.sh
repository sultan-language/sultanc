#!/usr/bin/env bash
set -euo pipefail

# Build SultanC through the repository-owned entry point ./build.sh (C Stage0
# -> Stage1 -> Stage2) and retain logs. Host-specific build.sh options, CMake
# cache seeds and environment come from the host record (ci_hosts[].build);
# nothing here branches on an operating-system name.

fail() {
  echo "$*" >&2
  exit 1
}

working_directory="${SULTANC_WORKING_DIRECTORY:?working directory is required}"
stage="${SULTANC_BUILD_STAGE:-stage2}"
target="${SULTANC_TARGET:-}"
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
contracts="$script_dir/../../ci/contracts.json"
host="$("$script_dir/../../scripts/host-matrix.sh" "$contracts" --current)"
suffix="$(jq -r '.executable_suffix' <<< "$host")"

case "$stage" in
  stage0|stage2|verify) ;;
  *) fail "unsupported build stage: $stage" ;;
esac

diag="${RUNNER_TEMP:-/tmp}/sultanc-ci/build/$stage"
mkdir -p "$diag"
cd -- "$working_directory"
[[ -f ./build.sh ]] || fail "build.sh is missing"
chmod +x ./build.sh

mapfile -t build_arguments < <(jq -r '.build.arguments[]' <<< "$host")
mapfile -t build_environment < <(jq -r '.build.environment | to_entries[] | "\(.key)=\(.value)"' <<< "$host")
mapfile -t cache_seeds < <(jq -r '.build.cmake_cache | to_entries[] | "-D\(.key)=\(.value)"' <<< "$host")
if [[ -n "$target" ]]; then
  build_arguments+=("--target=$target")
fi

# Cache entries build.sh has no option for are seeded into build.sh's own build
# tree first (the producer backend is a build.sh option: --backend=native|llvm).
# build.sh's configure keeps every cache entry it does not pass itself.
if ((${#cache_seeds[@]} > 0)); then
  env "${build_environment[@]}" cmake -S cmake -B build/bootstrap/cmake \
    "-DSULTANC_REQUESTED_STAGE:STRING=$stage" "${cache_seeds[@]}" \
    2>&1 | tee "$diag/cmake-seed.log"
fi

stage_argument=()
[[ "$stage" == "stage2" ]] || stage_argument=("$stage")
printf 'build: ./build.sh'
printf ' %q' "${stage_argument[@]}" "${build_arguments[@]}"
printf '\n'
env "${build_environment[@]}" ./build.sh "${stage_argument[@]}" "${build_arguments[@]}" \
  2>&1 | tee "$diag/build.log"

case "$stage" in
  stage0) product="build/bootstrap/sultanc-stage0$suffix" ;;
  *) product="build/sultanc$suffix" ;;
esac
[[ -x "$product" ]] || fail "$product was not produced"
file "$product" | tee "$diag/product-file.txt"

{
  printf '### SultanC build (`%s`)\n' "$stage"
  printf -- '- Command: `./build.sh %s`\n' "${stage_argument[*]:-} ${build_arguments[*]:-}"
  printf -- '- Product: `%s`\n' "$product"
} >> "${GITHUB_STEP_SUMMARY:-/dev/null}"
