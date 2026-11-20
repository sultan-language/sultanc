#!/bin/sh
# Lint: CI verifies; only promote.yml moves a branch, and nothing releases.
# The owner releases master by hand. Fails when
#   - a workflow has no top-level permissions block (the token default may
#     be write),
#   - a workflow grants contents: write or packages: write (what pushing,
#     merging, tagging, releases and packages need through the workflow
#     token),
#   - a workflow or CI script (outside comments) uses a release action or the
#     gh commands that create, edit or delete a release,
#   - a workflow or CI script other than promote.yml merges, pushes, tags or
#     moves a ref (git push / tag / merge, gh pr merge, the merges and
#     git/refs endpoints), or
#   - anything forces a ref (force=true, --force): promotion is a fast-forward.
set -eu
cd "$(dirname "$0")/../.."
status=0
for workflow in .github/workflows/*.yml; do
    if ! grep -q '^permissions:' "$workflow"; then
        echo "::error file=$workflow::no top-level permissions block (the token must not be able to write)"
        status=1
    fi
done
release='contents:[[:space:]]*write|packages:[[:space:]]*write|action-gh-release|create-release|upload-release'
release="$release|gh[[:space:]]+release[[:space:]]+(create|upload|edit|delete)|/releases\"?[[:space:]]+(-X|--method)"
release="$release|force=true|--force"
move='gh[[:space:]]+pr[[:space:]]+merge|git[[:space:]]+(push|tag|merge)([[:space:]]|$)|/merges|/git/refs'
all=
others=
for file in .github/workflows/*.yml .github/ci/*.sh .github/ci/*.ps1 .github/ci/*.py; do
    [ "$file" = .github/ci/no_release.sh ] && continue
    all="$all $file"
    [ "$file" = .github/workflows/promote.yml ] || others="$others $file"
done
# shellcheck disable=SC2086 # word splitting of the file lists is intended
bad=$( { grep -nE "$release" $all || true; grep -nE "$move" $others || true; } | grep -vE '^[^:]+:[0-9]+:[[:space:]]*#' || true)
if [ -n "$bad" ]; then
    echo "::error::CI must never release or force a ref, and only promote.yml moves a branch:"
    echo "$bad"
    status=1
fi
[ "$status" -ne 0 ] || echo "no workflow can release or force a ref; only promote.yml moves a branch"
exit "$status"
