#!/bin/sh
# Lint: CI verifies; only promote.yml moves a branch, and nothing publishes a
# release. The one exception is the publish job of release.yml: it creates a
# DRAFT release of an existing tag, which the owner publishes by hand. Fails
# when
#   - a workflow has no top-level permissions block (the token default may
#     be write),
#   - a workflow grants contents: write or packages: write (what pushing,
#     merging, tagging, releases and packages need through the workflow
#     token), id-token: write or attestations: write, except the publish job
#     of release.yml, which grants contents, id-token and attestations write
#     once each,
#   - a workflow or CI script (outside comments) uses a release action, the gh
#     commands that create, edit, upload to or delete a release, or the
#     releases API, except one `gh release create TAG --draft --verify-tag`
#     (a draft, of a tag that already exists) in that job,
#   - anything turns a draft into a published release (draft false),
#   - release.yml reads a secret (it writes only with its own job token), or
#     its publish job is not in the environment `release` (the owner's
#     approval),
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
release='contents:[[:space:]]*write|packages:[[:space:]]*write|id-token:[[:space:]]*write|attestations:[[:space:]]*write'
release="$release|action-gh-release|create-release|upload-release|(create|update|delete)Release"
release="$release|gh[[:space:]]+release[[:space:]]+(create|upload|edit|delete)|/releases\"?[[:space:]]+(-X|--method)"
release="$release|gh[[:space:]]+api[^#]*releases|api\.github\.com/[^[:space:]]*releases|uploads\.github\.com"
release="$release|draft[\"']?[[:space:]]*[=:][[:space:]]*false"
release="$release|force=true|--force"
move='gh[[:space:]]+pr[[:space:]]+merge|git[[:space:]]+(push|tag|merge)([[:space:]]|$)|/merges|/git/refs'
all=
others=
for file in .github/workflows/*.yml .github/ci/*.sh .github/ci/*.ps1 .github/ci/*.py; do
    [ "$file" = .github/ci/no_release.sh ] && continue
    all="$all $file"
    [ "$file" = .github/workflows/promote.yml ] || others="$others $file"
done

# The publish job of release.yml: its first and last line (0 0 without it).
publisher=.github/workflows/release.yml
range="0 0"
if [ -f "$publisher" ]; then
    range=$(awk '
        /^jobs:/ { jobs = 1; next }
        jobs && /^  publish:[[:space:]]*$/ { start = NR; next }
        start && !end && (/^  [A-Za-z0-9_-]+:/ || /^[^[:space:]#]/) { end = NR - 1 }
        END { if (!start) print "0 0"; else print start, (end ? end : NR) }' "$publisher")
    if grep -n 'secrets\.' "$publisher" | grep -vE '^[0-9]+:[[:space:]]*#' > /dev/null; then
        echo "::error file=$publisher::release.yml reads a secret: it may write only with its own job token"
        status=1
    fi
fi
first=${range% *}
last=${range#* }
# One line of the publish job may grant each write and one may create the
# draft; every other match is an error.
allowed() { # file:line:text -> 0 when the publish job may have it
    case "$1" in "$publisher":*) ;; *) return 1 ;; esac
    n=${1#"$publisher":}
    text=${n#*:}
    n=${n%%:*}
    [ "$first" -gt 0 ] && [ "$n" -gt "$first" ] && [ "$n" -le "$last" ] || return 1
    printf '%s\n' "$text" | grep -qE '^[[:space:]]+(contents|id-token|attestations):[[:space:]]*write[[:space:]]*$' && return 0
    printf '%s\n' "$text" | grep -E 'gh[[:space:]]+release[[:space:]]+create[[:space:]]' |
        grep -E -- '--draft([[:space:]]|$)' | grep -qE -- '--verify-tag([[:space:]]|$)' && return 0
    return 1
}
# shellcheck disable=SC2086 # word splitting of the file lists is intended
matches=$( { grep -nHE "$release" $all || true; } | grep -vE '^[^:]+:[0-9]+:[[:space:]]*#' || true)
bad=
granted=
created=0
newline='
'
while IFS= read -r match; do
    [ -n "$match" ] || continue
    if allowed "$match"; then
        case "$match" in
            *"release create"*) created=$((created + 1)) ;;
            *) granted="$granted$(printf '%s' "${match#*:*:}" | tr -d ' ')$newline" ;;
        esac
    else
        bad="$bad$match$newline"
    fi
done <<EOF
$matches
EOF
if [ "$first" -gt 0 ]; then
    for grant in contents:write id-token:write attestations:write; do
        count=$(printf '%s' "$granted" | grep -cx "$grant" || true)
        if [ "$count" -gt 1 ]; then
            bad="$bad$publisher: the publish job grants $grant $count times$newline"
        fi
    done
    if [ "$created" -gt 1 ]; then
        bad="$bad$publisher: the publish job creates $created releases (one draft only)$newline"
    fi
    if ! sed -n "${first},${last}p" "$publisher" | grep -qE '^    environment:[[:space:]]*release[[:space:]]*$'; then
        bad="$bad$publisher: the publish job must run in the environment release (the owner approves it)$newline"
    fi
fi
# shellcheck disable=SC2086 # word splitting of the file lists is intended
moves=$( { grep -nHE "$move" $others || true; } | grep -vE '^[^:]+:[0-9]+:[[:space:]]*#' || true)
[ -z "$moves" ] || bad="$bad$moves$newline"
if [ -n "$bad" ]; then
    echo "::error::CI must never publish a release or force a ref, only release.yml's publish job creates a draft, and only promote.yml moves a branch:"
    printf '%s' "$bad"
    status=1
fi
[ "$status" -ne 0 ] || echo "no workflow can publish a release or force a ref; release.yml's publish job only drafts one; only promote.yml moves a branch"
exit "$status"
