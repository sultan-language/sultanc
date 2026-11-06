#!/usr/bin/env bash
# Shared, contract-selected native/shell path conversion. Source from Bash.
# An empty command is an identity conversion. Other converters are installed
# prerequisites of the selected runner, not operating-system dispatch cases.

host_paths_init() {
  local host="${1:?host contract is required}"
  host_path_command="$(jq -r '.path_converter.command' <<< "$host")"
  mapfile -t host_path_to_shell_args < <(jq -r '.path_converter.to_shell_arguments[]' <<< "$host")
  mapfile -t host_path_to_native_args < <(jq -r '.path_converter.to_native_arguments[]' <<< "$host")
  if [[ -n "$host_path_command" ]]; then
    command -v "$host_path_command" >/dev/null 2>&1 || {
      echo "registered host path converter is missing: $host_path_command" >&2
      return 1
    }
  fi
}

host_path_to_shell() {
  if [[ -n "$host_path_command" ]]; then
    "$host_path_command" "${host_path_to_shell_args[@]}" "$1"
  else
    printf '%s\n' "$1"
  fi
}

host_path_to_native() {
  if [[ -n "$host_path_command" ]]; then
    "$host_path_command" "${host_path_to_native_args[@]}" "$1"
  else
    printf '%s\n' "$1"
  fi
}
