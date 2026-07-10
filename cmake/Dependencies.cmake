include(FetchContent)
set(FETCHCONTENT_BASE_DIR "${PROJECT_SOURCE_DIR}/.fetchcontent-cache" CACHE PATH "" FORCE)
if(DEFINED GODOTCPP_TARGET)
  message(VERBOSE "Reserved godot-cpp target: ${GODOTCPP_TARGET}")
endif()

# Some workspace volumes do not record ownership, so Git requires the nested
# FetchContent clone to be explicitly trusted. Keep the exception process-local
# and restricted to the immutable Box3D checkout.
set(_ninho_had_git_config_count FALSE)
if(DEFINED ENV{GIT_CONFIG_COUNT})
  set(_ninho_had_git_config_count TRUE)
  set(_ninho_git_config_count "$ENV{GIT_CONFIG_COUNT}")
else()
  set(_ninho_git_config_count 0)
endif()
set(_ninho_safe_directory_index "${_ninho_git_config_count}")
math(EXPR _ninho_next_git_config_count "${_ninho_git_config_count} + 1")
file(TO_CMAKE_PATH "${FETCHCONTENT_BASE_DIR}/box3d-src" _ninho_box3d_safe_directory)
set(ENV{GIT_CONFIG_COUNT} "${_ninho_next_git_config_count}")
set(ENV{GIT_CONFIG_KEY_${_ninho_safe_directory_index}} "safe.directory")
set(ENV{GIT_CONFIG_VALUE_${_ninho_safe_directory_index}} "${_ninho_box3d_safe_directory}")

FetchContent_Declare(box3d
  GIT_REPOSITORY https://github.com/erincatto/box3d.git
  GIT_TAG 8441b4a06d6d09dcfb0b0f704df4d847d1437b92
  # Keep objects isolated per preset while reusing the immutable source clone.
  BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/box3d-build"
  GIT_SHALLOW FALSE)
FetchContent_MakeAvailable(box3d)
message(STATUS "Box3D 0.1.0 pinned at 8441b4a06d6d09dcfb0b0f704df4d847d1437b92")

unset(ENV{GIT_CONFIG_KEY_${_ninho_safe_directory_index}})
unset(ENV{GIT_CONFIG_VALUE_${_ninho_safe_directory_index}})
if(_ninho_had_git_config_count)
  set(ENV{GIT_CONFIG_COUNT} "${_ninho_git_config_count}")
else()
  unset(ENV{GIT_CONFIG_COUNT})
endif()
unset(_ninho_box3d_safe_directory)
unset(_ninho_git_config_count)
unset(_ninho_had_git_config_count)
unset(_ninho_next_git_config_count)
unset(_ninho_safe_directory_index)
