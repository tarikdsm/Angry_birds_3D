if(NOT DEFINED CACHE_DIRECTORY OR CACHE_DIRECTORY STREQUAL "")
  message(FATAL_ERROR "CACHE_DIRECTORY is required")
endif()

file(MAKE_DIRECTORY "${CACHE_DIRECTORY}")
file(WRITE "${CACHE_DIRECTORY}/extension_list.cfg"
  "res://bin/ninho_physics.gdextension\n")
