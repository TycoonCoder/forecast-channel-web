#!/usr/bin/env bash
# Forecast Channel web — publish to GitHub + Pages. Idempotent; safe to re-run.
set -euo pipefail
cd "$(dirname "$0")"

REPO=forecast-channel-web
OWNER=TycoonCoder

# --- git identity fallbacks (only set if missing) ---
git config user.name  >/dev/null 2>&1 || git config user.name  "TycoonCoder"
git config user.email >/dev/null 2>&1 || git config user.email "TycoonCoder@users.noreply.github.com"

# --- commit everything ---
if [ ! -d .git ]; then git init -b main; fi
git add -A
git commit -m "Forecast Channel web recreation" || echo "(nothing new to commit)"

# --- create/push repo ---
if gh repo view "$OWNER/$REPO" >/dev/null 2>&1; then
  git remote -v | grep -q origin || git remote add origin "https://github.com/$OWNER/$REPO.git"
  git push -u origin main
else
  gh repo create "$REPO" --public --source=. --remote=origin --push
fi

# --- enable Pages (main / root) ---
gh api -X POST "repos/$OWNER/$REPO/pages" \
    -f build_type=legacy -f 'source[branch]=main' -f 'source[path]=/' \
  && echo "Pages enabled." \
  || echo "(Pages may already be enabled — checking)"

gh api "repos/$OWNER/$REPO/pages" --jq '"Live at: " + .html_url'
