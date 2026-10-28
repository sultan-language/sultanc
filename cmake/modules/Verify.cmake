# Bootstrap convergence (project contract): Stage2 builds Stage3, Stage3 builds
# Stage4, and Stage3/Stage4 must be byte-identical. On hosts with Native
# support, both backends and cross-backend equivalence are checked. Windows
# currently supports LLVM convergence only; backend selection is explicit:
#   native:  Stage2 -> stage3 -> stage4               compare stage3/stage4
#   llvm:    Stage2 -> llvm-stage3 -> llvm-stage4     compare llvm-stage3/4
#   cross:   llvm-stage3 builds a native compiler     compare with stage3
# Every executed compiler targets the build host; a foreign Stage2 is never
# run. Steps are chained so at most one compiler build runs at a time (each
# self-build of the compiler needs several GB of memory).
set(_sultanc_verify_directory "${SULTANC_BINARY_OUTPUT_DIRECTORY}/verify")
set(_sultanc_verify_inputs ${SULTANC_COMPILER_SOURCES})

function(_sultanc_compiler_step output compiler backend after comment)
    set(_runtime_copy "")
    set(_runtime_dependency "")
    if(WIN32)
        # Run before the Stage3/Stage4 compiler: both generated executables
        # import LLVM-C.dll from this configuration-specific directory.
        list(APPEND _runtime_copy COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${SULTANC_LLVM_DLL}" "${_sultanc_verify_directory}/LLVM-C.dll")
        set(_runtime_dependency "${SULTANC_LLVM_DLL}")
    endif()
    add_custom_command(
        OUTPUT "${output}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_sultanc_verify_directory}"
        ${_runtime_copy}
        COMMAND "${compiler}"
            "--backend=${backend}"
            "--target=${SULTANC_HOST_TARGET}"
            --emit=exe
            -o "${output}"
            compiler/main.sn
        DEPENDS "${compiler}" ${after} ${_sultanc_verify_inputs} ${_runtime_dependency}
        WORKING_DIRECTORY "${SULTANC_SOURCE_ROOT}"
        COMMENT "${comment}"
        VERBATIM)
endfunction()

function(_sultanc_compare_step stamp first second comment)
    add_custom_command(
        OUTPUT "${stamp}"
        COMMAND "${CMAKE_COMMAND}" -E compare_files "${first}" "${second}"
        COMMAND "${CMAKE_COMMAND}" -E touch "${stamp}"
        DEPENDS "${first}" "${second}"
        COMMENT "${comment}"
        VERBATIM)
endfunction()

if(SULTANC_HOST_TARGET AND SULTANC_STAGE2_TARGET STREQUAL SULTANC_HOST_TARGET)
    set(_d "${_sultanc_verify_directory}")
    if(WIN32)
        # No Windows Native backend. Check the LLVM self-hosted byte-identical
        # sequence only. Never advertise Native or cross-backend convergence.
        if(NOT SULTANC_STAGE2_BACKEND STREQUAL "llvm")
            message(FATAL_ERROR "Windows verify requires explicit SULTANC_STAGE2_BACKEND=llvm")
        endif()
        _sultanc_compiler_step("${_d}/sultanc-llvm-stage3${CMAKE_EXECUTABLE_SUFFIX}"
            "${SULTANC_STAGE2_OUTPUT}" llvm "" "Stage2 builds LLVM Stage3")
        _sultanc_compiler_step("${_d}/sultanc-llvm-stage4${CMAKE_EXECUTABLE_SUFFIX}"
            "${_d}/sultanc-llvm-stage3${CMAKE_EXECUTABLE_SUFFIX}" llvm ""
            "LLVM Stage3 builds LLVM Stage4")
        _sultanc_compare_step("${_d}/llvm.converged"
            "${_d}/sultanc-llvm-stage3${CMAKE_EXECUTABLE_SUFFIX}"
            "${_d}/sultanc-llvm-stage4${CMAKE_EXECUTABLE_SUFFIX}"
            "LLVM convergence: Stage3 and Stage4 must be byte-identical")
        set(_all "")
        if(SULTANC_REQUESTED_STAGE STREQUAL "verify")
            set(_all ALL)
        endif()
        add_custom_target(sultanc-verify-llvm DEPENDS "${_d}/llvm.converged")
        add_custom_target(sultanc-verify-native
            COMMAND "${CMAKE_COMMAND}" -E echo "Windows Native convergence is unavailable"
            COMMAND "${CMAKE_COMMAND}" -E false VERBATIM)
        add_custom_target(sultanc-verify ${_all} DEPENDS "${_d}/llvm.converged")
        add_dependencies(sultanc-verify sultanc-stage2)
    else()
        _sultanc_compiler_step("${_d}/sultanc-stage3" "${SULTANC_STAGE2_OUTPUT}" native ""
            "Stage2 builds native Stage3")
        _sultanc_compiler_step("${_d}/sultanc-stage4" "${_d}/sultanc-stage3" native ""
            "Stage3 builds native Stage4")
        _sultanc_compare_step("${_d}/native.converged" "${_d}/sultanc-stage3" "${_d}/sultanc-stage4"
            "Native convergence: Stage3 and Stage4 must be byte-identical")
        _sultanc_compiler_step("${_d}/sultanc-llvm-stage3" "${SULTANC_STAGE2_OUTPUT}" llvm
            "${_d}/native.converged" "Stage2 builds LLVM-backend Stage3")
        _sultanc_compiler_step("${_d}/sultanc-llvm-stage4" "${_d}/sultanc-llvm-stage3" llvm ""
            "LLVM Stage3 builds LLVM-backend Stage4")
        _sultanc_compare_step("${_d}/llvm.converged" "${_d}/sultanc-llvm-stage3" "${_d}/sultanc-llvm-stage4"
            "LLVM convergence: LLVM Stage3 and Stage4 must be byte-identical")
        _sultanc_compiler_step("${_d}/sultanc-native-by-llvm" "${_d}/sultanc-llvm-stage3" native
            "${_d}/llvm.converged" "LLVM Stage3 builds a native compiler")
        _sultanc_compare_step("${_d}/cross.converged" "${_d}/sultanc-stage3" "${_d}/sultanc-native-by-llvm"
            "Cross-backend: the native compiler built by LLVM Stage3 must equal native Stage3")

        set(_all "")
        if(SULTANC_REQUESTED_STAGE STREQUAL "verify")
            set(_all ALL)
        endif()
        add_custom_target(sultanc-verify-native DEPENDS "${_d}/native.converged")
        add_custom_target(sultanc-verify-llvm DEPENDS "${_d}/llvm.converged" "${_d}/cross.converged")
        add_custom_target(sultanc-verify ${_all}
            DEPENDS "${_d}/native.converged" "${_d}/llvm.converged" "${_d}/cross.converged")
        add_dependencies(sultanc-verify sultanc-stage2)
    endif()
else()
    add_custom_target(sultanc-verify
        COMMAND "${CMAKE_COMMAND}" -E echo
            "sultanc-verify needs a Stage2 for the build host (${SULTANC_HOST_TARGET}); configured Stage2 target: ${SULTANC_STAGE2_TARGET}"
        COMMAND "${CMAKE_COMMAND}" -E false
        VERBATIM)
endif()
