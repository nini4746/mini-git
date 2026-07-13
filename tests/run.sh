#!/bin/bash
# Master compatibility suite: runs plan tests 01-05, then the R9 win-condition
# scenarios, and reports a single pass/fail with an exit code.
DIR="$(cd "$(dirname "$0")" && pwd)"
source "$DIR/lib.sh"

TESTS_FAILED=0
run_test() {
    echo
    if bash "$DIR/$1"; then :; else TESTS_FAILED=$((TESTS_FAILED+1)); fi
}

if [ ! -x "$MG" ]; then
    echo "building minigit..."
    (cd "$ROOT" && make) || { echo "build failed"; exit 1; }
fi

run_test 01-objects.sh
run_test 02-index-tree.sh
run_test 03-commit-log.sh
run_test 04-checkout.sh
run_test 05-diff.sh

echo
echo "== R9 win-condition scenarios =="
set_fixed_env
SC=0; SP=0
sc_eq() { SC=$((SC+1)); if [ "$2" = "$3" ]; then SP=$((SP+1)); echo "  ok   - $1"; else echo "  FAIL - $1 (got '$2' want '$3')"; fi; }
sc_ok() { SC=$((SC+1)); if "$@"; then SP=$((SP+1)); echo "  ok   - $2"; else echo "  FAIL - $2"; fi; }

# Scenario A: minigit-only init/add/commit x3, then real git reads it.
MGR=$(mktemp -d); cd "$MGR"; "$MG" init >/dev/null
for i in 1 2 3; do
    printf 'line %s\n' "$i" >> log.txt
    "$MG" add log.txt >/dev/null
    "$MG" commit -m "commit $i" >/dev/null
done
GLOG=$(git log --format=%H)
MLOG=$("$MG" log | awk '/^commit /{print $2}')
sc_eq "win: real git log shows 3 commits in minigit repo, correct order" "$GLOG" "$MLOG"
sc_eq "win: exactly 3 commits" "$(echo "$GLOG" | grep -c .)" "3"

# Scenario B: git fsck clean
if git fsck --strict >/tmp/mg_fsck.$$ 2>&1; then FSCK_OK=yes; else FSCK_OK=no; fi
ERRS=$(grep -c -iE 'error|missing|corrupt|dangling|bad' /tmp/mg_fsck.$$ 2>/dev/null)
rm -f /tmp/mg_fsck.$$
sc_eq "win: git fsck reports 0 errors" "${ERRS:-0}" "0"

# Scenario C: identical inputs -> identical object set (path + inflated content).
GR=$(mktemp -d); cd "$GR"; git init -q -b main
for i in 1 2 3; do
    printf 'line %s\n' "$i" >> log.txt
    git add log.txt; git commit -q -m "commit $i"
done
# Compare the set of object paths
MGSET=$(cd "$MGR" && find .git/objects -type f | sort)
GSET=$(cd "$GR" && find .git/objects -type f | sort)
sc_eq "win: object path sets identical" "$MGSET" "$GSET"
# Compare inflated content of every object
allok=1
for rel in $MGSET; do
    if ! cmp -s <(python3 -c "$INFLATE" "$MGR/$rel") <(python3 -c "$INFLATE" "$GR/$rel"); then
        allok=0; echo "    content diff: $rel"
    fi
done
sc_eq "win: every object inflates byte-identically" "$allok" "1"

# must-NOT: minigit must never shell out to git
if grep -rEn '\b(system|popen|execl|execlp|execv|execvp)\b' "$ROOT/src" | grep -qv '//'; then
    sc_eq "must-NOT: no shelling out to git in src/" "found" "clean"
else
    sc_ok true "must-NOT: minigit never calls git internally"
fi

echo
echo "  win scenarios: $SP/$SC passed"
[ "$SP" -eq "$SC" ] || TESTS_FAILED=$((TESTS_FAILED+1))

cd /; rm -rf "$MGR" "$GR"

echo
echo "========================================"
if [ "$TESTS_FAILED" -eq 0 ]; then
    echo "ALL SUITES PASSED"
    exit 0
else
    echo "SUITES FAILED: $TESTS_FAILED"
    exit 1
fi
