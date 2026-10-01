#!/usr/bin/env bash
set -euo pipefail

# Keep issue text bounded and single-line even when a tool emits verbose diagnostics.
readonly MAX_ERROR_LENGTH=600

# Create a label only when the repository does not already define it.
ensure_label() {
  local label="$1"
  local color="$2"
  local description="$3"

  local existing_labels
  existing_labels="$(gh label list --repo "$SULTANC_REPOSITORY" --limit 1000 --json name --jq '.[].name')"
  if grep -Fxq "$label" <<< "$existing_labels"; then
    return 0
  fi

  if gh label create "$label" \
    --repo "$SULTANC_REPOSITORY" \
    --color "$color" \
    --description "$description" >/dev/null 2>&1; then
    return 0
  fi

  # Tolerate another failure reporter creating the same label concurrently, but surface real errors.
  existing_labels="$(gh label list --repo "$SULTANC_REPOSITORY" --limit 1000 --json name --jq '.[].name')"
  grep -Fxq "$label" <<< "$existing_labels"
}

# Remove common credential forms defensively before an error line leaves Actions logs.
sanitize_error() {
  sed -E \
    -e 's/(github_pat_[A-Za-z0-9_]{10,})/[REDACTED]/g' \
    -e 's/(gh[pousr]_[A-Za-z0-9_]{10,})/[REDACTED]/g' \
    -e 's/((Bearer|token|password|secret)[[:space:]=:]+)[^[:space:]]+/\1[REDACTED]/Ig' \
    -e 's/`/'"'"'/g'
}

# Extract the first diagnostic-like line from one completed failed job.
first_meaningful_error() {
  local job_id="$1"
  local logs
  local line

  logs="$(gh run view "$SULTANC_RUN_ID" \
    --repo "$SULTANC_REPOSITORY" \
    --job "$job_id" \
    --log-failed 2>/dev/null || true)"

  if [[ -z "$logs" ]]; then
    logs="$(gh run view "$SULTANC_RUN_ID" \
      --repo "$SULTANC_REPOSITORY" \
      --job "$job_id" \
      --log 2>/dev/null || true)"
  fi

  # gh prefixes log lines with job, step, and timestamp fields; keep the emitted message when present.
  line="$(printf '%s\n' "$logs" \
    | cut -f4- \
    | grep -Eiv 'Process completed with exit code|0 errors|no errors' \
    | grep -Eim1 '(^|[[:space:]])(error(\[[^]]+\])?|fatal|panic|failure|failed|خطأ)([[:space:]:\[]|$)|AssertionError|No such file or directory|command not found' \
    || true)"

  if [[ -z "$line" ]]; then
    line="No concise diagnostic line could be extracted; see the linked Actions run."
  fi

  printf '%s' "$line" \
    | sanitize_error \
    | tr '\r\n' '  ' \
    | cut -c1-"$MAX_ERROR_LENGTH"
}

# Compute SHA-256 with the tool available on GitHub-hosted Linux runners.
stable_hash() {
  local value="$1"
  printf '%s' "$value" | sha256sum | awk '{print $1}'
}

# Ensure the labels shared by every automated CI issue exist without changing existing labels.
ensure_label "ci" "1d76db" "Continuous integration failure"
ensure_label "automated" "6f42c1" "Created or maintained automatically by GitHub Actions"

# Materialize caller-owned labels once so every failed job receives the same component identity.
IFS=',' read -r -a component_labels <<< "$SULTANC_COMPONENT_LABELS"
for component_label in "${component_labels[@]}"; do
  component_label="${component_label#${component_label%%[![:space:]]*}}"
  component_label="${component_label%${component_label##*[![:space:]]}}"
  [[ -n "$component_label" ]] || continue
  ensure_label "$component_label" "0e8a16" "SultanC CI component"
done

# Query completed failures from upstream jobs in the current run; this reporter job is still in progress and is excluded.
failed_jobs="$(gh api --paginate \
  --method GET \
  "repos/$SULTANC_REPOSITORY/actions/runs/$SULTANC_RUN_ID/jobs" \
  -f per_page=100 \
  --jq '.jobs[] | select(.conclusion == "failure") | [.id, .name, (.runner_name // "unknown"), (.labels | join(", ")), ([.steps[]? | select(.conclusion == "failure") | .name][0] // "unknown")] | @tsv')"

# A defensive no-op keeps the action safe if GitHub reports no completed failed jobs.
[[ -n "$failed_jobs" ]] || exit 0

while IFS=$'\t' read -r job_id job_name runner_name runner_labels failed_step; do
  [[ -n "$job_id" ]] || continue

  target="n/a"
  case "$job_name" in
    *" / x86_64-linux") target="x86_64-linux" ;;
    *" / arm64-darwin") target="arm64-darwin" ;;
  esac

  # Workflow + job + target remains stable across commits and workflow-run identifiers.
  failure_identity="$SULTANC_WORKFLOW|$job_name|$target"
  failure_hash="$(stable_hash "$failure_identity")"
  marker="<!-- sultanc-ci-failure-key:$failure_hash -->"
  first_error="$(first_meaningful_error "$job_id")"
  run_url="$SULTANC_SERVER_URL/$SULTANC_REPOSITORY/actions/runs/$SULTANC_RUN_ID"
  issue_title="[CI] $SULTANC_WORKFLOW — $job_name"

  labels=("ci" "automated")
  for component_label in "${component_labels[@]}"; do
    component_label="${component_label#${component_label%%[![:space:]]*}}"
    component_label="${component_label%${component_label##*[![:space:]]}}"
    [[ -n "$component_label" ]] && labels+=("$component_label")
  done

  if [[ "$target" != "n/a" ]]; then
    ensure_label "$target" "c5def5" "SultanC target"
    labels+=("$target")
  fi

  # Add bootstrap ownership only when the failed step or diagnostic explicitly identifies that boundary.
  if printf '%s\n%s\n' "$failed_step" "$first_error" | grep -Eiq 'stage0|bootstrap'; then
    ensure_label "bootstrap" "5319e7" "Stage0 or bootstrap boundary"
    labels+=("bootstrap")
  fi

  label_create_args=()
  label_edit_args=()
  declare -A seen_labels=()
  for label in "${labels[@]}"; do
    [[ -n "${seen_labels[$label]:-}" ]] && continue
    seen_labels[$label]=1
    label_create_args+=(--label "$label")
    label_edit_args+=(--add-label "$label")
  done
  unset seen_labels

  body_file="$(mktemp "${RUNNER_TEMP:-/tmp}/sultanc-ci-issue.XXXXXX")"
  trap 'rm -f "$body_file"' EXIT

  cat > "$body_file" <<EOF_BODY
$marker
Automated SultanC CI failure report. This issue is updated when the same failure key recurs while the issue remains open.

| Field | Value |
| --- | --- |
| Workflow | \`$SULTANC_WORKFLOW\` |
| Failed job | \`$job_name\` |
| Failed step | \`$failed_step\` |
| Branch | \`$SULTANC_BRANCH\` |
| Commit | \`$SULTANC_SHA\` |
| Target | \`$target\` |
| Platform | \`$runner_labels\` |
| Runner | \`$runner_name\` |
| Run | $run_url |

**First meaningful error**

> $first_error

**Stable failure key**

\`$failure_identity\`
EOF_BODY

  # Scan open automated CI issues directly instead of relying on eventually-consistent issue search indexing.
  issue_number="$(gh api --paginate \
    --method GET \
    "repos/$SULTANC_REPOSITORY/issues" \
    -f state=open \
    -f labels='ci,automated' \
    -f per_page=100 \
    --jq ".[] | select(.pull_request == null) | select((.body // \"\") | contains(\"sultanc-ci-failure-key:$failure_hash\")) | .number" \
    | head -n1)"

  if [[ -n "$issue_number" ]]; then
    gh issue edit "$issue_number" \
      --repo "$SULTANC_REPOSITORY" \
      --title "$issue_title" \
      --body-file "$body_file" \
      "${label_edit_args[@]}" >/dev/null
    printf 'Updated CI issue #%s for %s\n' "$issue_number" "$job_name"
  else
    gh issue create \
      --repo "$SULTANC_REPOSITORY" \
      --title "$issue_title" \
      --body-file "$body_file" \
      "${label_create_args[@]}" >/dev/null
    printf 'Created CI issue for %s\n' "$job_name"
  fi

  rm -f "$body_file"
  trap - EXIT
done <<< "$failed_jobs"
