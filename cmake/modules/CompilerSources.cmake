# The SultanC compiler's own sources: every stage that compiles compiler/main.sn
# depends on them (Stage1.cmake, Stage1Wasm.cmake, Stage2.cmake, Verify.cmake).
file(GLOB_RECURSE SULTANC_COMPILER_SOURCES CONFIGURE_DEPENDS
    "${SULTANC_SOURCE_ROOT}/compiler/*.sn"
    "${SULTANC_SOURCE_ROOT}/library/std/*.sn"
    "${SULTANC_SOURCE_ROOT}/library/alloc/*.sn"
    "${SULTANC_SOURCE_ROOT}/library/core/*.sn")
# The compiler knows std itself (compiler/driver/std_location.sn), and the
# libraries beneath it, alloc and core; the module spellings of each are in
# its root file (library/std/std.sn, library/alloc/alloc.sn,
# library/core/core.sn), which the globs above cover.
