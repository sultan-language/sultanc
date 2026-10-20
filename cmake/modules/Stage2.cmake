if(NOT DEFINED SULTANC_STAGE2_TARGET)
    set(SULTANC_STAGE2_TARGET "" CACHE STRING "SultanC target for Stage2 output")
endif()
if(NOT SULTANC_STAGE2_OUTPUT_EXPLICIT)
    set(SULTANC_STAGE2_OUTPUT "${SULTANC_BINARY_OUTPUT_DIRECTORY}/sultanc")
endif()
get_filename_component(SULTANC_STAGE2_OUTPUT_DIRECTORY
    "${SULTANC_STAGE2_OUTPUT}" DIRECTORY)

file(CONFIGURE
    OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/sultanc-stage2-target.txt"
    CONTENT "${SULTANC_STAGE2_TARGET}\n")

# The only generator of Stage2 is the self-hosted Stage1 compiler. The backend,
# link policy, runtime, and target selection belong to SultanC itself.
add_custom_command(
    OUTPUT "${SULTANC_STAGE2_OUTPUT}"
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${SULTANC_STAGE2_OUTPUT_DIRECTORY}"
    COMMAND "$<TARGET_FILE:sultanc-stage1-executable>"
        --backend=native
        "--target=${SULTANC_STAGE2_TARGET}"
        --emit=exe
        -o "${SULTANC_STAGE2_OUTPUT}"
        compiler/main.sn
    DEPENDS sultanc-stage1-executable ${SULTANC_COMPILER_SOURCES}
        "${CMAKE_CURRENT_BINARY_DIR}/sultanc-stage2-target.txt"
    WORKING_DIRECTORY "${SULTANC_SOURCE_ROOT}"
    COMMENT "Stage1 self-hosts SultanC as Stage2"
    VERBATIM
)
set(_sultanc_stage2_products "${SULTANC_STAGE2_OUTPUT}")
if(SULTANC_EXPORT_DIRECTORY AND NOT SULTANC_STAGE2_OUTPUT_EXPLICIT)
    sultanc_export_destination(sultanc "${SULTANC_EXPORT_DIRECTORY}" SULTANC_STAGE2_EXPORT)
    # An export directory that is the internal output directory names the
    # product itself: no copy (and no second rule for that file) is needed.
    cmake_path(ABSOLUTE_PATH SULTANC_STAGE2_EXPORT BASE_DIRECTORY "${CMAKE_BINARY_DIR}"
        NORMALIZE OUTPUT_VARIABLE _sultanc_stage2_export_normal)
    cmake_path(ABSOLUTE_PATH SULTANC_STAGE2_OUTPUT BASE_DIRECTORY "${CMAKE_BINARY_DIR}"
        NORMALIZE OUTPUT_VARIABLE _sultanc_stage2_output_normal)
    if(NOT _sultanc_stage2_export_normal STREQUAL _sultanc_stage2_output_normal)
        sultanc_export_executable("${SULTANC_STAGE2_OUTPUT}" "${SULTANC_STAGE2_EXPORT}"
            "Exporting Stage2 to ${SULTANC_STAGE2_EXPORT}")
        list(APPEND _sultanc_stage2_products "${SULTANC_STAGE2_EXPORT}")
    endif()
endif()
# Stage2 is part of the default build only when it (or verify) was requested.
if(SULTANC_REQUESTED_STAGE STREQUAL "stage2" OR
   SULTANC_REQUESTED_STAGE STREQUAL "verify")
    add_custom_target(sultanc-stage2 ALL DEPENDS ${_sultanc_stage2_products})
else()
    add_custom_target(sultanc-stage2 DEPENDS ${_sultanc_stage2_products})
endif()
