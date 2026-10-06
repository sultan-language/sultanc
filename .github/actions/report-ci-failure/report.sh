#!/usr/bin/env bash
set -euo pipefail

# Create or update one deduplicated issue for each completed failed CI job.
# Diagnostics are bounded and sanitized before leaving the Actions log boundary.

readonly MAX_ERROR_LENGTH=600
ERROR_PATTERN='(^|[[:space:]])(error(\[[^]]+\])?|fatal|panic|failure|failed|خطأ)'
ERROR_PATTERN+="([[:space:]:\\[]|$)|AssertionError|No such file or directory"
ERROR_PATTERN+="|command not found"
readonly ERROR_PATTERN

fail() {
  echo "$*" >&2
  exit 1
}

list_labels() {
  gh label list \
    --repo "$SULTANC_REPOSITORY" \
    --limit 1000 \
    --json name \
    --jq '.[].name'
}

ensure_label() {
  local label="$1"
  local color="$2"
  local description="$3"
  local existing_labels

  existing_labels="$(list_labels)"
  if grep -Fxq "$label" <<< "$existing_labels"; then
    return 0
  fi

  if gh label create "$label" \
    --repo "$SULTANC_REPOSITORY" \
    --color "$color" \
    --description "$description" \
    >/dev/null 2>&1; then
    return 0
  fi

  # Another reporter may have created the same label concurrently.
  existing_labels="$(list_labels)"
  grep -Fxq "$label" <<< "$existing_labels"
}

sanitize_error() {
  sed -E \
    -e 's/(github_pat_[A-Za-z0-9_]{10,})/[REDACTED]/g' \
    -e 's/(gh[pousr]_[A-Za-z0-9_]{10,})/[REDACTED]/g' \
    -e 's/((Bearer|token|password|secret)[[:space:]=:]+)[^[:space:]]+/\1[REDACTED]/Ig' \
    -e 's/`/'"'"'/g'
}

first_meaningful_error() {
  local job_id="$1"
  local logs
  local line

  logs="$(
    gh run view "$SULTANC_RUN_ID" \
      --repo "$SULTANC_REPOSITORY" \
      --job "$job_id" \
      --log-failed \
      2>/dev/null || true
  )"

  if [[ -z "$logs" ]]; then
    logs="$(
      gh run view "$SULTANC_RUN_ID" \
        --repo "$SULTANC_REPOSITORY" \
        --job "$job_id" \
        --log \
        2>/dev/null || true
    )"
  fi

  # gh prefixes each line with job, step, and timestamp columns.
  line="$(
    printf '%s\n' "$logs" \
      | cut -f4- \
      | grep -Eiv 'Process completed with exit code|0 errors|no errors' \
      | grep -Eim1 "$ERROR_PATTERN" \
      || true
  )"

  if [[ -z "$line" ]]; then
    line="No concise diagnostic found; see the linked Actions run."
  fi

  printf '%s' "$line" \
    | sanitize_error \
    | tr '\r\n' '  ' \
    | cut -c1-"$MAX_ERROR_LENGTH"
}

stable_hash() {
  printf '%s' "$1" | sha256sum | awk '{print $1}'
}

trim() {
  local value="$1"
  value="${value#${value%%[![:space:]]*}}"
  value="${value%${value##*[![:space:]]}}"
  printf '%s\n' "$value"
}

failed_jobs() {
  local filter

  filter='\
    .jobs[] |\
    select(.conclusion == "failure") |\
    [\
      .id,\
      .name,\
      (.runner_name // "unknown"),\
      (.labels | join(", ")),\
      ([.steps[]? | select(.conclusion == "failure") | .name][0] // "unknown")\
    ] |\
    @tsv\
  '

  gh api --paginate \
    --method GET \
    "repos/$SULTANC_REPOSITORY/actions/runs/$SULTANC_RUN_ID/jobs" \
    -f per_page=100 \
    --jq "$filter"
}

issue_for_failure_key() {
  local failure_hash="$1"
  local issues
  local marker="sultanc-ci-failure-key:$failure_hash"

  issues="$(
    gh api --paginate \
      --method GET \
      "repos/$SULTANC_REPOSITORY/issues" \
      -f state=open \
      -f labels='ci,automated' \
      -f per_page=100
  )"

  jq -r \
    --arg marker "$marker" \
    '.[] |\
     select(.pull_request == null) |\
     select((.body // "") | contains($marker)) |\
     .number' \
    <<< "$issues" \
    | head -n1
}

build_label_arguments() {
  local label
  local existing="|"

  label_create_args=()
  label_edit_args=()

  for label in "${labels[@]}"; do
    [[ "$existing" == *"|$label|"* ]] && continue
    existing+="$label|"
    label_create_args+=(--label "$label")
    label_edit_args+=(--add-label "$label")
  done
}

write_issue_body() {
  local body_file="$1"

  cat > "$body_file" <<EOF_BODY
$marker
Automated SultanC CI failure report.
The same open failure key is updated instead of creating duplicate issues.

| Field | Value |
| --- | --- |
| Workflow | \`$SULTANC_WORKFLOW\` |
| Failed job | \`$job_name\` |
| Failed step | \`$failed_step\` |
| Branch | \`$SULTANC_BRANCH\` |
| Candidate SHA | \`$SULTANC_SHA\` |
| Harness SHA | \`$SULTANC_HARNESS_SHA\` |
| Target | \`$target\` |
| Platform | \`$runner_labels\` |
| Runner | \`$runner_name\` |
| Run | $run_url |

**First meaningful error**
> $first_error

**Stable failure key**

\`$failure_identity\`
EOF_BODY
}

upsert_issue() {
  local body_file="$1"
  local issue_number

  issue_number="$(issue_for_failure_key "$failure_hash")"
  if [[ -n "$issue_number" ]]; then
    gh issue edit "$issue_number" \
      --repo "$SULTANC_REPOSITORY" \
      --title "$issue_title" \
      --body-file "$body_file" \
      "${label_edit_args[@]}" \
      >/dev/null
    printf 'Updated CI issue #%s for %s\n' "$issue_number" "$job_name"
    return
  fi

  gh issue create \
    --repo "$SULTANC_REPOSITORY" \
    --title "$issue_title" \
    --body-file "$body_file" \
    "${label_create_args[@]}" \
    >/dev/null
  printf 'Created CI issue for %s\n' "$job_name"
}

: "${SULTANC_REPOSITORY:?SULTANC_REPOSITORY is required}"
: "${SULTANC_RUN_ID:?SULTANC_RUN_ID is required}"
: "${SULTANC_WORKFLOW:?SULTANC_WORKFLOW is required}"
: "${SULTANC_BRANCH:?SULTANC_BRANCH is required}"
: "${SULTANC_SHA:?SULTANC_SHA is required}"
: "${SULTANC_HARNESS_SHA:?SULTANC_HARNESS_SHA is required}"
: "${SULTANC_COMPONENT_LABELS:=}"
: "${SULTANC_SERVER_URL:=https://github.com}"

command -v gh >/dev/null 2>&1 || fail "gh is required"
command -v jq >/dev/null 2>&1 || fail "jq is required"
command -v sha256sum >/dev/null 2>&1 || fail "sha256sum is required"

ensure_label "ci" "1d76db" "Continuous integration failure"
ensure_label \
  "automated" \
  "6f42c1" \
  "Created or maintained automatically by GitHub Actions"

IFS=',' read -r -a raw_component_labels <<< "$SULTANC_COMPONENT_LABELS"
component_labels=()
for raw_label in "${raw_component_labels[@]}"; do
  component_label="$(trim "$raw_label")"
  [[ -n "$component_label" ]] || continue
  component_labels+=("$component_label")
  ensure_label "$component_label" "0e8a16" "SultanC CI component"
done

failed_jobs_output="$(failed_jobs)"
[[ -n "$failed_jobs_output" ]] || exit 0

while IFS=$'\t' read -r \
  job_id job_name runner_name runner_labels failed_step; do
  [[ -n "$job_id" ]] || continue

  if [[ -n "${SULTANC_JOB_FILTER:-}" && \
        "$job_name" != *"$SULTANC_JOB_FILTER"* ]]; then
    continue
  fi

  target="n/a"
  case "$job_name" in
    *" / x86_64-linux") target="x86_64-linux" ;;
    *" / arm64-darwin") target="arm64-darwin" ;;
  esac

  failure_identity="$SULTANC_WORKFLOW|$SULTANC_BRANCH|$job_name|$target"
  failure_hash="$(stable_hash "$failure_identity")"
  marker="<!-- sultanc-ci-failure-key:$failure_hash -->"
  first_error="$(first_meaningful_error "$job_id")"
  run_url="$SULTANC_SERVER_URL/$SULTANC_REPOSITORY/actions/runs/$SULTANC_RUN_ID"
  issue_title="[CI] $SULTANC_WORKFLOW — $job_name"
  labels=("ci" "automated" "${component_labels[@]}")

  if [[ "$target" != "n/a" ]]; then
    ensure_label "$target" "c5def5" "SultanC target"
    labels+=("$target")
  fi

  # Attribute bootstrap only when the failed step or diagnostic identifies it.
  if printf '%s\n%s\n' "$failed_step" "$first_error" \
    | grep -Eiq 'stage0|bootstrap'; then
    ensure_label "bootstrap" "5319e7" "Stage0 or bootstrap boundary"
    labels+=("bootstrap")
  fi

  build_label_arguments
  body_file="$(mktemp "${RUNNER_TEMP:-/tmp}/sultanc-ci-issue.XXXXXX")"
  trap 'rm -f "$body_file"' EXIT
  write_issue_body "$body_file"
  upsert_issue "$body_file"
  rm -f "$body_file"
  trap - EXIT
done <<< "$failed_jobs_output"
