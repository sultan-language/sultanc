#!/usr/bin/env bash
set -euo pipefail

# Execute the trusted host-declared prerequisite commands as argv arrays.
# No shell evaluation, OS switch, or additional adapter registration is needed.
contracts="${1:?contract path is required}"
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
host="$("$script_dir/../../scripts/host-matrix.sh" "$contracts" --current)"
source "$script_dir/../../scripts/host-path.sh"
host_paths_init "$host"

steps="$(jq '.bootstrap | length' <<< "$host")"
for ((step_index = 0; step_index < steps; step_index++)); do
  step="$(jq -c --argjson index "$step_index" '.bootstrap[$index]' <<< "$host")"
  executable="$(jq -r '.command' <<< "$step")"
  script="$(jq -r '.script' <<< "$step")"
  mapfile -t arguments < <(jq -r '.arguments[]' <<< "$step")
  mapfile -t environment < <(jq -r '.environment | to_entries[] | "\(.key)=\(.value)"' <<< "$step")
  command -v "$executable" >/dev/null || {
    echo "registered host bootstrap executable is missing: $executable" >&2
    exit 1
  }
  if [[ -n "$script" ]]; then
    [[ -f "$script_dir/$script" ]] || {
      echo "registered host bootstrap script is missing: $script_dir/$script" >&2
      exit 1
    }
    arguments+=("$(host_path_to_native "$script_dir/$script")")
  fi
  env "${environment[@]}" "$executable" "${arguments[@]}"
done
