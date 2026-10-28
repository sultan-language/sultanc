cmake_minimum_required(VERSION 3.21)
project(SultanCBootstrap LANGUAGES C)

# The CMake project is the sole owner of bootstrap stage configuration, host
# selection, output paths and build ordering. build.sh only invokes this project.
get_filename_component(SULTANC_SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/.." ABSOLUTE)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/modules")

# Stages:
#   stage0     the C seed compiler (Stage0)
#   stage1     Stage0 compiles the SultanC compiler into the Stage1 executable
#   bootstrap  Stage0 + Stage1 only: produce the bootstrap compiler, nothing more
#   stage2     Stage1 self-hosts the production compiler (explicit backend)
#   verify     stage2, then Stage3/Stage4 byte-identical convergence of the
#              available selected backend(s)
set(SULTANC_REQUESTED_STAGE "stage2" CACHE STRING
    "Requested build stage: stage0, stage1, stage2, bootstrap, verify")
set_property(CACHE SULTANC_REQUESTED_STAGE PROPERTY STRINGS
    stage0 stage1 stage2 bootstrap verify)
set(SULTANC_REQUESTED_TARGET "" CACHE STRING
    "Requested target; Stage1 always runs on the build host")
set(SULTANC_REQUESTED_OUTPUT "" CACHE STRING
    "Optional output path, or output directory for bootstrap")
set(SULTANC_STAGE2_BACKEND "native" CACHE STRING
    "Backend used by Stage1 to build Stage2: native or llvm (never implicit fallback)")
set_property(CACHE SULTANC_STAGE2_BACKEND PROPERTY STRINGS native llvm)
if(NOT SULTANC_STAGE2_BACKEND MATCHES "^(native|llvm)$")
    message(FATAL_ERROR "Unsupported SULTANC_STAGE2_BACKEND=${SULTANC_STAGE2_BACKEND}; choose native or llvm")
endif()

if(NOT SULTANC_REQUESTED_STAGE MATCHES "^(stage0|stage1|stage2|bootstrap|verify)$")
    message(FATAL_ERROR "Unknown SultanC bootstrap stage: ${SULTANC_REQUESTED_STAGE}")
endif()

# Stage1 is executed by the build and must match the host. The production
# target for Stage2 is independent; Stage0 can emit objects for LLVM targets.
set(SULTANC_HOST_TARGET "")
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" AND
   CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
    set(SULTANC_HOST_TARGET "arm64-darwin")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" AND
       CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(x86_64|amd64)$")
    set(SULTANC_HOST_TARGET "x86_64-linux")
elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    # CMAKE_HOST_SYSTEM_PROCESSOR is the real host architecture, not the
    # generator's selected cross/compatibility architecture.
    if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(AMD64|amd64|x86_64)$")
        set(SULTANC_HOST_TARGET "x86_64-windows-msvc")
        set(_sultanc_windows_arch "x64")
    elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(ARM64|arm64|aarch64)$")
        set(SULTANC_HOST_TARGET "aarch64-windows-msvc")
        set(_sultanc_windows_arch "ARM64")
    endif()
endif()

if(SULTANC_REQUESTED_STAGE STREQUAL "stage0")
    set(SULTANC_STAGE0_DEFAULT_TARGET "${SULTANC_REQUESTED_TARGET}")
elseif(SULTANC_REQUESTED_STAGE STREQUAL "stage1" OR
       SULTANC_REQUESTED_STAGE STREQUAL "bootstrap")
    if(SULTANC_REQUESTED_TARGET AND SULTANC_HOST_TARGET AND
       NOT SULTANC_REQUESTED_TARGET STREQUAL SULTANC_HOST_TARGET)
        message(FATAL_ERROR
            "Stage1 must run on the build host (${SULTANC_HOST_TARGET}); "
            "requested ${SULTANC_REQUESTED_TARGET}")
    endif()
else()
    if(SULTANC_REQUESTED_TARGET)
        set(SULTANC_STAGE2_TARGET "${SULTANC_REQUESTED_TARGET}")
    endif()
endif()

# Stage0 must execute on the build host in order to produce the Stage1 object.
# A cross-compiled Stage0 executable cannot be used for self-hosting.
if(CMAKE_CROSSCOMPILING AND NOT SULTANC_REQUESTED_STAGE STREQUAL "stage0")
    message(FATAL_ERROR
        "Stage1 requires a host-executable Stage0; the configured C compiler "
        "is cross-compiling. Use a native host compiler for bootstrap.")
endif()

if(NOT SULTANC_HOST_TARGET AND NOT SULTANC_REQUESTED_STAGE STREQUAL "stage0")
    message(FATAL_ERROR
        "No runnable Stage1 host target is registered for "
        "${CMAKE_HOST_SYSTEM_NAME}/${CMAKE_HOST_SYSTEM_PROCESSOR}. "
        "Stage0 can still be built directly with target sultanc-stage0.")
endif()
if(NOT SULTANC_HOST_TARGET)
    # Unspecified host targets remain unspecified: LLVM chooses its own host
    # triple only when Stage0 itself is invoked without --target.
    set(SULTANC_STAGE1_TARGET "")
else()
    set(SULTANC_STAGE1_TARGET "${SULTANC_HOST_TARGET}")
endif()
if(NOT DEFINED SULTANC_STAGE2_TARGET OR NOT SULTANC_STAGE2_TARGET)
    set(SULTANC_STAGE2_TARGET "${SULTANC_HOST_TARGET}")
endif()

# Stage0's C objects and the Stage1 object must target the same Darwin
# architecture.  Stage1 currently has only the arm64-darwin host contract;
# a universal or x86_64 C link cannot consume its single-architecture object.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin" AND
   NOT SULTANC_REQUESTED_STAGE STREQUAL "stage0")
    if(CMAKE_OSX_ARCHITECTURES AND
       NOT CMAKE_OSX_ARCHITECTURES STREQUAL "arm64")
        message(FATAL_ERROR
            "Stage1 targets ${SULTANC_STAGE1_TARGET}, but "
            "CMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES}. "
            "Stage1 requires a single arm64 Darwin linker architecture.")
    endif()
    if(CMAKE_C_COMPILER_TARGET AND
       NOT CMAKE_C_COMPILER_TARGET MATCHES "^(arm64|aarch64)(-|$)")
        message(FATAL_ERROR
            "Stage1 targets ${SULTANC_STAGE1_TARGET}, but the configured "
            "C compiler targets ${CMAKE_C_COMPILER_TARGET}.")
    endif()
    if(CMAKE_C_COMPILER_ARCHITECTURE_ID AND
       NOT CMAKE_C_COMPILER_ARCHITECTURE_ID MATCHES "^(arm64|aarch64)$")
        message(FATAL_ERROR
            "Stage1 targets ${SULTANC_STAGE1_TARGET}, but the configured "
            "C compiler architecture is ${CMAKE_C_COMPILER_ARCHITECTURE_ID}.")
    endif()
endif()

# Stage1 is a native executable. Reject emulated/mismatched Windows toolchains
# instead of linking an object for the host into a different-architecture EXE.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows" AND
   NOT SULTANC_REQUESTED_STAGE STREQUAL "stage0")
    if(NOT MSVC)
        message(FATAL_ERROR
            "Windows Stage1 requires the MSVC ABI (MSVC or clang-cl). "
            "MinGW/GNU C toolchains are not supported by this host bootstrap.")
    endif()
    if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
        message(FATAL_ERROR "Windows Stage1 requires a 64-bit C toolchain.")
    endif()
    # Different spellings are normal (AMD64 versus x86_64), so normalize
    # the processor, generator platform and compiler's reported architecture.
    set(_sultanc_compiler_arch "${CMAKE_SYSTEM_PROCESSOR}")
    foreach(_sultanc_arch_fact IN ITEMS
            "${CMAKE_VS_PLATFORM_NAME}" "${CMAKE_GENERATOR_PLATFORM}"
            "${CMAKE_C_COMPILER_ARCHITECTURE_ID}" "${_sultanc_compiler_arch}")
        if(NOT _sultanc_arch_fact)
            continue()
        endif()
        if(_sultanc_arch_fact MATCHES "^(x64|X64|AMD64|amd64|x86_64)$")
            set(_sultanc_arch_fact "x64")
        elseif(_sultanc_arch_fact MATCHES "^(ARM64|arm64|aarch64)$")
            set(_sultanc_arch_fact "ARM64")
        else()
            message(FATAL_ERROR
                "Unsupported Windows C toolchain architecture: ${_sultanc_arch_fact}")
        endif()
        if(NOT _sultanc_arch_fact STREQUAL _sultanc_windows_arch)
            message(FATAL_ERROR
                "Windows Stage1 host is ${SULTANC_HOST_TARGET}, but the C "
                "toolchain/generator targets ${_sultanc_arch_fact}.")
        endif()
    endforeach()
    if(CMAKE_C_COMPILER_TARGET)
        if(_sultanc_windows_arch STREQUAL "x64" AND
           NOT CMAKE_C_COMPILER_TARGET MATCHES "^(x86_64|amd64)(-|$)")
            message(FATAL_ERROR "C compiler target ${CMAKE_C_COMPILER_TARGET} does not match ${SULTANC_HOST_TARGET}")
        elseif(_sultanc_windows_arch STREQUAL "ARM64" AND
               NOT CMAKE_C_COMPILER_TARGET MATCHES "^(aarch64|arm64)(-|$)")
            message(FATAL_ERROR "C compiler target ${CMAKE_C_COMPILER_TARGET} does not match ${SULTANC_HOST_TARGET}")
        endif()
        if(NOT CMAKE_C_COMPILER_TARGET MATCHES "windows-msvc")
            message(FATAL_ERROR "Windows Stage1 requires an MSVC target triple, not ${CMAKE_C_COMPILER_TARGET}")
        endif()
    endif()
endif()

# The registered Windows targets currently support delegated cross-compilation,
# not native machine code generation. Explicit LLVM is required for Stage2.
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows" AND
   (SULTANC_REQUESTED_STAGE STREQUAL "stage2" OR
    SULTANC_REQUESTED_STAGE STREQUAL "verify") AND
   SULTANC_STAGE2_BACKEND STREQUAL "native")
    message(FATAL_ERROR
        "Windows Native Stage2 is unavailable in the canonical target/backend "
        "implementation. Configure with -DSULTANC_STAGE2_BACKEND=llvm; "
        "no backend fallback is performed.")
endif()

# Native backend coverage for the Stage2 target is validated by the compiler
# itself (compiler/backend/target/target_registry.sn); an unsupported target
# fails in Stage1 with the compiler's diagnostic, never with an LLVM fallback.
# Convergence (verify) executes Stage2/Stage3, so it needs a host Stage2.
if(SULTANC_REQUESTED_STAGE STREQUAL "verify" AND
   NOT SULTANC_STAGE2_TARGET STREQUAL SULTANC_HOST_TARGET)
    message(FATAL_ERROR
        "verify runs Stage2 and Stage3 on the build host (${SULTANC_HOST_TARGET}); "
        "requested target ${SULTANC_STAGE2_TARGET} cannot be executed here.")
endif()

# Output policy. Products of this build tree stay in this build tree
# (${CMAKE_BINARY_DIR}/bin, per configuration for multi-config generators), so
# independent build trees never overwrite each other. An explicitly requested
# output is written exactly there. SULTANC_EXPORT_DIRECTORY additionally copies
# the requested stage's product to <dir>/sultanc (Stage2) or
# <dir>/bootstrap/sultanc-stageN; it defaults to <source>/build only for the
# canonical tree <source>/build/bootstrap/cmake used by build.sh, preserving the
# documented build/sultanc location.
get_property(SULTANC_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(SULTANC_MULTI_CONFIG)
    set(SULTANC_BINARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/$<CONFIG>")
else()
    set(SULTANC_BINARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()
file(REAL_PATH "${CMAKE_BINARY_DIR}" _sultanc_binary_real)
get_filename_component(_sultanc_canonical_tree
    "${SULTANC_SOURCE_ROOT}/build/bootstrap/cmake" ABSOLUTE)
set(_sultanc_default_export "")
if(_sultanc_binary_real STREQUAL _sultanc_canonical_tree)
    set(_sultanc_default_export "${SULTANC_SOURCE_ROOT}/build")
endif()
set(SULTANC_EXPORT_DIRECTORY "${_sultanc_default_export}" CACHE PATH
    "Optional directory receiving a copy of the requested stage product")

# Exported copies are real build outputs: a missing or stale copy is recreated
# by the next build of its stage. A multi-config generator exports each
# configuration to its own <dir>/<config>/ so configurations never overwrite
# or reuse one another; single-config trees (the canonical build.sh tree)
# export directly to <dir>/, preserving build/sultanc.
function(sultanc_export_destination name directory output)
    if(SULTANC_MULTI_CONFIG)
        set(${output} "${directory}/$<CONFIG>/${name}" PARENT_SCOPE)
    else()
        set(${output} "${directory}/${name}" PARENT_SCOPE)
    endif()
endfunction()
function(sultanc_export_executable source destination comment)
    get_filename_component(_destination_directory "${destination}" DIRECTORY)
    if(TARGET "${source}")
        set(_file "$<TARGET_FILE:${source}>")
    else()
        set(_file "${source}")
    endif()
    set(_runtime_copy "")
    set(_runtime_deps "")
    if(WIN32)
        # Every exported compiler imports LLVM-C.dll. Keep its runtime next to
        # the executable, including explicit and per-configuration exports.
        list(APPEND _runtime_copy
            COMMAND "${CMAKE_COMMAND}" -E copy_if_different
                "${SULTANC_LLVM_DLL}" "${_destination_directory}/LLVM-C.dll")
        list(APPEND _runtime_deps "${SULTANC_LLVM_DLL}")
    endif()
    add_custom_command(
        OUTPUT "${destination}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${_destination_directory}"
        COMMAND "${CMAKE_COMMAND}" -E copy "${_file}" "${destination}"
        ${_runtime_copy}
        DEPENDS "${source}" ${_runtime_deps}
        COMMENT "${comment}"
        VERBATIM)
endfunction()
# A stage target (sultanc-stage0, sultanc-stage1) is what build.sh and users
# build; it depends on the internal executable target and on the exported copy.
# Stage modules create it before the executable: Ninja Multi-Config also
# aliases an executable's output file name (sultanc-stageN), and the stage
# target must own that name.

set(SULTANC_STAGE0_OUTPUT_EXPLICIT FALSE)
set(SULTANC_STAGE1_OUTPUT_EXPLICIT FALSE)
set(SULTANC_STAGE2_OUTPUT_EXPLICIT FALSE)
if(SULTANC_REQUESTED_OUTPUT)
    # One exact path cannot hold the products of several configurations.
    list(LENGTH CMAKE_CONFIGURATION_TYPES _sultanc_configuration_count)
    if(SULTANC_MULTI_CONFIG AND NOT _sultanc_configuration_count EQUAL 1)
        message(FATAL_ERROR
            "SULTANC_REQUESTED_OUTPUT names one exact path, but this multi-config "
            "generator has the configurations '${CMAKE_CONFIGURATION_TYPES}'. "
            "Set CMAKE_CONFIGURATION_TYPES to a single configuration, or omit the "
            "output (products then stay per configuration under bin/<config>).")
    endif()
    if(IS_ABSOLUTE "${SULTANC_REQUESTED_OUTPUT}")
        set(_sultanc_output "${SULTANC_REQUESTED_OUTPUT}")
    else()
        get_filename_component(_sultanc_output
            "${SULTANC_SOURCE_ROOT}/${SULTANC_REQUESTED_OUTPUT}" ABSOLUTE)
    endif()
    if(SULTANC_REQUESTED_STAGE STREQUAL "stage0")
        get_filename_component(SULTANC_STAGE0_OUTPUT_DIRECTORY
            "${_sultanc_output}" DIRECTORY)
        get_filename_component(SULTANC_STAGE0_OUTPUT_NAME
            "${_sultanc_output}" NAME)
        set(SULTANC_STAGE0_OUTPUT_EXPLICIT TRUE)
    elseif(SULTANC_REQUESTED_STAGE STREQUAL "stage1")
        get_filename_component(SULTANC_STAGE1_OUTPUT_DIRECTORY
            "${_sultanc_output}" DIRECTORY)
        get_filename_component(SULTANC_STAGE1_OUTPUT_NAME
            "${_sultanc_output}" NAME)
        set(SULTANC_STAGE1_OUTPUT_EXPLICIT TRUE)
    elseif(SULTANC_REQUESTED_STAGE STREQUAL "bootstrap")
        set(SULTANC_STAGE0_OUTPUT_DIRECTORY "${_sultanc_output}")
        set(SULTANC_STAGE1_OUTPUT_DIRECTORY "${_sultanc_output}")
        set(SULTANC_STAGE0_OUTPUT_EXPLICIT TRUE)
        set(SULTANC_STAGE1_OUTPUT_EXPLICIT TRUE)
    else()
        set(SULTANC_STAGE2_OUTPUT "${_sultanc_output}")
        set(SULTANC_STAGE2_OUTPUT_EXPLICIT TRUE)
    endif()
endif()

# Optimization and warning policy of the C seed. -O1 applies only when no
# build type/configuration selects optimization flags itself.
option(SULTANC_STAGE0_WARNINGS_AS_ERRORS
    "Treat Stage0 C compiler warnings as errors" ON)

include(LLVMToolchain)
include(Stage0)
include(Stage1)
include(Stage2)
include(Verify)
# bootstrap means exactly: produce the bootstrap compiler (Stage0 + Stage1).
# Convergence is owned by sultanc-verify. Both stage targets include their
# exported copies.
add_custom_target(sultanc-bootstrap)
add_dependencies(sultanc-bootstrap sultanc-stage0 sultanc-stage1)
