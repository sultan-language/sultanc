#!/bin/sh
# Runs the stdlib tests that meet host memory (the macOS foreign-call
# bridge: errno, getcwd, posix_spawnp, readdir, getaddrinfo, the current
# executable) natively and interpreted, and prints one row per test:
#   test  native-exit  interpreted-exit  same|different
# followed by the last 10 lines of the interpreted stderr of every
# difference. Each test should exit 42 both ways.
#
# Run from the repository root after building the compiler:
#   sh tools/tests/mac_interp_check.sh        (or bash / zsh)
# SC overrides the compiler (default ./build/sultanc). Everything the script
# writes goes under build/tmp/mac_interp_check/. It is plain POSIX sh: it
# runs under macOS's /bin/sh, bash 3.2 and zsh, and on Linux.

SC=${SC:-./build/sultanc}
OUT=build/tmp/mac_interp_check

if [ ! -x "$SC" ]; then
    echo "mac_interp_check.sh: missing compiler $SC (build it, or set SC)" >&2
    exit 2
fi
if [ ! -d tests/en/stdlib ]; then
    echo "mac_interp_check.sh: run from the repository root" >&2
    exit 2
fi
rm -rf "$OUT"
mkdir -p "$OUT"
: > "$OUT/differences.txt"

same=0
different=0

printf '%-46s %7s %7s  %s\n' "test" "native" "interp" "result"
printf '%-46s %7s %7s  %s\n' "----" "------" "------" "------"

# One test: $1 = language (ar|en), $2 = directory under stdlib, $3 = name.
check_one() {
    lang=$1
    dir=$2
    name=$3
    src="tests/$lang/stdlib/$dir/$name.sn"
    base="$OUT/${lang}_${dir}_${name}"
    label="$lang/$dir/$name"
    if [ ! -f "$src" ]; then
        printf '%-46s %7s %7s  %s\n' "$label" "-" "-" "missing"
        different=$((different + 1))
        return
    fi
    if "$SC" "$src" -o "$base.exe" > "$base.build" 2>&1; then
        "$base.exe" < /dev/null > "$base.native.out" 2> "$base.native.err"
        native=$?
    else
        native=build-failed
    fi
    "$SC" interpret "$src" < /dev/null > "$base.interp.out" 2> "$base.interp.err"
    interp=$?
    if [ "$native" = "$interp" ] && cmp -s "$base.native.out" "$base.interp.out"; then
        result=same
        same=$((same + 1))
    else
        result=different
        different=$((different + 1))
        {
            echo "== $label (native $native, interpreted $interp)"
            if [ "$native" = "build-failed" ]; then
                echo "-- native build output:"
                tail -n 10 "$base.build"
            fi
            if ! cmp -s "$base.native.out" "$base.interp.out" 2>/dev/null; then
                echo "-- stdout differs (see $base.native.out and $base.interp.out)"
            fi
            echo "-- last 10 lines of interpreted stderr:"
            tail -n 10 "$base.interp.err"
            echo
        } >> "$OUT/differences.txt"
    fi
    printf '%-46s %7s %7s  %s\n' "$label" "$native" "$interp" "$result"
}

for lang in en ar; do
    for name in socket_names env_variables process_kill unix_sockets fs_lock \
        process_spawn env_set_remove socket_timeout fs_links_attributes \
        process_command env_executable host_bridge; do
        check_one "$lang" behavior "$name"
    done
    check_one "$lang" error convert_os
done

echo
echo "same=$same different=$different (outputs in $OUT)"
if [ "$different" -ne 0 ]; then
    echo
    cat "$OUT/differences.txt"
    exit 1
fi
exit 0
