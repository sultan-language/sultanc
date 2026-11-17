#!/bin/sh
# Lint: CI only verifies. No workflow or CI script may merge, promote, push,
# tag, publish a package or create, edit or publish a GitHub release; MASTER
# promotes main -> beta -> master and releases by hand. Fails when
#   - a workflow has no top-level permissions block (the token default may
#     be write),
#   - a workflow grants contents: write or packages: write (what pushing,
#     merging, tagging, releases and packages need), or
#   - a workflow or CI script (outside comments) uses a release action, the
#     gh commands that release or merge, or git push / tag / merge.
set -eu
cd "$(dirname "$0")/../.."
status=0
for workflow in .github/workflows/*.yml; do
    if ! grep -q '^permissions:' "$workflow"; then
        echo "::error file=$workflow::no top-level permissions block (the token must not be able to write)"
        status=1
    fi
done
pattern='contents:[[:space:]]*write|packages:[[:space:]]*write|action-gh-release|create-release|upload-release'
pattern="$pattern|gh[[:space:]]+release[[:space:]]+(create|upload|edit|delete)|gh[[:space:]]+pr[[:space:]]+merge"
pattern="$pattern|git[[:space:]]+(push|tag|merge)([[:space:]]|$)|/merges|/git/refs|/releases\"?[[:space:]]+(-X|--method)"
set --
for file in .github/workflows/*.yml .github/ci/*.sh .github/ci/*.ps1 .github/ci/*.py; do
    [ "$file" = .github/ci/no_release.sh ] || set -- "$@" "$file"
done
bad=$(grep -nE "$pattern" "$@" | grep -vE '^[^:]+:[0-9]+:[[:space:]]*#' || true)
if [ -n "$bad" ]; then
    echo "::error::CI must never merge, promote, push, tag or release:"
    echo "$bad"
    status=1
fi
[ "$status" -ne 0 ] || echo "no workflow or CI script can merge, promote, tag or release"
exit "$status"
