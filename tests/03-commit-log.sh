#!/bin/bash
# Plan 03: commit, refs, log
source "$(dirname "$0")/lib.sh"
echo "== 03 commit / refs / log =="
set_fixed_env

MGR=$(mktemp -d); GR=$(mktemp -d)

# 3-commit scenario, identical inputs, fixed env -> byte-identical commits.
run_scenario() {
    local d=$1 tool=$2   # tool: "mg" or "git"
    cd "$d"
    if [ "$tool" = git ]; then git init -q -b main; else "$MG" init >/dev/null; fi
    for i in 1 2 3; do
        printf 'content %s\n' "$i" > file.txt
        if [ "$tool" = git ]; then
            git add file.txt; git commit -q -m "commit $i"
        else
            "$MG" add file.txt >/dev/null; "$MG" commit -m "commit $i" >/dev/null
        fi
    done
}
run_scenario "$MGR" mg
run_scenario "$GR" git

# commit hash list + order
MG_LIST=$(cd "$MGR" && "$MG" log | awk '/^commit /{print $2}')
G_LIST=$(cd "$GR" && git log --format=%H)
eq "commit hashes + order == git (byte-identical commits)" "$MG_LIST" "$G_LIST"

# ref file content = tip hash + newline
TIP=$(echo "$MG_LIST" | head -1)
eq "refs/heads/main content is tip hash" "$(cat "$MGR/.git/refs/heads/main")" "$TIP"

# first commit has no parent, later ones do
FIRST=$(echo "$MG_LIST" | tail -1)
SECOND=$(echo "$MG_LIST" | sed -n '2p')
cd "$MGR"
"$MG" cat-file -p "$FIRST" | grep -q '^parent ' && HASP=yes || HASP=no
eq "first commit has no parent line" "$HASP" "no"
PAR=$("$MG" cat-file -p "$SECOND" | awk '/^parent /{print $2}')
eq "second commit parent == first commit" "$PAR" "$FIRST"

# timestamp format "<epoch> <tz>"
"$MG" cat-file -p "$TIP" | grep -Eq '^author .* [0-9]+ [+-][0-9]{4}$' \
    && pass "author timestamp is '<epoch> <tz>'" || fail "author timestamp format"

# log output contains author name and message
LOG=$(cd "$MGR" && "$MG" log)
echo "$LOG" | grep -q "Alice Dev" && pass "log shows author name" || fail "log shows author name"
echo "$LOG" | grep -q "commit 3" && pass "log shows commit message" || fail "log shows message"

# git reads minigit repo
eq "git log on minigit repo == minigit log order" "$(cd "$MGR" && git log --format=%H)" "$MG_LIST"

# empty-index commit rejected
ER=$(mktemp -d); cd "$ER"; "$MG" init >/dev/null
"$MG" commit -m x >/dev/null 2>&1; eq "commit with empty index rejected" "$?" "1"

# log on repo with no commits errors
"$MG" log >/dev/null 2>&1; eq "log with no commits errors nonzero" "$?" "1"

cd /; rm -rf "$MGR" "$GR" "$ER"
finish
