#!/bin/bash
# Plan 02: index, add, tree
source "$(dirname "$0")/lib.sh"
echo "== 02 index / add / tree =="

# Build the same fixture in a minigit repo and a real-git repo, compare.
MGR=$(mktemp -d); GR=$(mktemp -d)

mkfix() {
    local d=$1
    printf 'hello world\n' > "$d/a.txt"
    printf 'second\n' > "$d/b file.txt"
    mkdir -p "$d/dir/sub"; printf 'nested\n' > "$d/dir/sub/file.txt"
    printf 'run\n' > "$d/script.sh"; chmod 755 "$d/script.sh"
    printf 'korean\n' > "$d/한글.txt"
    printf 'F\n' > "$d/foo.txt"
    mkdir -p "$d/foo"; printf 'bar\n' > "$d/foo/bar"
}
mkfix "$MGR"; mkfix "$GR"
FILES=(a.txt "b file.txt" dir/sub/file.txt script.sh 한글.txt foo.txt foo/bar)

cd "$MGR"; "$MG" init >/dev/null; "$MG" add "${FILES[@]}"
cd "$GR"; git init -q -b main; git add "${FILES[@]}"

# ls-files --stage identical (mode, hash, stage, path)  [covers unicode/space/nested]
eq "ls-files --stage matches git (mode/hash/stage/path)" \
   "$(cd "$MGR" && git ls-files --stage)" "$(cd "$GR" && git ls-files --stage)"

# re-add updates in place (no duplicate)
cd "$MGR"; printf 'CHANGED\n' > a.txt; "$MG" add a.txt
N=$(git ls-files | grep -c '^a.txt$')
eq "re-add updates entry without duplicate" "$N" "1"
# restore for tree comparison
printf 'hello world\n' > a.txt; "$MG" add a.txt

# .git path rejected
cd "$MGR"; "$MG" add .git/HEAD >/dev/null 2>&1
eq ".git path rejected (nonzero)" "$?" "1"

# write-tree hash matches git write-tree
MG_TREE=$(cd "$MGR" && "$MG" write-tree)
G_TREE=$(cd "$GR" && git write-tree)
eq "write-tree hash == git write-tree (sort/mode rules)" "$MG_TREE" "$G_TREE"

# foo / foo.txt / foo/bar ordering encoded in tree bytes -> equal hash already
# proves the '/' virtual-suffix sort rule. Also check dir mode + file modes.
TREE_TXT=$(cd "$GR" && git -c core.quotePath=false cat-file -p "$G_TREE")
echo "$TREE_TXT" | grep -q "^040000 tree .*	foo$"  && pass "dir entry mode is 040000 (stored 40000)" || fail "dir entry mode is 040000"
echo "$TREE_TXT" | grep -q "^100755 blob .*	script.sh$" && pass "executable file mode 100755" || fail "executable file mode 100755"
echo "$TREE_TXT" | grep -q "^100644 blob .*	a.txt$" && pass "regular file mode 100644" || fail "regular file mode 100644"

# cat-file -p tree matches git (quotePath off so unicode name compares raw)
eq "cat-file -p tree matches git" \
   "$(cd "$MGR" && "$MG" cat-file -p "$MG_TREE")" \
   "$(cd "$GR" && git -c core.quotePath=false cat-file -p "$G_TREE")"

cd /; rm -rf "$MGR" "$GR"
finish
