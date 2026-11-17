#!/bin/sh
# Qualification evidence for one exact commit:
#   sh .github/ci/evidence.sh SHA
# prints the URL of a successful run of the SultanC Beta qualification
# (beta.yml) or SultanC Master (master.yml) workflow on exactly SHA, started by
# a push or a manual dispatch, or prints nothing. Pull-request runs never
# count: they qualify nothing. A run counts only when the whole workflow
# succeeded, and those workflows succeed only when they ran the full
# qualification on SHA or found such a run themselves, so every URL leads
# back to a full qualification of that commit.
#
# Environment: REPO (owner/name), GH_TOKEN (actions: read). An API error
# prints nothing: the caller then qualifies the commit instead of trusting
# evidence it could not read.
set -eu
sha=${1:?usage: evidence.sh SHA}
: "${REPO:?evidence.sh: REPO is not set}"
# All runs of the commit, filtered by workflow path: each branch carries its
# own .github, so beta.yml and master.yml need not exist on the branch that
# asks.
url=$(gh api --paginate "repos/$REPO/actions/runs?head_sha=$sha&status=success&per_page=100" \
    --jq ".workflow_runs[] | select(.head_sha == \"$sha\")
          | select(.path == \".github/workflows/beta.yml\" or .path == \".github/workflows/master.yml\")
          | select(.event == \"push\" or .event == \"workflow_dispatch\") | .html_url" \
    2>/dev/null | head -n 1) || url=
if [ -n "$url" ]; then
    printf '%s\n' "$url"
fi
