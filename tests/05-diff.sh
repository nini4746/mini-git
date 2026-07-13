#!/bin/bash
# Plan 05: Myers diff (optional)
source "$(dirname "$0")/lib.sh"
echo "== 05 myers diff =="

T=$(mktemp -d); cd "$T"

# identical files -> empty output
printf 'a\nb\nc\n' > x.txt
OUT=$("$MG" diff x.txt x.txt)
eq "identical files produce empty diff" "$OUT" ""

# one delete + one add -> edit count matches git minimal edit count
printf 'a\nb\nc\n' > p.txt
printf 'a\nX\nc\n' > q.txt
EDITS=$("$MG" diff p.txt q.txt | grep -c '^[-+]')
NUM=$(git diff --no-index --numstat p.txt q.txt | awk '{print $1+$2}')
eq "edit count == git minimal edit count" "$EDITS" "$NUM"

# one side empty -> all additions
printf '' > e.txt
ADD=$("$MG" diff e.txt x.txt)
eq "empty vs full -> all additions" "$ADD" "$(printf '+a\n+b\n+c')"

# full vs empty -> all deletions
DEL=$("$MG" diff x.txt e.txt)
eq "full vs empty -> all deletions" "$DEL" "$(printf '\055a\n\055b\n\055c')"

cd /; rm -rf "$T"
finish
