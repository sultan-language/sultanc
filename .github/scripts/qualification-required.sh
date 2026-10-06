#!/usr/bin/env bash
set -euo pipefail

# Enforce the mandatory job graph for one qualification profile and depth.

fail() {
  echo "$*" >&2
  exit 1
}

: "${RESOLVE_RESULT:?resolve result is required}"
: "${NATIVE_RESULT:?native result is required}"
: "${LLVM_CONTRACTS_RESULT:?LLVM contract result is required}"
: "${LLVM_BROAD_RESULT:?LLVM broad result is required}"
: "${LLVM_ACTIVE:=false}"
: "${LLVM_COMPLETE:=false}"
: "${PROFILE:?profile is required}"
: "${MODE:?mode is required}"

[[ "$RESOLVE_RESULT" == "success" ]] || fail \
  "candidate resolution did not succeed"
[[ "$NATIVE_RESULT" == "success" ]] || fail \
  "native qualification did not succeed"

broad_expected="false"
if [[ "$PROFILE" != "main" || "$MODE" == "deep" ]]; then
  broad_expected="true"
fi

if [[ "$broad_expected" == "false" ]]; then
  [[ "$LLVM_CONTRACTS_RESULT" == "skipped" ]] || fail \
    "unexpected LLVM contract job result: $LLVM_CONTRACTS_RESULT"
  [[ "$LLVM_BROAD_RESULT" == "skipped" ]] || fail \
    "unexpected LLVM broad job result: $LLVM_BROAD_RESULT"
  exit 0
fi

[[ "$LLVM_CONTRACTS_RESULT" == "success" ]] || fail \
  "LLVM coverage contract audit did not succeed"

if [[ "$PROFILE" == "beta" || "$PROFILE" == "master" ]]; then
  [[ "$LLVM_COMPLETE" == "true" ]] || fail \
    "$PROFILE candidate requires all 34 LLVM output contracts"
  [[ "$LLVM_ACTIVE" == "true" ]] || fail \
    "$PROFILE candidate requires active LLVM broad coverage"
  [[ "$LLVM_BROAD_RESULT" == "success" ]] || fail \
    "$PROFILE broad LLVM coverage did not succeed"
  exit 0
fi

if [[ "$LLVM_ACTIVE" == "true" ]]; then
  [[ "$LLVM_BROAD_RESULT" == "success" ]] || fail \
    "activated LLVM broad coverage did not succeed"
else
  [[ "$LLVM_BROAD_RESULT" == "skipped" ]] || fail \
    "LLVM broad job had unexpected result: $LLVM_BROAD_RESULT"
fi
