if(NOT DEFINED SULTANC_STAGE1_TARGET)
    set(SULTANC_STAGE1_TARGET "" CACHE STRING "Host target for the Stage1 executable")
endif()
if(NOT DEFINED SULTANC_STAGE1_OUTPUT_NAME)
    set(SULTANC_STAGE1_OUTPUT_NAME "sultanc-stage1")
endif()

# The first SultanC compiler is produced by Stage0, not by compiling C again.
# CMake's configured host C linker driver links the resulting machine object.
# No C source or Stage0 object participates in this link.
file(GLOB_RECURSE SULTANC_COMPILER_SOURCES CONFIGURE_DEPENDS
    "${SULTANC_SOURCE_ROOT}/compiler/*.sn"
    "${SULTANC_SOURCE_ROOT}/library/std/*.sn")
list(APPEND SULTANC_COMPILER_SOURCES
    "${SULTANC_SOURCE_ROOT}/supax.toml"
    "${SULTANC_SOURCE_ROOT}/library/Supax.toml")
file(CONFIGURE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sultanc-stage1-target.txt"
    CONTENT "${SULTANC_STAGE1_TARGET}\n")
# Each configuration's Stage0 produces its own Stage1 object.
if(SULTANC_MULTI_CONFIG)
    set(SULTANC_STAGE1_OBJECT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/stage1/$<CONFIG>")
else()
    set(SULTANC_STAGE1_OBJECT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/stage1")
endif()
set(SULTANC_STAGE1_OBJECT
    "${SULTANC_STAGE1_OBJECT_DIRECTORY}/sultanc-stage1${CMAKE_C_OUTPUT_EXTENSION}")
add_custom_command(
    OUTPUT "${SULTANC_STAGE1_OBJECT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${SULTANC_STAGE1_OBJECT_DIRECTORY}"
    COMMAND "$<TARGET_FILE:sultanc-stage0-executable>"
        compiler/main.sn
        "--target=${SULTANC_STAGE1_TARGET}"
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
target_link_libraries(sultanc-stage1-executable PRIVATE SultanC::LLVM)

if(_sultanc_stage1_export)
    sultanc_export_executable(sultanc-stage1-executable "${_sultanc_stage1_export}"
        "Exporting Stage1 to ${SULTANC_EXPORT_DIRECTORY}/bootstrap")
endif()
