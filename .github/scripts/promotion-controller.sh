#!/usr/bin/env bash
set -euo pipefail

# Construct a protected promotion candidate from one qualified branch to the next.
# This controller never force-pushes or bypasses destination required checks.

fail() {
  echo "$*" >&2
  exit 1
}

require_environment() {
  : "${GH_TOKEN:?GH_TOKEN is required}"
  : "${SULTANC_REPOSITORY:?SULTANC_REPOSITORY is required}"
  : "${SOURCE_BRANCH:?SOURCE_BRANCH is required}"
  : "${DESTINATION_BRANCH:?DESTINATION_BRANCH is required}"
  : "${SOURCE_SHA:?SOURCE_SHA is required}"
  : "${DESTINATION_SHA:?DESTINATION_SHA is required}"
  : "${TRIGGER_RUN_ID:?TRIGGER_RUN_ID is required}"
  : "${TRIGGER_RUN_ATTEMPT:?TRIGGER_RUN_ATTEMPT is required}"
  : "${HARNESS_SHA:?HARNESS_SHA is required}"
  : "${CONTRACT_DIGEST:?CONTRACT_DIGEST is required}"
  : "${MERGE_METHOD:?MERGE_METHOD is required}"
}

validate_edge() {
  case "$SOURCE_BRANCH:$DESTINATION_BRANCH" in
    main:beta|beta:master) ;;
    *) fail "illegal promotion edge" ;;
  esac

  case "$MERGE_METHOD" in
    merge|squash|rebase) ;;
    *) fail "unsupported promotion merge method" ;;
  esac
}

branch_sha() {
  local branch="$1"
  gh api \
    "repos/$SULTANC_REPOSITORY/git/ref/heads/$branch" \
    --jq '.object.sha'
}

verify_branch_tips() {
  local phase="$1"
  local current_source
  local current_destination

  current_source="$(branch_sha "$SOURCE_BRANCH")"
  current_destination="$(branch_sha "$DESTINATION_BRANCH")"

  [[ "$current_source" == "$SOURCE_SHA" ]] || fail \
    "source moved $phase"
  [[ "$current_destination" == "$DESTINATION_SHA" ]] || fail \
    "destination moved $phase"
}

construct_candidate() {
  git checkout -B "$promotion_branch" "$DESTINATION_SHA"

  if ! git -c core.hooksPath=/dev/null \
    merge --no-ff --no-commit "$SOURCE_SHA"; then
    git merge --abort || true
    fail "promotion merge conflict requires user resolution"
  fi

  "$script_dir/render-branch-profile.sh" "$DESTINATION_BRANCH" "$repo"
  "$script_dir/validate-branch-profile.sh" "$DESTINATION_BRANCH" "$repo"

  git config user.name "sultanc-promotion[bot]"
  git config user.email \
    "sultanc-promotion[bot]@users.noreply.github.com"

  export GIT_AUTHOR_DATE
  export GIT_COMMITTER_DATE
  GIT_AUTHOR_DATE="$(git show -s --format=%cI "$SOURCE_SHA")"
  GIT_COMMITTER_DATE="$GIT_AUTHOR_DATE"

  git add .github/workflows .github/ci/branch-profiles
  git -c core.hooksPath=/dev/null commit \
    -m "promote: $SOURCE_BRANCH to $DESTINATION_BRANCH"
}

push_candidate_once() {
  local remote_head

  expected_head="$(git rev-parse HEAD)"
  remote_head="$(
    git ls-remote --heads origin "$promotion_branch" | awk '{print $1}'
  )"

  if [[ -z "$remote_head" ]]; then
    git push origin "HEAD:refs/heads/$promotion_branch"
    return
  fi

  [[ "$remote_head" == "$expected_head" ]] || fail \
    "existing promotion branch mismatch: "\
    "expected=$expected_head actual=$remote_head"
}

close_superseded_candidates() {
  local number
  local candidate_title
  local head

  while IFS=$'\t' read -r number candidate_title head; do
    [[ -n "$number" ]] || continue
    [[ "$candidate_title" == "$title" ]] || continue
    [[ "$head" == "$promotion_branch" ]] && continue

    gh pr close "$number" \
      --repo "$SULTANC_REPOSITORY" \
      --comment "Superseded by a newer qualified promotion candidate." \
      >/dev/null
  done < <(
    gh pr list \
      --repo "$SULTANC_REPOSITORY" \
      --base "$DESTINATION_BRANCH" \
      --state open \
      --json number,title,headRefName \
      --jq '.[] | [.number, .title, .headRefName] | @tsv'
  )
}

find_promotion_pr() {
  gh pr list \
    --repo "$SULTANC_REPOSITORY" \
    --base "$DESTINATION_BRANCH" \
    --head "$promotion_branch" \
    --state open \
    --json number \
    --jq '.[0].number // empty'
}

create_promotion_pr() {
  local body="$work/body.md"

  cat > "$body" <<EOF_BODY
Automatic protected SultanC promotion candidate.

- Source branch: \`$SOURCE_BRANCH\`
- Qualified landed source SHA: \`$SOURCE_SHA\`
- Destination branch: \`$DESTINATION_BRANCH\`
- Destination base SHA: \`$DESTINATION_SHA\`
- Expected candidate head SHA: \`$expected_head\`
- Integration run: \`$TRIGGER_RUN_ID\` attempt \`$TRIGGER_RUN_ATTEMPT\`
- Trusted controller harness SHA: \`$HARNESS_SHA\`
- Qualification contract digest: \`$CONTRACT_DIGEST\`
- Destination branch profile was projected before candidate checks.
- Releases: none.
EOF_BODY

  gh pr create \
    --repo "$SULTANC_REPOSITORY" \
    --base "$DESTINATION_BRANCH" \
    --head "$promotion_branch" \
    --title "$title" \
    --body-file "$body" \
    >/dev/null
}

verify_promotion_pr() {
  local pr_json
  local pr_head
  local pr_base
  local pr_head_ref
  local pr_base_ref

  pr_json="$(gh api "repos/$SULTANC_REPOSITORY/pulls/$pr")"
  pr_head="$(jq -r '.head.sha' <<< "$pr_json")"
  pr_base="$(jq -r '.base.sha' <<< "$pr_json")"
  pr_head_ref="$(jq -r '.head.ref' <<< "$pr_json")"
  pr_base_ref="$(jq -r '.base.ref' <<< "$pr_json")"

  [[ "$pr_head" == "$expected_head" ]] || fail \
    "promotion PR head SHA is not the intended candidate"
  [[ "$pr_head_ref" == "$promotion_branch" ]] || fail \
    "promotion PR head branch is not the intended candidate"
  [[ "$pr_base" == "$DESTINATION_SHA" ]] || fail \
    "promotion PR base moved; rebuild the candidate"
  [[ "$pr_base_ref" == "$DESTINATION_BRANCH" ]] || fail \
    "promotion PR base branch is incorrect"
}

request_auto_merge() {
  local merge_flag="--$MERGE_METHOD"

  gh pr merge "$pr" \
    --repo "$SULTANC_REPOSITORY" \
    --auto \
    "$merge_flag" \
    --match-head-commit "$expected_head"

  printf \
    'Promotion PR #%s is bound to %s and waiting on protected checks.\n' \
    "$pr" \
    "$expected_head"
}

require_environment
validate_edge
script_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
work="$(mktemp -d "${RUNNER_TEMP:-/tmp}/sultanc-promotion.XXXXXX")"
trap 'rm -rf "$work"' EXIT
repo="$work/repo"

source_short="${SOURCE_SHA:0:12}"
destination_short="${DESTINATION_SHA:0:12}"
promotion_branch="ci/promote-${SOURCE_BRANCH}-to-${DESTINATION_BRANCH}"
promotion_branch+="-${source_short}-${destination_short}"
title="promote: $SOURCE_BRANCH to $DESTINATION_BRANCH"

gh repo clone \
  "$SULTANC_REPOSITORY" \
  "$repo" \
  -- \
  --filter=blob:none \
  --no-checkout
cd "$repo"
git -c core.hooksPath=/dev/null fetch \
  origin \
  "$SOURCE_SHA" \
  "$DESTINATION_SHA"

# Bind candidate construction to the exact source and destination tips qualified.
verify_branch_tips "after qualification"
construct_candidate
push_candidate_once
close_superseded_candidates

pr="$(find_promotion_pr)"
if [[ -z "$pr" ]]; then
  create_promotion_pr
  pr="$(find_promotion_pr)"
fi
[[ -n "$pr" ]] || fail "promotion PR could not be located after creation"

verify_promotion_pr

# Recheck both protected branch tips immediately before requesting auto-merge.
verify_branch_tips "before auto-merge request"
request_auto_merge
