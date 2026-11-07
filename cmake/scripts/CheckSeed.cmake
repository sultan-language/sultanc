# Compares a rebuilt seed with the committed one (used by the sultanc-check-seed target).
# Inputs: WASM2C (translator), SEED (rebuilt sultanc.wasm), COMMITTED (the
# committed seed/sultanc.wasm.zst), DECODED (where to write it decompressed).
# The committed seed is decompressed by the in-tree decoder, then compared
# byte for byte with the rebuilt module.
execute_process(COMMAND "${WASM2C}" --decompress "${COMMITTED}" "${DECODED}" RESULT_VARIABLE _rc)
if(NOT _rc EQUAL 0)
    message(FATAL_ERROR "seed check: cannot decompress ${COMMITTED}")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -E compare_files "${SEED}" "${DECODED}" RESULT_VARIABLE _rc)
if(_rc EQUAL 0)
    message(STATUS "seed check: the committed seed is reproduced exactly")
else()
    message(FATAL_ERROR "seed check: the current sources give a different seed\n"
                        "  committed ${COMMITTED} (decompressed to ${DECODED})\n  rebuilt   ${SEED}\n"
                        "  To replace it: build-bootstrap --update-seed (bootstrap.c)")
endif()
