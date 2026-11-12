#!/bin/sh
# Corpus check of the source formatter (compiler/format, docs/en/reference/formatter.md).
#
#   SC=<sultanc> tools/tests/format_corpus.sh [work-dir]
#
# 1. Copies the checkout to <work-dir>/original and <work-dir>/formatted and formats
#    compiler/ and library/std/ of the formatted copy.
# 2. Every formatted file must hold the same tokens, comments and marks as the
#    original (tools/tests/format_tokens.sn, the compiler's own lexer).
# 3. A second run must change nothing (--check).
# 4. Unless FORMAT_CORPUS_BUILD=0: Stage2 is built from both copies with $SC, and
#    the two compilers must write byte-identical objects for the same inputs
#    (compiler/main.sn among them), compiled from the original sources.
#
# Heavy: run it through the machine's build lock where one exists.
set -eu
root=$(cd "$(dirname "$0")/../.." && pwd)
SC=${SC:-$root/build/sultanc}
work=${1:-$(mktemp -d)}
mkdir -p "$work"
work=$(cd "$work" && pwd)
case "$SC" in /*) ;; *) SC=$(cd "$(dirname "$SC")" && pwd)/$(basename "$SC") ;; esac

fail() { echo "format corpus: $*" >&2; exit 1; }

echo "format corpus: building the tools"
(cd "$root" && "$SC" --backend=native tools/tests/format_files.sn -o "$work/format_files") || fail "format_files does not build"
(cd "$root" && "$SC" --backend=native tools/tests/format_tokens.sn -o "$work/format_tokens") || fail "format_tokens does not build"

copy_tree() {
    rm -rf "$1"
    mkdir -p "$1"
    (cd "$root" && git ls-files -z --cached --others --exclude-standard | xargs -0 cp --parents -t "$1")
}
copy_tree "$work/original"
copy_tree "$work/formatted"

echo "format corpus: formatting compiler/ and library/std/"
status=0
(cd "$work/formatted" && "$work/format_files" compiler library/std) || status=$?
[ "$status" -eq 0 ] || fail "the formatter failed on the corpus (status $status)"

echo "format corpus: comparing tokens"
files=$(cd "$work/original" && find compiler library/std -name '*.sn' | sort)
count=0
changed=0
for f in $files; do
    count=$((count + 1))
    if ! cmp -s "$work/original/$f" "$work/formatted/$f"; then
        changed=$((changed + 1))
        "$work/format_tokens" "$work/original/$f" "$work/formatted/$f" || fail "$f: tokens changed (status $?)"
    fi
done
echo "format corpus: $count files, $changed reformatted, every token kept"

echo "format corpus: idempotence"
(cd "$work/formatted" && "$work/format_files" --check compiler library/std) || fail "a second run would change the formatted corpus"

if [ "${FORMAT_CORPUS_BUILD:-1}" = "0" ]; then
    echo "format corpus: OK (no compiler build)"
    exit 0
fi

echo "format corpus: building Stage2 from both copies"
for side in original formatted; do
    (cd "$work/$side" && ./build.sh stage2 --bootstrap=existing --with="$SC" --backend=native "$work/sultanc-$side" > "$work/build-$side.log" 2>&1) \
        || fail "Stage2 does not build from the $side copy (see $work/build-$side.log)"
done

echo "format corpus: comparing the objects of both compilers"
capability="--module-source=compiler.backend.provider.llvm.llvm_codegen=compiler/backend/provider/llvm/unavailable.sn"
compile() {
    # $1 compiler, $2 output, $3 input, remaining: extra options
    cc_bin=$1; out=$2; input=$3; shift 3
    (cd "$work/original" && "$cc_bin" --backend=native --emit=obj "$@" -o "$out" "$input")
}
for input in compiler/main.sn tests/en/programs/adt_command_pipeline.sn tests/ar/programs/adt_command_pipeline.sn tests/en/programs/sieve_eratosthenes.sn tools/tests/format_files.sn; do
    name=$(echo "$input" | tr '/' '_')
    extra=""
    [ "$input" = compiler/main.sn ] && extra=$capability
    compile "$work/sultanc-original" "$work/$name.original.o" "$input" $extra || fail "$input does not compile with the original compiler"
    compile "$work/sultanc-formatted" "$work/$name.formatted.o" "$input" $extra || fail "$input does not compile with the formatted compiler"
    cmp "$work/$name.original.o" "$work/$name.formatted.o" || fail "$input: the two compilers write different objects"
    echo "format corpus: $input: identical objects"
done
echo "format corpus: OK"
