if(NOT DEFINED SULTANC_STAGE0_DEFAULT_TARGET)
    set(SULTANC_STAGE0_DEFAULT_TARGET "" CACHE STRING "Stage0 default LLVM object target")
endif()
if(NOT DEFINED SULTANC_STAGE0_OUTPUT_NAME)
    set(SULTANC_STAGE0_OUTPUT_NAME "sultanc-stage0")
endif()

file(GLOB_RECURSE SULTANC_STAGE0_C_SOURCES CONFIGURE_DEPENDS
    "${SULTANC_SOURCE_ROOT}/bootstrap/src/*.c")
set(_sultanc_stage0_export "")
if(SULTANC_EXPORT_DIRECTORY AND NOT SULTANC_STAGE0_OUTPUT_EXPLICIT AND
   (SULTANC_REQUESTED_STAGE STREQUAL "stage0" OR
    SULTANC_REQUESTED_STAGE STREQUAL "bootstrap"))
    sultanc_export_destination("${SULTANC_STAGE0_OUTPUT_NAME}${CMAKE_EXECUTABLE_SUFFIX}"
        "${SULTANC_EXPORT_DIRECTORY}/bootstrap" _sultanc_stage0_export)
endif()
# The stage target precedes the internal executable target (see
# sultanc_export_destination in CMakeLists.txt).
add_custom_target(sultanc-stage0 ALL DEPENDS ${_sultanc_stage0_export})
add_executable(sultanc-stage0-executable ${SULTANC_STAGE0_C_SOURCES})
add_dependencies(sultanc-stage0 sultanc-stage0-executable)
set_target_properties(sultanc-stage0-executable PROPERTIES
    C_STANDARD 11
    C_STANDARD_REQUIRED YES
    C_EXTENSIONS NO
    OUTPUT_NAME "${SULTANC_STAGE0_OUTPUT_NAME}")
if(SULTANC_STAGE0_OUTPUT_EXPLICIT)
    # A generator expression keeps multi-config generators from appending a
    # per-configuration subdirectory: the requested path is exact.
    set_target_properties(sultanc-stage0-executable PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "$<1:${SULTANC_STAGE0_OUTPUT_DIRECTORY}>")
else()
    # Internal product of this build tree (multi-config: per configuration).
    set_target_properties(sultanc-stage0-executable PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
endif()
target_include_directories(sultanc-stage0-executable PRIVATE
    "${SULTANC_SOURCE_ROOT}/bootstrap/include/private"
    "${SULTANC_SOURCE_ROOT}/bootstrap/include/public")
target_compile_options(sultanc-stage0-executable PRIVATE
    $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang,GNU>:-Wall>
    $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang,GNU>:-Wextra>)
if(SULTANC_STAGE0_WARNINGS_AS_ERRORS)
    target_compile_options(sultanc-stage0-executable PRIVATE
        $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang,GNU>:-Werror>)
endif()
# Without a build type the seed is still built optimized; a selected build
# type or configuration owns optimization through CMake's own flags.
if(NOT SULTANC_MULTI_CONFIG AND NOT CMAKE_BUILD_TYPE)
    target_compile_options(sultanc-stage0-executable PRIVATE
        $<$<COMPILE_LANG_AND_ID:C,Clang,AppleClang,GNU>:-O1>)
endif()
if(SULTANC_STAGE0_DEFAULT_TARGET)
    target_compile_definitions(sultanc-stage0-executable PRIVATE
        "SULTANC_BOOTSTRAP_DEFAULT_TARGET=\"${SULTANC_STAGE0_DEFAULT_TARGET}\"")
endif()
target_link_libraries(sultanc-stage0-executable PRIVATE SultanC::LLVM)

if(_sultanc_stage0_export)
    sultanc_export_executable(sultanc-stage0-executable "${_sultanc_stage0_export}"
        "Exporting Stage0 to ${SULTANC_EXPORT_DIRECTORY}/bootstrap")
endif()
