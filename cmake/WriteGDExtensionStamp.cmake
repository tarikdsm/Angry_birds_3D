foreach(_required IN ITEMS DLL_PATH STAMP_PATH TOKEN CONFIG TARGET DLL_NAME)
  if(NOT DEFINED ${_required} OR "${${_required}}" STREQUAL "")
    message(FATAL_ERROR "Missing ${_required} for GDExtension build stamp")
  endif()
endforeach()
if(NOT EXISTS "${DLL_PATH}")
  message(FATAL_ERROR "Cannot stamp missing GDExtension DLL: ${DLL_PATH}")
endif()

file(TO_CMAKE_PATH "${DLL_PATH}" _normalized_dll_path)
file(SHA256 "${DLL_PATH}" _dll_sha256)
file(WRITE "${STAMP_PATH}"
  "TOKEN=${TOKEN}\n"
  "CONFIG=${CONFIG}\n"
  "TARGET=${TARGET}\n"
  "DLL_PATH=${_normalized_dll_path}\n"
  "DLL_NAME=${DLL_NAME}\n"
  "SHA256=${_dll_sha256}\n"
)
