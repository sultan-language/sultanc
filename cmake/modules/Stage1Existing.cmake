# SULTANC_BOOTSTRAP=existing: an already-built SultanC compiler stands in for
# Stage1, the way Go builds itself with GOROOT_BOOTSTRAP and rustc's stage1
# comes from the previous release. It is the fast development route: no C
# Stage0 and no seed translation, only one self-host step from the existing
# compiler to the new Stage2. The from-source routes (c, wasm) stay the
# reference: use them for a fresh system, when the existing compiler is too
# old for the sources (new syntax or primitives), and to verify a release.
#
# SULTANC_EXISTING_COMPILER names the compiler (default: build/sultanc). It is
# snapshotted into the build tree at configure time, so the Stage2 this route
# exports to build/sultanc never overwrites the compiler that is producing it;
# the next configure takes the new build/sultanc as its existing compiler.
include(CompilerSources)

set(_sultanc_existing_default "${SULTANC_SOURCE_ROOT}/build/sultanc${CMAKE_EXECUTABLE_SUFFIX}")
set(SULTANC_EXISTING_COMPILER "${_sultanc_existing_default}" CACHE FILEPATH
    "Existing SultanC compiler used as Stage1 by SULTANC_BOOTSTRAP=existing")
if(NOT EXISTS "${SULTANC_EXISTING_COMPILER}")
    message(FATAL_ERROR
        "SULTANC_BOOTSTRAP=existing needs an existing SultanC compiler, and "
        "${SULTANC_EXISTING_COMPILER} does not exist. Build one from source first "
        "(./build.sh, which uses the C Stage0, or ./build.sh --bootstrap=wasm), or name "
        "one with ./build.sh --bootstrap=existing --with=PATH.")
endif()
if(SULTANC_COMPILER_LLVM AND NOT SULTANC_STAGE2_BACKEND STREQUAL "native")
    message(STATUS "SULTANC_BOOTSTRAP=existing: the existing compiler must contain the "
                   "${SULTANC_STAGE2_BACKEND} backend to generate Stage2")
endif()

set(_sultanc_existing_dir "${CMAKE_BINARY_DIR}/existing")
set(_sultanc_existing_copy "${_sultanc_existing_dir}/sultanc-existing${CMAKE_EXECUTABLE_SUFFIX}")
file(MAKE_DIRECTORY "${_sultanc_existing_dir}")
file(COPY_FILE "${SULTANC_EXISTING_COMPILER}" "${_sultanc_existing_copy}" ONLY_IF_DIFFERENT)
file(CHMOD "${_sultanc_existing_copy}" PERMISSIONS
    OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

add_executable(sultanc-stage1-executable IMPORTED GLOBAL)
set_target_properties(sultanc-stage1-executable PROPERTIES IMPORTED_LOCATION "${_sultanc_existing_copy}")

add_custom_target(sultanc-stage0
    COMMAND "${CMAKE_COMMAND}" -E echo
        "SULTANC_BOOTSTRAP=existing has no Stage0: Stage1 is ${SULTANC_EXISTING_COMPILER}"
    COMMAND "${CMAKE_COMMAND}" -E false
    VERBATIM)
add_custom_target(sultanc-stage1 ALL
    COMMAND "${CMAKE_COMMAND}" -E echo "Stage1 is the existing compiler ${SULTANC_EXISTING_COMPILER}"
    VERBATIM)

# An LLVM-enabled Stage2 links the LLVM C library by its resolved path, as on
# the wasm route (an existing compiler may or may not carry libLLVM itself).
set(SULTANC_STAGE1_LINK_INPUTS "")
if(SULTANC_COMPILER_LLVM)
    if(WIN32)
        message(FATAL_ERROR "SULTANC_BOOTSTRAP=existing does not build an LLVM-enabled Windows "
                            "Stage2 yet; use SULTANC_BOOTSTRAP=c there")
    endif()
    get_filename_component(_sultanc_llvm_real "${SULTANC_LLVM_LIBRARY}" REALPATH)
    set(SULTANC_STAGE1_LINK_INPUTS "${_sultanc_llvm_real}")
endif()
