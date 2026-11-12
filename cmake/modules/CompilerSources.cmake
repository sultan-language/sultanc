# The SultanC compiler's own sources: every stage that compiles compiler/main.sn
# depends on them (Stage1.cmake, Stage1Wasm.cmake, Stage2.cmake, Verify.cmake).
file(GLOB_RECURSE SULTANC_COMPILER_SOURCES CONFIGURE_DEPENDS
    "${SULTANC_SOURCE_ROOT}/compiler/*.sn"
    "${SULTANC_SOURCE_ROOT}/library/std/*.sn")
# The compiler knows std itself (compiler/driver/std_location.sn); the module
# spellings of std are in library/std/std.sn, which the glob above covers.
