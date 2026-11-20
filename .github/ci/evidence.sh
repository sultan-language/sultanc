#!/bin/sh
# Qualification evidence for one exact commit:
#   sh .github/ci/evidence.sh SHA
# prints the URL of the run that qualified SHA, or prints nothing.
#
# The evidence is the commit status `sultanc/qualification` in state success,
# which only the Qualified job of ci.yml sets: on the commit a full-tier run
# tested (`ref`), after its gate passed (every planned job of every host
# succeeded), when the caller asked for qualification (nightly.yml,
# promote.yml, beta.yml). A status lives on the commit, not on a branch, so
# it follows the commit through every fast-forward promotion; quick, focused
# and pull-request runs never set it.
#
# Environment: REPO (owner/name), GH_TOKEN (statuses: read). An API error
# prints nothing: the caller then qualifies the commit instead of trusting
# evidence it could not read.
set -eu
sha=${1:?usage: evidence.sh SHA}
: "${REPO:?evidence.sh: REPO is not set}"
# The combined status lists the latest status of each context.
url=$(gh api "repos/$REPO/commits/$sha/status" \
    --jq '.statuses[] | select(.context == "sultanc/qualification" and .state == "success") | .target_url' \
    2>/dev/null | head -n 1) || url=
if [ -n "$url" ]; then
    printf '%s\n' "$url"
fi
