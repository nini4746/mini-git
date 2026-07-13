#!/bin/bash
# Plan 04: checkout
source "$(dirname "$0")/lib.sh"
echo "== 04 checkout =="
set_fixed_env

T=$(mktemp -d); cd "$T"
"$MG" init >/dev/null

printf 'version one\n' > a.txt
mkdir -p dir/sub; printf 'nested one\n' > dir/sub/n.txt
printf 'exec\n' > run.sh; chmod 755 run.sh
"$MG" add a.txt dir/sub/n.txt run.sh >/dev/null
A=$("$MG" commit -m "A")

printf 'version two\n' > a.txt
printf 'nested two\n' > dir/sub/n.txt
"$MG" add a.txt dir/sub/n.txt >/dev/null
B=$("$MG" commit -m "B")

eq "working tree currently at B" "$(cat a.txt)" "version two"

"$MG" checkout "$A" >/dev/null
eq "checkout A restores a.txt bytes" "$(cat a.txt)" "version one"
eq "checkout A restores nested file" "$(cat dir/sub/n.txt)" "nested one"
[ -x run.sh ] && pass "checkout restores 100755 exec bit" || fail "checkout restores exec bit"
eq "HEAD is detached at commit A" "$(cat .git/HEAD)" "$A"

# bad hash: worktree untouched, error
cp a.txt a.before
"$MG" checkout 2222222222222222222222222222222222222222 >/dev/null 2>&1
eq "bad-hash checkout errors nonzero" "$?" "1"
eq "bad-hash checkout leaves worktree untouched" "$(cat a.txt)" "$(cat a.before)"

cd /; rm -rf "$T"
finish
