# Stage1 from the WebAssembly seed (SULTANC_BOOTSTRAP=wasm, included by
# Stage1Wasm.cmake). bootstrap-wasm/ holds only the seed and its C sources:
# wasm2c/ (the translator), runtime/ (w2c.h and w2c.c, the runtime the
# translated code uses, and wasi.c, the WASI host).
#
#   sultanc_wasm_add_stage1(<executable-target>
#       [WASM2C <prebuilt sultanc-wasm2c>]   # cross builds: a build-machine translator
#       [UNITS <n>]                          # generated function units (default 8)
#       [EXCLUDE_FROM_ALL])
#
# Defines the translator target `sultanc-wasm2c` (unless WASM2C is given), the
# generated C (seed/sultanc.wasm.zst translated as committed), and
# <executable-target>: the translated seed linked with the runtime. Sets
# SULTANC_WASM2C_COMMAND and SULTANC_WASM2C_DEPENDS in the caller's scope.
set(SULTANC_WASM_DIR "${SULTANC_SOURCE_ROOT}/bootstrap-wasm")

function(sultanc_wasm_add_stage1 exe_target)
    cmake_parse_arguments(ARG "EXCLUDE_FROM_ALL" "WASM2C;UNITS" "" ${ARGN})
    if(NOT ARG_UNITS)
        set(ARG_UNITS 8)
    endif()
    set(_seed "${SULTANC_WASM_DIR}/seed/sultanc.wasm.zst")

    # 1. The translator: a build-machine tool.
    if(ARG_WASM2C)
        set(_wasm2c "${ARG_WASM2C}")
        set(_wasm2c_dep "${ARG_WASM2C}")
    elseif(CMAKE_CROSSCOMPILING)
        message(FATAL_ERROR "Cross builds of the wasm bootstrap need a prebuilt sultanc-wasm2c "
                            "for the build machine (SULTANC_WASM2C)")
    else()
        set(_w2c "${SULTANC_WASM_DIR}/wasm2c")
        add_executable(sultanc-wasm2c "${_w2c}/common.c" "${_w2c}/stream.c" "${_w2c}/module.c"
                                      "${_w2c}/funcgen.c" "${_w2c}/output.c" "${_w2c}/main.c")
        set_target_properties(sultanc-wasm2c PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES)
        if(MSVC)
            target_compile_definitions(sultanc-wasm2c PRIVATE _CRT_SECURE_NO_WARNINGS)
        endif()
        set(_wasm2c "$<TARGET_FILE:sultanc-wasm2c>")
        set(_wasm2c_dep sultanc-wasm2c)
    endif()

    # 2. Translate the seed (the translator rejects a malformed zstd frame or
    #    module with a diagnostic).
    set(_gen "${CMAKE_CURRENT_BINARY_DIR}/bootstrap-wasm-gen")
    # The seed's content, not its file time, decides when to translate again:
    # an unpacked archive or a checkout can give a new seed an older time than
    # the C a previous build generated, and a stale Stage1 would then build
    # the sources. The stamp is rewritten (and so newer) only when the hash
    # changes; every configure (each build.sh run) checks it.
    file(SHA256 "${_seed}" _seed_hash)
    set(_seed_stamp "${CMAKE_CURRENT_BINARY_DIR}/bootstrap-wasm-seed.sha256")
    set(_old_hash "")
    if(EXISTS "${_seed_stamp}")
        file(READ "${_seed_stamp}" _old_hash)
    endif()
    if(NOT _old_hash STREQUAL _seed_hash)
        file(WRITE "${_seed_stamp}" "${_seed_hash}")
    endif()
    set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${_seed}")
    set(_sources "${_gen}/sultanc-module.c")
    foreach(_i RANGE 1 ${ARG_UNITS})
        list(APPEND _sources "${_gen}/sultanc-${_i}.c")
    endforeach()
    add_custom_command(
        OUTPUT "${_gen}/sultanc.h" ${_sources}
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_gen}"
        COMMAND "${_wasm2c}" "--units=${ARG_UNITS}" "${_seed}" "${_gen}/sultanc"
        DEPENDS ${_wasm2c_dep} "${_seed}" "${_seed_stamp}"
        COMMENT "Translating the SultanC WASI seed to C"
        VERBATIM)

    # 3. Stage1: the translated seed plus the runtime and the WASI host. Plain
    #    C99, any toolchain.
    set(_exclude "")
    if(ARG_EXCLUDE_FROM_ALL)
        set(_exclude EXCLUDE_FROM_ALL)
    endif()
    add_executable(${exe_target} ${_exclude} ${_sources} "${SULTANC_WASM_DIR}/runtime/w2c.c"
                                 "${SULTANC_WASM_DIR}/runtime/wasi.c")
    target_include_directories(${exe_target} PRIVATE "${_gen}" "${SULTANC_WASM_DIR}/runtime")
    set_target_properties(${exe_target} PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED YES C_EXTENSIONS NO)
    set_source_files_properties(${_sources} PROPERTIES GENERATED TRUE)
    if(MSVC)
        set_source_files_properties(${_sources} PROPERTIES COMPILE_OPTIONS "/w;/bigobj")
        target_compile_definitions(${exe_target} PRIVATE _CRT_SECURE_NO_WARNINGS)
    else()
        set_source_files_properties(${_sources} PROPERTIES COMPILE_OPTIONS "-w")
    endif()
    # Optimize even without a build type: Stage1 compiles the whole compiler.
    get_property(_multi GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(NOT _multi AND NOT CMAKE_BUILD_TYPE)
        target_compile_options(${exe_target} PRIVATE
            $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang,GNU>:-O2>
            $<$<COMPILE_LANG_AND_ID:C,MSVC>:/O2>)
    endif()

    set(SULTANC_WASM2C_COMMAND "${_wasm2c}" PARENT_SCOPE)
    set(SULTANC_WASM2C_DEPENDS "${_wasm2c_dep}" PARENT_SCOPE)
endfunction()
