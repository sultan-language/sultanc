if(NOT DEFINED SULTANC_STAGE1_TARGET)
    set(SULTANC_STAGE1_TARGET "" CACHE STRING "Host target for the Stage1 executable")
endif()
if(NOT DEFINED SULTANC_STAGE1_OUTPUT_NAME)
    set(SULTANC_STAGE1_OUTPUT_NAME "sultanc-stage1")
endif()

# The first SultanC compiler is produced by Stage0, not by compiling C again.
# CMake's configured host C linker driver links the resulting machine object.
# No C source or Stage0 object participates in this link.
include(CompilerSources)
file(CONFIGURE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sultanc-stage1-target.txt"
    CONTENT "${SULTANC_STAGE1_TARGET}\n")
# Each configuration's Stage0 produces its own Stage1 object.
if(SULTANC_MULTI_CONFIG)
    set(SULTANC_STAGE1_OBJECT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/stage1/$<CONFIG>")
else()
    set(SULTANC_STAGE1_OBJECT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/stage1")
endif()
# Stage0 emits the host's object format; CMake consumes it as an external
# object (MSVC/clang-cl: COFF .obj, Darwin/Linux: their normal .o extension).
if(WIN32)
    set(_sultanc_stage1_object_extension ".obj")
else()
    set(_sultanc_stage1_object_extension "${CMAKE_C_OUTPUT_EXTENSION}")
endif()
set(SULTANC_STAGE1_OBJECT
    "${SULTANC_STAGE1_OBJECT_DIRECTORY}/sultanc-stage1${_sultanc_stage1_object_extension}")
add_custom_command(
    OUTPUT "${SULTANC_STAGE1_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${SULTANC_STAGE1_OBJECT_DIRECTORY}"
    # Stage0 has neither the thread primitives nor closure literals: the
    # machine interpreter's host threads are its stub in Stage1
    # (compiler/interpreter/machine/host_threads.sn). Nor has it `استنسخ`:
    # the copy of kept later-phase facts, which only incremental tooling
    # analysis asks for, is its stub (compiler/frontend/later_phase_facts_copy.sn).
    # Nor has it casts between integers and pointers: the interpreter's
    # width-exact raw memory access is its stub, word accesses
    # (compiler/interpreter/machine/raw_memory_access.sn).
    COMMAND "$<TARGET_FILE:sultanc-stage0-executable>"
        compiler/main.sn
        "--target=${SULTANC_STAGE1_TARGET}"
        "--module-source=compiler.interpreter.machine.host_threads=compiler/interpreter/machine/host_threads_unavailable.sn"
        "--module-source=compiler.frontend.later_phase_facts_copy=compiler/frontend/later_phase_facts_copy_unavailable.sn"
        "--module-source=compiler.interpreter.machine.raw_memory_access=compiler/interpreter/machine/raw_memory_access_unavailable.sn"
        -o "${SULTANC_STAGE1_OBJECT}"
    DEPENDS sultanc-stage0-executable ${SULTANC_COMPILER_SOURCES}
        "${CMAKE_CURRENT_BINARY_DIR}/sultanc-stage1-target.txt"
    WORKING_DIRECTORY "${SULTANC_SOURCE_ROOT}"
    COMMENT "Stage0 compiles SultanC sources into the Stage1 host object"
    VERBATIM
)
set(_sultanc_stage1_export "")
if(SULTANC_EXPORT_DIRECTORY AND NOT SULTANC_STAGE1_OUTPUT_EXPLICIT AND
   (SULTANC_REQUESTED_STAGE STREQUAL "stage1" OR
    SULTANC_REQUESTED_STAGE STREQUAL "bootstrap"))
    sultanc_export_destination("${SULTANC_STAGE1_OUTPUT_NAME}${CMAKE_EXECUTABLE_SUFFIX}"
        "${SULTANC_EXPORT_DIRECTORY}/bootstrap" _sultanc_stage1_export)
endif()
# Stage0-only builds must not generate or link the Stage1 compiler by default.
# The stage target precedes the internal executable target (see
# sultanc_export_destination in CMakeLists.txt).
if(SULTANC_REQUESTED_STAGE STREQUAL "stage0")
    add_custom_target(sultanc-stage1 DEPENDS ${_sultanc_stage1_export})
    add_executable(sultanc-stage1-executable EXCLUDE_FROM_ALL "${SULTANC_STAGE1_OBJECT}")
else()
    add_custom_target(sultanc-stage1 ALL DEPENDS ${_sultanc_stage1_export})
    add_executable(sultanc-stage1-executable "${SULTANC_STAGE1_OBJECT}")
endif()
add_dependencies(sultanc-stage1 sultanc-stage1-executable)
sultanc_compiler_stack(sultanc-stage1-executable)
set_source_files_properties("${SULTANC_STAGE1_OBJECT}" PROPERTIES
    GENERATED TRUE EXTERNAL_OBJECT TRUE)
set_target_properties(sultanc-stage1-executable PROPERTIES
    LINKER_LANGUAGE C
    OUTPUT_NAME "${SULTANC_STAGE1_OUTPUT_NAME}")
if(SULTANC_STAGE1_OUTPUT_EXPLICIT)
    set_target_properties(sultanc-stage1-executable PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "$<1:${SULTANC_STAGE1_OUTPUT_DIRECTORY}>")
else()
    set_target_properties(sultanc-stage1-executable PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()
# SultanC's LLVM backend is part of the Stage1 compiler: its C API symbols
# must be resolved at link time, using the same selected LLVM SDK as Stage0.
# Stage0 has no compiler-build capability input; it compiles the default
# source of the LLVM inclusion boundary (compiler/backend/provider/llvm/
# llvm_codegen.sn), so Stage1 always contains LLVM references and always
# needs this library. SULTANC_COMPILER_LLVM selects the capability of the
# self-hosted stages only (Stage2.cmake, Verify.cmake).
target_link_libraries(sultanc-stage1-executable PRIVATE SultanC::LLVM)

# Windows: Stage0's object carries no /DEFAULTLIB records (cl writes them only
# into the objects it compiles), so the link names the C runtime (CMake's
# default, the DLL runtime: msvcrt with ucrt and vcruntime, which also give
# mainCRTStartup, __chkstk and _fltused) and the system libraries std imports
# by DLL (library/std/os/platform/windows/win32): ws2_32, crypt32, the
# synchronization API set (WaitOnAddress), ntdll, and bcryptprimitives
# (ProcessPrng), whose import library is made here from a .def file because
# not every Windows SDK ships one.
if(WIN32 AND MSVC)
    set(_sultanc_prng_def "${CMAKE_CURRENT_BINARY_DIR}/bcryptprimitives.def")
    set(_sultanc_prng_lib "${CMAKE_CURRENT_BINARY_DIR}/bcryptprimitives.lib")
    file(WRITE "${_sultanc_prng_def}" "LIBRARY bcryptprimitives.dll\nEXPORTS\n    ProcessPrng\n")
    if(CMAKE_C_COMPILER_ARCHITECTURE_ID MATCHES "ARM64")
        set(_sultanc_lib_machine ARM64)
    else()
        set(_sultanc_lib_machine X64)
    endif()
    add_custom_command(
        OUTPUT "${_sultanc_prng_lib}"
        COMMAND "${CMAKE_AR}" /nologo "/def:${_sultanc_prng_def}" "/out:${_sultanc_prng_lib}"
            "/machine:${_sultanc_lib_machine}"
        DEPENDS "${_sultanc_prng_def}"
        COMMENT "Import library for bcryptprimitives.dll (ProcessPrng)"
        VERBATIM)
    add_custom_target(sultanc-stage1-prng-implib DEPENDS "${_sultanc_prng_lib}")
    add_dependencies(sultanc-stage1-executable sultanc-stage1-prng-implib)
    target_link_libraries(sultanc-stage1-executable PRIVATE
        msvcrt ucrt vcruntime ws2_32 crypt32 synchronization ntdll "${_sultanc_prng_lib}")
endif()

if(_sultanc_stage1_export)
    sultanc_export_executable(sultanc-stage1-executable "${_sultanc_stage1_export}"
        "Exporting Stage1 to ${SULTANC_EXPORT_DIRECTORY}/bootstrap")
endif()

# An explicitly placed Stage1 executable also needs its own runtime DLL.
if(WIN32)
    add_custom_command(TARGET sultanc-stage1-executable POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${SULTANC_LLVM_DLL}" "$<TARGET_FILE_DIR:sultanc-stage1-executable>/LLVM-C.dll"
        VERBATIM)
endif()
