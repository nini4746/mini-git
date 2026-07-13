#!/bin/bash
# Plan 01: init, SHA-1, hash-object, cat-file (blob)
source "$(dirname "$0")/lib.sh"
echo "== 01 object store =="

# SHA-1 self-test vectors (incl. "abc" -> a9993e...)
check "SHA-1 RFC 3174 test vectors" "$MG" sha1-test

T=$(mktemp -d); cd "$T"

# init compatibility
"$MG" init >/dev/null
check "git accepts minigit-init repo (git status)" git status
eq "HEAD content is 'ref: refs/heads/main'" "$(cat .git/HEAD)" "ref: refs/heads/main"
# re-init must fail
"$MG" init >/dev/null 2>&1; eq "re-init fails with nonzero exit" "$?" "1"

# hash-object matches git
printf 'hello minigit\n' > f.txt
eq "hash-object == git hash-object" "$("$MG" hash-object f.txt)" "$(git hash-object f.txt)"

# empty blob hash
printf '' > empty
eq "empty blob hash" "$("$MG" hash-object empty)" "e69de29bb2d1d6434b8b29ae775ad8c2e48c5391"

# -w stores byte-identical (inflated) object vs git
SHA=$("$MG" hash-object -w f.txt)
GT=$(mktemp -d); (cd "$GT" && git init -q && git hash-object -w "$T/f.txt" >/dev/null)
MGOBJ=".git/objects/${SHA:0:2}/${SHA:2}"
GOBJ="$GT/.git/objects/${SHA:0:2}/${SHA:2}"
if cmp -s <(python3 -c "$INFLATE" "$MGOBJ") <(python3 -c "$INFLATE" "$GOBJ"); then
    pass "-w object inflates byte-identical to git's"
else
    fail "-w object inflates byte-identical to git's"
fi

# idempotent -w
BEFORE=$(md5 -q "$MGOBJ" 2>/dev/null || md5sum "$MGOBJ")
"$MG" hash-object -w f.txt >/dev/null
AFTER=$(md5 -q "$MGOBJ" 2>/dev/null || md5sum "$MGOBJ")
eq "second -w leaves object unchanged" "$BEFORE" "$AFTER"

# cat-file blob
eq "cat-file -t blob" "$("$MG" cat-file -t "$SHA")" "blob"
eq "cat-file -s blob size" "$("$MG" cat-file -s "$SHA")" "14"
eq "cat-file -p blob content" "$("$MG" cat-file -p "$SHA")" "hello minigit"
eq "cat-file -t matches git" "$("$MG" cat-file -t "$SHA")" "$(cd "$GT" && git cat-file -t "$SHA")"

# minigit reads a git-created blob
GSHA=$(cd "$GT" && printf 'made by git\n' | git hash-object -w --stdin)
# copy git's object into minigit store so minigit can read it
mkdir -p ".git/objects/${GSHA:0:2}"
cp "$GT/.git/objects/${GSHA:0:2}/${GSHA:2}" ".git/objects/${GSHA:0:2}/${GSHA:2}"
eq "reads git-created blob" "$("$MG" cat-file -p "$GSHA")" "made by git"

# bad hash errors
"$MG" cat-file -p 1111111111111111111111111111111111111111 >/dev/null 2>&1
eq "missing object errors nonzero" "$?" "1"

cd /; rm -rf "$T" "$GT"
finish
