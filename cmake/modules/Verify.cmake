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
#
# Every generated compiler keeps the configured compiler-build capability
# (SULTANC_COMPILER_LLVM via SULTANC_COMPILER_CAPABILITY_ARGS), so it survives
# self-hosting. A Native-only Stage2 contains no LLVM backend, so with
# SULTANC_COMPILER_LLVM=OFF the LLVM producer is Stage1 (always LLVM-enabled):
#   native:  Stage2 -> stage3 -> stage4               compare stage3/stage4
#   cross:   Stage1 --backend=llvm -> llvm-stage3 (Native-only, LLVM-generated)
#            llvm-stage3 --backend=native -> native-by-llvm, compare with stage3
#   llvm:    unavailable (a Native-only compiler cannot run the LLVM backend)
# With SULTANC_BOOTSTRAP=wasm, Stage1 is Native-only too, so the Native-only
# cross step is skipped (native convergence only); with SULTANC_COMPILER_LLVM=ON
# Stage2 has LLVM and all three checks run as above.
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
            ${SULTANC_COMPILER_CAPABILITY_ARGS}
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
    if(WIN32 AND SULTANC_BOOTSTRAP STREQUAL "wasm")
        # Windows convergence needs the LLVM backend, which the seed's Stage1 lacks.
        add_custom_target(sultanc-verify
            COMMAND "${CMAKE_COMMAND}" -E echo
                "Windows verify needs SULTANC_BOOTSTRAP=c (an LLVM-enabled Stage1)"
            COMMAND "${CMAKE_COMMAND}" -E false VERBATIM)
    elseif(WIN32)
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
    elseif(NOT SULTANC_COMPILER_LLVM)
        _sultanc_compiler_step("${_d}/sultanc-stage3" "${SULTANC_STAGE2_OUTPUT}" native ""
            "Native-only Stage2 builds native Native-only Stage3")
        _sultanc_compiler_step("${_d}/sultanc-stage4" "${_d}/sultanc-stage3" native ""
            "Native-only Stage3 builds native Native-only Stage4")
        _sultanc_compare_step("${_d}/native.converged" "${_d}/sultanc-stage3" "${_d}/sultanc-stage4"
            "Native convergence: Native-only Stage3 and Stage4 must be byte-identical")
        # The LLVM producer of the cross-backend check is Stage1, which only the
        # C route builds with LLVM; the wasm route's Stage1 is Native-only, and an
        # existing compiler (SULTANC_BOOTSTRAP=existing) is not assumed to have LLVM.
        set(_sultanc_cross_available TRUE)
        if(SULTANC_BOOTSTRAP MATCHES "^(wasm|existing)$")
            set(_sultanc_cross_available FALSE)
        endif()
        if(_sultanc_cross_available)
            _sultanc_compiler_step("${_d}/sultanc-llvm-stage3" "$<TARGET_FILE:sultanc-stage1-executable>" llvm
                "${_d}/native.converged" "Stage1 builds an LLVM-generated Native-only Stage3")
            _sultanc_compiler_step("${_d}/sultanc-native-by-llvm" "${_d}/sultanc-llvm-stage3" native ""
                "LLVM-generated Native-only Stage3 builds a native Native-only compiler")
            _sultanc_compare_step("${_d}/cross.converged" "${_d}/sultanc-stage3" "${_d}/sultanc-native-by-llvm"
                "Cross-backend: the compiler built by the LLVM-generated Stage3 must equal native Stage3")
        endif()
        set(_all "")
        if(SULTANC_REQUESTED_STAGE STREQUAL "verify")
            set(_all ALL)
        endif()
        add_custom_target(sultanc-verify-native DEPENDS "${_d}/native.converged")
        add_custom_target(sultanc-verify-llvm
            COMMAND "${CMAKE_COMMAND}" -E echo
                "LLVM convergence needs SULTANC_COMPILER_LLVM=ON: a Native-only compiler has no LLVM backend"
            COMMAND "${CMAKE_COMMAND}" -E false VERBATIM)
        if(_sultanc_cross_available)
            add_custom_target(sultanc-verify-cross DEPENDS "${_d}/cross.converged")
            add_custom_target(sultanc-verify ${_all}
                DEPENDS "${_d}/native.converged" "${_d}/cross.converged")
        else()
            add_custom_target(sultanc-verify-cross
                COMMAND "${CMAKE_COMMAND}" -E echo
                    "Cross-backend convergence needs an LLVM backend: SULTANC_BOOTSTRAP=c or SULTANC_COMPILER_LLVM=ON"
                COMMAND "${CMAKE_COMMAND}" -E false VERBATIM)
            add_custom_target(sultanc-verify ${_all} DEPENDS "${_d}/native.converged")
        endif()
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
