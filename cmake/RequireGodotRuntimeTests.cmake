if(NINHO_BUILD_GDEXTENSION AND BUILD_TESTING)
  if(NOT DEFINED NINHO_GODOT_EXECUTABLE
      OR "${NINHO_GODOT_EXECUTABLE}" STREQUAL ""
      OR NOT EXISTS "${NINHO_GODOT_EXECUTABLE}"
      OR IS_DIRECTORY "${NINHO_GODOT_EXECUTABLE}")
    message(FATAL_ERROR
      "NINHO_BUILD_GDEXTENSION=ON with BUILD_TESTING=ON requires the pinned "
      "Godot runtime. Expected NINHO_GODOT_EXECUTABLE='${NINHO_GODOT_EXECUTABLE}'. "
      "Run tools/bootstrap.ps1 -InstallPortable or configure with "
      "-DNINHO_GODOT_EXECUTABLE=<absolute-path-to-Godot-4.5.1>.")
  endif()
endif()
