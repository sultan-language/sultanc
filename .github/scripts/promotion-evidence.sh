#!/usr/bin/env bash
set -euo pipefail

# Resolve one exact successful landed integration run for main->beta or beta->master.

fail() {
  echo "$*" >&2
  exit 1
}

branch_sha() {
  local branch="$1"
  gh api \
    "repos/$REPOSITORY/git/ref/heads/$branch" \
    --jq '.object.sha'
}

latest_successful_run_for_sha() {
  local branch="$1"
  local sha="$2"
  local runs_json
  local filter

  runs_json="$(
    gh api \
      --method GET \
      "repos/$REPOSITORY/actions/workflows/$integration_workflow/runs" \
      -f branch="$branch" \
      -f event=push \
      -f status=completed \
      -f per_page=100
  )"
  filter='\
    [\
      .workflow_runs[] |\
      select(.head_sha == $sha and .conclusion == "success")\
    ] |\
    sort_by(.run_number, .run_attempt) |\
    last |\
    .id // empty\
  '
  jq -r --arg sha "$sha" "$filter" <<< "$runs_json"
}

verify_required_job() {
  local run_id="$1"
  local run_attempt="$2"
  local jobs_json
  local filter
  local conclusion

  jobs_json="$(
    gh api --paginate \
      "repos/$REPOSITORY/actions/runs/$run_id/attempts/$run_attempt/jobs"
  )"
  filter='\
    [.jobs[] | select(.name == $name)] |\
    if length == 1 then .[0].conclusion else "invalid" end\
  '
  conclusion="$(
    jq -r --arg name "$required_job" "$filter" <<< "$jobs_json"
  )"
  [[ "$conclusion" == "success" ]] || fail \
    "required landed integration aggregate was not uniquely successful"
}

: "${EVENT_NAME:?event name is required}"
: "${REPOSITORY:?repository is required}"
: "${CONTRACTS_FILE:?contracts file is required}"
: "${MANUAL_SOURCE:=}"
: "${EVENT_RUN_ID:=}"

integration_workflow="$(
  jq -r '.promotion.integration_workflow // empty' "$CONTRACTS_FILE"
)"
required_job="$(
  jq -r '.promotion.required_integration_job // empty' "$CONTRACTS_FILE"
)"
[[ -n "$integration_workflow" && -n "$required_job" ]] || fail \
  "promotion integration evidence contract is incomplete"

if [[ "$EVENT_NAME" == "workflow_run" ]]; then
  run_id="$EVENT_RUN_ID"
  [[ -n "$run_id" ]] || fail "workflow_run id is missing"
  run_json="$(gh api "repos/$REPOSITORY/actions/runs/$run_id")"
  source="$(jq -r '.head_branch' <<< "$run_json")"
  source_sha="$(jq -r '.head_sha' <<< "$run_json")"
else
  source="$MANUAL_SOURCE"
  case "$source" in
    main|beta) ;;
    *)
      echo "illegal manual promotion source" >&2
      exit 2
      ;;
  esac

  source_sha="$(branch_sha "$source")"
  run_id="$(latest_successful_run_for_sha "$source" "$source_sha")"
  [[ -n "$run_id" ]] || fail \
    "manual retry refused: current source SHA has no successful integration run"
  run_json="$(gh api "repos/$REPOSITORY/actions/runs/$run_id")"
fi

case "$source" in
  main) destination="beta" ;;
  beta) destination="master" ;;
  *)
    echo "illegal promotion source" >&2
    exit 2
    ;;
esac

run_name="$(jq -r '.name' <<< "$run_json")"
run_event="$(jq -r '.event' <<< "$run_json")"
run_conclusion="$(jq -r '.conclusion' <<< "$run_json")"
run_path="$(jq -r '.path' <<< "$run_json")"
run_attempt="$(jq -r '.run_attempt' <<< "$run_json")"
run_branch="$(jq -r '.head_branch' <<< "$run_json")"
run_sha="$(jq -r '.head_sha' <<< "$run_json")"

[[ "$run_name" == "SultanC Integration" ]] || fail \
  "integration run has unexpected workflow name"
[[ "$run_event" == "push" && "$run_conclusion" == "success" ]] || fail \
  "integration run event/conclusion is not eligible"
[[ "$run_path" == ".github/workflows/$integration_workflow" ]] || fail \
  "unexpected integration workflow path: $run_path"
[[ "$run_branch" == "$source" && "$run_sha" == "$source_sha" ]] || fail \
  "integration evidence branch/SHA mismatch"

verify_required_job "$run_id" "$run_attempt"

current_source="$(branch_sha "$source")"
[[ "$current_source" == "$source_sha" ]] || fail \
  "source moved after qualification; newer SHA must qualify"
destination_sha="$(branch_sha "$destination")"

{
  printf 'source=%s\n' "$source"
  printf 'destination=%s\n' "$destination"
  printf 'source_sha=%s\n' "$source_sha"
  printf 'destination_sha=%s\n' "$destination_sha"
  printf 'trigger_run_id=%s\n' "$run_id"
  printf 'trigger_run_attempt=%s\n' "$run_attempt"
} >> "$GITHUB_OUTPUT"
