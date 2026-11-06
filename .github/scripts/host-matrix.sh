#!/usr/bin/env bash
set -euo pipefail

# One declarative host registry, with shared defaults. The resolved record is
# the same in provisioning, cache restore, build, seed bootstrap and tests.
#
#   host-matrix.sh CONTRACTS_JSON all        matrix of every registered host
#   host-matrix.sh CONTRACTS_JSON HOST_ID    matrix of one host
#   host-matrix.sh CONTRACTS_JSON --current  the record of this runner
#                                            (RUNNER_OS, RUNNER_ARCH and,
#                                            when set, SULTANC_CI_HOST_ID)
#
# Every resolved record carries `gates`: for each check of ci_check_catalog,
# "required", "experimental" or "off". Workflows select and soften steps only
# through these values; no script branches on an operating-system name.
usage() {
  echo "usage: host-matrix.sh CONTRACTS_JSON {all|HOST_ID|--current}" >&2
  exit 2
}

[[ $# -eq 2 ]] || usage
contracts="$1"
selection="$2"
[[ -f "$contracts" ]] || { echo "host contract missing: $contracts" >&2; exit 1; }
command -v jq >/dev/null || { echo "jq is required" >&2; exit 1; }

# Host entry overrides shared defaults (shallow: a nested object given by a
# host replaces the default object as a whole). gates are derived, not stored.
filter='
def gates($catalog):
  . as $h |
  reduce ($catalog | keys[]) as $check ({};
    .[$check] = (if ($h.checks.required | index($check)) != null then "required"
                 elif ($h.checks.experimental | index($check)) != null then "experimental"
                 else "off" end));
def hosts:
  .ci_check_catalog as $catalog |
  .ci_host_defaults as $defaults |
  [.ci_hosts[] | ($defaults + .) | . + {gates: gates($catalog)}];
def strings_ok: type == "array" and all(.[]; type == "string" and (contains("\n") | not));
def env_ok: type == "object" and all(to_entries[];
  (.key | test("^[A-Z][A-Z0-9_]*$")) and (.value | type == "string" and (contains("\n") | not)));
def sha_ok: type == "string" and (test("^[0-9a-f]{64}$") or test("^TODO"));
def archive_ok:
  type == "object" and
  (.archive | type == "string" and test("^[A-Za-z0-9._+-]+$")) and
  (.url | type == "string" and test("^https://")) and
  (.sha256 | sha_ok) and
  (.root | type == "string" and test("^[A-Za-z0-9._+-]+$")) and
  (.members | type == "array" and length > 0 and
    all(.[]; type == "string" and test("^[A-Za-z0-9._+*/-]+$") and (contains("..") | not)));
def relative_ok: type == "string" and test("^[A-Za-z0-9._+-]+(/[A-Za-z0-9._+-]+)*$") and (contains("..") | not);
'

jq -e "$filter"'
  (.ci_check_catalog | type == "object" and length > 0 and
    all(to_entries[]; (.key | test("^[a-z0-9-]+$")) and
      (.value.requires | type == "array" and all(.[]; type == "string")))) and
  (.ci_check_catalog as $catalog |
    all($catalog[]; all(.requires[]; $catalog[.] != null))) and
  (.ci_host_defaults | type == "object") and
  (.ci_hosts | type == "array" and length > 0 and length <= 256) and
  (.ci_check_catalog as $catalog | hosts as $hosts |
    ([$hosts[].id] | length == (unique | length)) and
    ([$hosts[] | select(.llvm_broad_host == true)] | length == 1) and
    all($hosts[]; . as $h |
      (.id | type == "string" and test("^[a-z0-9_-]+$")) and
      (.tier == "native" or .tier == "portable") and
      ((.runner | type == "string" and length > 0) or
       (.runner | type == "array" and length > 0 and
         all(.[]; type == "string" and length > 0))) and
      (.runner_os | type == "string" and length > 0) and
      (.runner_arch | type == "string" and length > 0) and
      (.target | type == "string" and test("^[a-z0-9_.-]+$")) and
      (.host_os | type == "string" and length > 0) and
      (.host_arch | type == "string" and length > 0) and
      (.file_pattern | type == "string" and length > 0) and
      (.compiler_c | type == "string" and length > 0) and
      (.compiler_cxx | type == "string" and length > 0) and
      (.compiler_id | type == "string" and length > 0) and
      (.compiler_binary_pattern | type == "string") and
      (.llvm_config | type == "string" and length > 0) and
      (.executable_suffix | type == "string" and test("^[a-zA-Z0-9._-]*$")) and
      (.prerequisites | type == "array" and all(.[]; type == "string" and test("^[A-Za-z0-9._+-]+$"))) and
      (.path_converter | type == "object" and
        (.command | type == "string" and (. == "" or test("^[/a-zA-Z0-9._+-]+$"))) and
        (.to_shell_arguments | strings_ok) and
        (.to_native_arguments | strings_ok) and
        (.command != "" or ((.to_shell_arguments | length) == 0 and
                              (.to_native_arguments | length) == 0))) and
      (.identity | type == "object" and
        (.command | type == "string" and (. == "" or test("^[/a-zA-Z0-9._+-]+$"))) and
        (.os_arguments | strings_ok) and
        (.arch_arguments | strings_ok) and
        (.command != "" or ((.os_arguments | length) == 0 and
                             (.arch_arguments | length) == 0))) and
      (.bootstrap | type == "array" and all(.[];
        (.command | type == "string" and test("^[/a-zA-Z0-9._+-]+$")) and
        (.arguments | strings_ok) and
        (.script | type == "string" and (. == "" or test("^[a-zA-Z0-9._-]+$"))) and
        (.environment | env_ok))) and
      (.archive_environment | env_ok) and
      (.archive_commands | type == "object" and
        (.tar | type == "string" and test("^[/a-zA-Z0-9._+-]+$")) and
        (.ln | type == "string" and test("^[/a-zA-Z0-9._+-]+$")) and
        (.member_arguments | strings_ok)) and
      (.llvm_broad_host == null or (.llvm_broad_host | type == "boolean")) and
      (.object_validator | type == "object" and
        (.executable_header_regex == null or (.executable_header_regex | type == "string" and length > 0))) and
      (.object_validator.type as $format |
        if $format == "elf" then
          (.object_validator.file_regex | type == "string" and length > 0) and
          (.object_validator.class | type == "string" and length > 0) and
          (.object_validator.machine_regex | type == "string" and length > 0)
        elif ($format == "macho" or $format == "coff" or $format == "file-header") then
          (.object_validator.header_regex | type == "string" and length > 0) and
          ($format != "file-header" or
            (.object_validator.file_regex | type == "string" and length > 0))
        else false end) and
      # Checks: known ids, disjoint, and every dependency at least as strong.
      (.checks | type == "object" and
        (.required | type == "array" and all(.[]; $catalog[.] != null)) and
        (.experimental | type == "array" and all(.[]; $catalog[.] != null))) and
      (.checks.required as $r | .checks.experimental as $e |
        ([$r[] | . as $c | select(($e | index($c)) != null)] | length == 0) and
        all($r[]; all($catalog[.].requires[]; . as $d | ($r | index($d)) != null)) and
        all($e[]; all($catalog[.].requires[]; . as $d |
          (($r | index($d)) != null) or (($e | index($d)) != null)))) and
      # The broad LLVM host builds the compiler with LLVM.
      (.llvm_broad_host != true or .gates["c-stage2"] == "required") and
      # Build arguments are build.sh options and CMake cache seeds, never shell text.
      (.build | type == "object" and
        (.arguments | type == "array" and all(.[]; type == "string" and test("^--[a-z][a-z0-9-]*(=[A-Za-z0-9._+-]+)?$"))) and
        (.cmake_cache | env_ok) and
        (.environment | env_ok)) and
      # Seed bootstrap drivers.
      (.seed | type == "object" and
        (.mode == "verify-check-seed" or .mode == "stage1-reproduce") and
        (.drivers | type == "array" and length > 0 and
          ([.[].id] | length == (unique | length)) and
          (.[0].status == "required") and
          all(.[];
            (.id | type == "string" and test("^[a-z0-9-]+$")) and
            (.status == "required" or .status == "experimental") and
            (.jobs | type == "number" and . >= 1 and . <= 64) and
            (if .kind == "bootstrap.c" then
               (.cc | type == "string" and test("^[A-Za-z0-9._+/-]+$")) and
               (.cflags | type == "string" and (contains("\n") | not)) and
               (.arguments | type == "array" and all(.[];
                 . == "--verify" or . == "--check-seed" or . == "--stage1-only"))
             elif .kind == "cmake" then
               (.generator | type == "string" and test("^[A-Za-z0-9 -]+$")) and
               (.c_compiler | type == "string" and test("^[A-Za-z0-9._+/-]+$")) and
               (.targets | type == "array" and length > 0 and all(.[]; type == "string" and test("^sultanc-[a-z0-9-]+$")))
             else false end)))) and
      # A verify-check-seed host must run Stage2, so it must be a native-tier host.
      (.seed.mode != "verify-check-seed" or .tier == "native") and
      (.gates["tests-portable"] == "off" or (.tests.portable_target | type == "string" and test("^[a-z0-9_.-]+$"))) and
      # LLVM SDK provisioning method and its data.
      (.llvm | type == "object" and
        (.method == "prebuilt" or .method == "source" or .method == "none") and
        ($h.gates["llvm-sdk"] == "off" or .method != "none") and
        (.method == "none" or (
          (.required_files | type == "array" and length > 0 and all(.[]; relative_ok)) and
          (.keep | type == "array" and length > 0 and all(.[]; relative_ok)) and
          (.exports | type == "object" and (.file | relative_ok) and
            (.command | type == "array" and length > 0 and all(.[]; type == "string")) and
            (.symbol_prefix | type == "string" and test("^[_]?$"))))) and
        (.method != "prebuilt" or (
          (.prebuilt | archive_ok) and
          (.shared_library == null or (.shared_library | type == "object" and
            (.output | relative_ok) and
            (.command | type == "array" and length > 0 and all(.[]; type == "string")) and
            (.aliases | type == "object" and all(to_entries[]; (.key | relative_ok) and (.value | test("^[A-Za-z0-9._+-]+$")))))))) and
        (.method != "source" or (
          (.host_compiler == null or (.host_compiler | archive_ok and
            (.c | relative_ok) and (.cxx | relative_ok))) and
          (.source_build == null or (.source_build | type == "object" and
            (.cmake_arguments == null or (.cmake_arguments | type == "array" and all(.[]; type == "string" and test("^-D[A-Za-z0-9_]+=[A-Za-z0-9._;+-]*$"))))))))
    )))
' "$contracts" >/dev/null || {
  echo "invalid ci_hosts registry (checks, seed drivers, llvm provisioning, tools or validators): $contracts" >&2
  exit 1
}

case "$selection" in
  --current)
    [[ -n "${RUNNER_OS:-}" && -n "${RUNNER_ARCH:-}" ]] || {
      echo "RUNNER_OS and RUNNER_ARCH are required to identify the configured host" >&2
      exit 1
    }
    host="$(jq -c "$filter"'
      hosts | map(select(.runner_os == $os and .runner_arch == $arch and
        ($id == "" or .id == $id))) |
      if length == 1 then .[0] else empty end
    ' --arg os "$RUNNER_OS" --arg arch "$RUNNER_ARCH" \
      --arg id "${SULTANC_CI_HOST_ID:-}" "$contracts")"
    [[ -n "$host" ]] || {
      echo "no unique CI host for runner: $RUNNER_OS/$RUNNER_ARCH (id=${SULTANC_CI_HOST_ID:-unset})" >&2
      exit 1
    }
    printf '%s\n' "$host"
    ;;
  *)
    matrix="$(jq -c "$filter"'
      {include: [hosts[] | select($selection == "all" or .id == $selection)]}
    ' --arg selection "$selection" "$contracts")"
    [[ "$(jq '.include | length' <<< "$matrix")" -gt 0 ]] || {
      echo "no configured CI host matches selection: $selection" >&2
      exit 1
    }
    if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
      broad_host="$(jq -c "$filter"'hosts[] | select(.llvm_broad_host == true)' "$contracts")"
      # Provisioning only runs for hosts that use an LLVM SDK.
      llvm_matrix="$(jq -c '{include: [.include[] | select(.llvm.method != "none")]}' <<< "$matrix")"
      printf 'matrix=%s\n' "$matrix" >> "$GITHUB_OUTPUT"
      printf 'llvm_matrix=%s\n' "$llvm_matrix" >> "$GITHUB_OUTPUT"
      printf 'llvm_host_count=%s\n' "$(jq '.include | length' <<< "$llvm_matrix")" >> "$GITHUB_OUTPUT"
      printf 'broad_host_id=%s\n' "$(jq -r '.id' <<< "$broad_host")" >> "$GITHUB_OUTPUT"
      printf 'broad_runner=%s\n' "$(jq -c '.runner' <<< "$broad_host")" >> "$GITHUB_OUTPUT"
      printf 'broad_target=%s\n' "$(jq -r '.target' <<< "$broad_host")" >> "$GITHUB_OUTPUT"
    else
      printf '%s\n' "$matrix"
    fi
    ;;
esac
