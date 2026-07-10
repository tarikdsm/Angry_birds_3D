if(NOT EXISTS "${DLL_PATH}")
  message(FATAL_ERROR "Expected GDExtension DLL does not exist: ${DLL_PATH}")
endif()

execute_process(
  COMMAND "${DUMPBIN_EXECUTABLE}" /exports "${DLL_PATH}"
  RESULT_VARIABLE _dumpbin_result
  OUTPUT_VARIABLE _dumpbin_output
  ERROR_VARIABLE _dumpbin_error
)
if(NOT _dumpbin_result EQUAL 0)
  message(FATAL_ERROR "dumpbin failed for ${DLL_PATH}: ${_dumpbin_error}")
endif()
if(NOT _dumpbin_output MATCHES "ninho_physics_library_init")
  message(FATAL_ERROR "GDExtension entry symbol is missing from ${DLL_PATH}")
endif()
