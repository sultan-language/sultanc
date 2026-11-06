#!/usr/bin/env bash
set -euo pipefail

# Zig-style wasm-seed bootstrap with every C toolchain driver the host
# declares (ci_hosts[].seed). It needs only a C compiler.
#
#   verify-check-seed  hosts where Stage2 runs (SultanC Native backend):
#                      the driver's own --verify --check-seed (bootstrap.c) or
#                      sultanc-verify/sultanc-check-seed targets (CMake).
#   stage1-reproduce   every other host: Stage1 (the translated seed) is
#                      built, must be a native executable of this host, and
#                      the primary driver's Stage1 must rebuild the seed module
#                      byte-for-byte (same command and module-source pin as
#                      bootstrap.c --check-seed). Together with the required
#                      --check-seed on the native hosts this proves the
#                      translated compiler is deterministic on this host.
#
# The primary (first, required) driver's Stage1 is copied to
# <output_root>/stage1<suffix> for the self-check and portable tests.
#
#   seed-bootstrap.sh CANDIDATE_DIR

fail() {
  echo "$*" >&2
  exit 1
}

[[ $# -eq 1 ]] || { echo "usage: seed-bootstrap.sh CANDIDATE_DIR" >&2; exit 2; }
candidate_dir="$(cd -- "$1" && pwd)"
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
contracts="$script_dir/../ci/contracts.json"
host="$("$script_dir/host-matrix.sh" "$contracts" --current)"
summary="${GITHUB_STEP_SUMMARY:-/dev/null}"

mode="$(jq -r '.seed.mode' <<< "$host")"
suffix="$(jq -r '.executable_suffix' <<< "$host")"
file_pattern="$(jq -r '.file_pattern' <<< "$host")"
seed_file="$(jq -r '.seed_bootstrap.seed' "$contracts")"
seed_sha_file="$(jq -r '.seed_bootstrap.seed_sha256_file' "$contracts")"
seed_zst_sha_file="$(jq -r '.seed_bootstrap.seed_zst_sha256_file' "$contracts")"
driver_source="$(jq -r '.seed_bootstrap.driver_source' "$contracts")"
cmake_source="$(jq -r '.seed_bootstrap.cmake_source' "$contracts")"
output_root="$(jq -r '.seed_bootstrap.output_root' "$contracts")"
mapfile -t reproduce_arguments < <(jq -r '.seed_bootstrap.reproduce_arguments[]' "$contracts")
diag="${RUNNER_TEMP:-/tmp}/sultanc-ci/seed"
rm -rf "$diag"
mkdir -p "$diag"

cd -- "$candidate_dir"
[[ -f "$seed_file" && -f "$seed_sha_file" && -f "$seed_zst_sha_file" && -f "$driver_source" ]] || fail \
  "wasm seed bootstrap sources are missing from the candidate"
expected="$(awk 'NR == 1 { print $1 }' "$seed_sha_file")"
[[ "$expected" =~ ^[0-9a-f]{64}$ ]] || fail "$seed_sha_file does not hold a SHA-256"
# The compressed seed is pinned by its own SHA-256, which the translator checks
# before decompressing. Check it here too, independently, with the host's tool.
expected_zst="$(awk 'NR == 1 { print $1 }' "$seed_zst_sha_file")"
[[ "$expected_zst" =~ ^[0-9a-f]{64}$ ]] || fail "$seed_zst_sha_file does not hold a SHA-256"
if command -v sha256sum >/dev/null 2>&1; then
  actual_zst="$(sha256sum -- "$seed_file" | awk '{print $1}')"
else
  actual_zst="$(shasum -a 256 -- "$seed_file" | awk '{print $1}')"
fi
[[ "$actual_zst" == "$expected_zst" ]] || fail \
  "$seed_file has SHA-256 $actual_zst, but $seed_zst_sha_file pins $expected_zst"
rm -rf -- "$output_root"
mkdir -p "$output_root"

# Run one driver (in a subshell: it runs under a pipeline). Every driver
# leaves Stage1 and the translator at <output_root>/<id>/.
run_driver() {
  local index="$1"
  local driver
  local id
  local kind
  local out
  local jobs
  local -a arguments=()

  driver="$(jq -c --argjson i "$index" '.seed.drivers[$i]' <<< "$host")"
  id="$(jq -r '.id' <<< "$driver")"
  kind="$(jq -r '.kind' <<< "$driver")"
  jobs="$(jq -r '.jobs' <<< "$driver")"
  out="$output_root/$id"
  mkdir -p "$out"

  case "$kind" in
    bootstrap.c)
      local cc
      local cflags
      cc="$(jq -r '.cc' <<< "$driver")"
      cflags="$(jq -r '.cflags' <<< "$driver")"
      mapfile -t arguments < <(jq -r '.arguments[]' <<< "$driver")
      command -v "$cc" >/dev/null 2>&1 || { echo "C compiler not found: $cc"; return 1; }
      "$cc" -O2 -o "$out/build-bootstrap$suffix" "$driver_source" || return 1
      "$out/build-bootstrap$suffix" "--cc=$cc" "--cflags=$cflags" "--jobs=$jobs" \
        "--out=$out" "${arguments[@]}" || return $?
      driver_stage1="$out/sultanc-stage1$suffix"
      driver_wasm2c="$out/sultanc-wasm2c$suffix"
      ;;
    cmake)
      local generator
      local c_compiler
      local target
      generator="$(jq -r '.generator' <<< "$driver")"
      c_compiler="$(jq -r '.c_compiler' <<< "$driver")"
      command -v "$c_compiler" >/dev/null 2>&1 || { echo "C compiler not found: $c_compiler"; return 1; }
      cmake -S "$cmake_source" -B "$out" -G "$generator" \
        "-DCMAKE_C_COMPILER=$c_compiler" -DCMAKE_BUILD_TYPE=Release || return 1
      while IFS= read -r target; do
        cmake --build "$out" --parallel "$jobs" --target "$target" || return 1
      done < <(jq -r '.targets[]' <<< "$driver")
      driver_stage1="$out/sultanc-stage1$suffix"
      driver_wasm2c="$out/sultanc-wasm2c$suffix"
      ;;
    *)
      echo "unsupported seed driver kind: $kind"
      return 1
      ;;
  esac

  [[ -x "$driver_stage1" ]] || { echo "Stage1 was not produced: $driver_stage1"; return 1; }
  # Stage1 must be a native executable of this host (not an emulated or
  # cross-built image).
  file -b "$driver_stage1" | tee "$out/stage1-file.txt"
  grep -Eiq -- "$file_pattern" "$out/stage1-file.txt" || {
    echo "Stage1 does not match the registered host format: $file_pattern"
    return 1
  }
  "$driver_stage1" version || { echo "Stage1 does not run on this host"; return 1; }

  if [[ "$mode" == "stage1-reproduce" && "$index" == 0 ]]; then
    local hash
    "$driver_stage1" "${reproduce_arguments[@]}" -o "$out/sultanc.wasm" || {
      echo "Stage1 failed to rebuild the seed module"
      return 1
    }
    hash="$("$driver_wasm2c" --sha256 "$out/sultanc.wasm" | tr -d '\r')"
    if [[ "$hash" != "$expected" ]]; then
      printf 'seed reproduction: Stage1 on this host gives a different module\n  committed %s\n  rebuilt   %s\n' \
        "$expected" "$hash"
      return 3
    fi
    printf 'seed reproduction: Stage1 rebuilt the committed seed exactly (%s)\n' "$hash"
  fi
}

{
  echo "### Wasm-seed bootstrap (\`$mode\`)"
  printf -- '- Committed seed module SHA-256: `%s`\n' "$expected"
  printf -- '- Committed compressed seed SHA-256: `%s` (checked)\n' "$expected_zst"
  echo '| Driver | Gate | Result |'
  echo '|---|---|---|'
} >> "$summary"

count="$(jq '.seed.drivers | length' <<< "$host")"
required_failures=0
for ((index = 0; index < count; index++)); do
  id="$(jq -r --argjson i "$index" '.seed.drivers[$i].id' <<< "$host")"
  status="$(jq -r --argjson i "$index" '.seed.drivers[$i].status' <<< "$host")"
  echo "::group::seed driver $id ($status)"
  set +e
  run_driver "$index" 2>&1 | tee "$diag/$id.log"
  rc="${PIPESTATUS[0]}"
  set -e
  echo "::endgroup::"
  if ((rc == 0)); then
    printf '| `%s` | %s | PASS |\n' "$id" "$status" >> "$summary"
    if ((index == 0)); then
      cp -- "$output_root/$id/sultanc-stage1$suffix" "$output_root/stage1$suffix"
      cp -- "$output_root/$id/sultanc-wasm2c$suffix" "$output_root/sultanc-wasm2c$suffix"
    fi
  elif [[ "$status" == "experimental" ]]; then
    printf '| `%s` | %s | FAIL (exit %s, not gating) |\n' "$id" "$status" "$rc" >> "$summary"
    echo "::warning title=Experimental seed driver failed::$id exited with $rc (see $diag/$id.log)"
  else
    printf '| `%s` | %s | FAIL (exit %s) |\n' "$id" "$status" "$rc" >> "$summary"
    echo "::error title=Seed driver failed::$id exited with $rc"
    required_failures=$((required_failures + 1))
  fi
done

((required_failures == 0)) || fail "$required_failures required seed bootstrap driver(s) failed"
[[ -x "$output_root/stage1$suffix" ]] || fail "primary Stage1 is missing"
