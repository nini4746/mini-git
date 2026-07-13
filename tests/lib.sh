# Shared helpers for minigit compatibility tests. Sourced by each test.
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
MG="$ROOT/minigit"
INFLATE='import zlib,sys; sys.stdout.buffer.write(zlib.decompress(open(sys.argv[1],"rb").read()))'

FAILS=0
CHECKS=0

pass() { CHECKS=$((CHECKS+1)); echo "  ok   - $1"; }
fail() { CHECKS=$((CHECKS+1)); FAILS=$((FAILS+1)); echo "  FAIL - $1"; }

# check "<desc>" <cmd...>  -> pass if cmd exits 0
check() {
    local desc="$1"; shift
    if "$@" >/dev/null 2>&1; then pass "$desc"; else fail "$desc"; fi
}

# eq "<desc>" "<got>" "<want>"
eq() {
    if [ "$2" = "$3" ]; then pass "$1"; else fail "$1 (got '$2' want '$3')"; fi
}

# Fixed identity for deterministic byte comparison with real git.
set_fixed_env() {
    export GIT_AUTHOR_NAME="Alice Dev" GIT_AUTHOR_EMAIL="alice@example.com"
    export GIT_COMMITTER_NAME="Alice Dev" GIT_COMMITTER_EMAIL="alice@example.com"
    export GIT_AUTHOR_DATE="1735689600 +0900" GIT_COMMITTER_DATE="1735689600 +0900"
}

finish() {
    echo "---- $(basename "$0"): $((CHECKS-FAILS))/$CHECKS checks passed ----"
    [ "$FAILS" -eq 0 ]
}
