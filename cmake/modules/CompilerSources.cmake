# The SultanC compiler's own sources: every stage that compiles compiler/main.sn
# depends on them (Stage1.cmake, Stage1Wasm.cmake, Stage2.cmake, Verify.cmake).
file(GLOB_RECURSE SULTANC_COMPILER_SOURCES CONFIGURE_DEPENDS
    "${SULTANC_SOURCE_ROOT}/compiler/*.sn"
    "${SULTANC_SOURCE_ROOT}/library/std/*.sn")
# The canonical manifest name is case-sensitive: Supax.toml. The repository
# may have a legacy lowercase supax.toml, which is not a package manifest.
# Track the optional root manifest only when its directory entry has the exact
# canonical spelling. CONFIGURE_DEPENDS notices creation/removal as well.
list(APPEND SULTANC_COMPILER_SOURCES
    "${SULTANC_SOURCE_ROOT}/library/Supax.toml")
file(GLOB _sultanc_root_toml CONFIGURE_DEPENDS
    "${SULTANC_SOURCE_ROOT}/*.toml")
foreach(_sultanc_root_toml IN LISTS _sultanc_root_toml)
    get_filename_component(_sultanc_root_name "${_sultanc_root_toml}" NAME)
    if(_sultanc_root_name STREQUAL "Supax.toml")
        list(APPEND SULTANC_COMPILER_SOURCES "${_sultanc_root_toml}")
    endif()
endforeach()
