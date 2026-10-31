#!/usr/bin/env bash
set -euo pipefail

# Resolve the one trusted host registry for both LLVM provisioning and native
# qualification. Every host runs the same jobs; only contract metadata varies.

if [[ $# -ne 2 ]]; then
  echo "usage: host-matrix.sh CONTRACTS_JSON {all|both|HOST_ID}" >&2
  exit 2
fi
contracts="$1"
selection="$2"
[[ -f "$contracts" ]] || { echo "host contract missing: $contracts" >&2; exit 1; }
command -v jq >/dev/null || { echo "jq is required" >&2; exit 1; }

# Reject malformed or duplicate hosts before starting expensive LLVM builds.
# Host identity and runner metadata must agree with the canonical toolchain and
# backend-smoke contracts instead of introducing fallback host assumptions.
jq -e '
  (.ci_hosts | type == "array" and length > 0 and length <= 256) and
  ([.ci_hosts[].id] | length == (unique | length)) and
  (. as $contract | all(.ci_hosts[];
    (.id | type == "string" and test("^[a-z0-9_-]+$")) and
    (.runner | type == "string" and length > 0) and
    (.target | type == "string" and length > 0) and
    (.host_os | type == "string" and length > 0) and
    (.host_arch | type == "string" and length > 0) and
    (.file_pattern | type == "string" and length > 0) and
    (if .host_os == "Windows" then .id else (.id | split("-")[0]) end) as $platform |
    ($contract.llvm_toolchain.platforms[$platform].host_arch == .host_arch) and
    ($contract.backend_smoke.validators[.target] != null) and
    (if .host_os == "Windows" then
       ($contract.backend_smoke.validators[.target].header_regex | type == "string" and length > 0)
     else true end)
  ))
' "$contracts" >/dev/null || {
  echo "invalid host inventory in $contracts: check IDs, runners, LLVM platforms and targets" >&2
  exit 1
}

# Resolve the PE machine-header pattern from the existing target validator,
# rather than maintaining an additional target-to-machine switch.
make_matrix() {
  jq -c --arg selection "$selection" '
    . as $contract | {include: [.ci_hosts[] |
      select($selection == "all" or
        ($selection == "both" and (.id == "linux-x86_64" or .id == "macos-arm64")) or
        .id == $selection) |
      . + {header_regex: ($contract.backend_smoke.validators[.target].header_regex // "")}
    ]}
  ' "$contracts"
}

case "$selection" in
  all|both) matrix="$(make_matrix)" ;;
  *) matrix="$(make_matrix)" ;;
esac

[[ "$(jq '.include | length' <<< "$matrix")" -gt 0 ]] || {
  echo "no configured CI host matches selection: $selection" >&2
  exit 1
}
if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
  printf 'matrix=%s\n' "$matrix" >> "$GITHUB_OUTPUT"
else
  printf '%s\n' "$matrix"
fi
