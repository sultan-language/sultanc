#!/usr/bin/env bash
set -euo pipefail

# First step of every host job: resolve this runner's registry record, check
# its declared prerequisites, run its declared bootstrap commands (for example
# the Visual Studio developer environment) and export the host facts later
# steps read: SULTANC_EXECUTABLE_SUFFIX and SULTANC_CI_TARGET.
#
#   host-setup.sh CONTRACTS_JSON

contracts="${1:?contract path is required}"
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
host="$("$script_dir/host-matrix.sh" "$contracts" --current)"

missing=()
while IFS= read -r tool; do
  command -v "$tool" >/dev/null 2>&1 || missing+=("$tool")
done < <(jq -r '.prerequisites[]' <<< "$host")
if ((${#missing[@]} > 0)); then
  echo "registered prerequisites missing on $(jq -r '.id' <<< "$host"): ${missing[*]}" >&2
  exit 1
fi

"$script_dir/../actions/llvm-toolchain/bootstrap.sh" "$contracts"

suffix="$(jq -r '.executable_suffix' <<< "$host")"
target="$(jq -r '.target' <<< "$host")"
if [[ -n "${GITHUB_ENV:-}" ]]; then
  printf 'SULTANC_EXECUTABLE_SUFFIX=%s\n' "$suffix" >> "$GITHUB_ENV"
  printf 'SULTANC_CI_TARGET=%s\n' "$target" >> "$GITHUB_ENV"
fi

{
  printf '### CI host `%s` (%s tier)\n' "$(jq -r '.id' <<< "$host")" "$(jq -r '.tier' <<< "$host")"
  printf -- '- Runner: `%s / %s`, SultanC target: `%s`\n' "$RUNNER_OS" "$RUNNER_ARCH" "$target"
  echo '| Check | Gate |'
  echo '|---|---|'
  jq -r '.gates | to_entries[] | select(.value != "off") | "| `\(.key)` | \(.value) |"' <<< "$host"
} >> "${GITHUB_STEP_SUMMARY:-/dev/null}"
