include(FetchContent)
set(FETCHCONTENT_BASE_DIR "${PROJECT_SOURCE_DIR}/.fetchcontent-cache" CACHE PATH "" FORCE)
if(DEFINED GODOTCPP_TARGET)
  message(VERBOSE "Reserved godot-cpp target: ${GODOTCPP_TARGET}")
endif()

# Some workspace volumes do not record ownership, so Git requires nested
# FetchContent clones to be explicitly trusted. Keep each exception
# process-local and restricted to the immutable dependency checkout.
set(_ninho_had_git_config_count FALSE)
if(DEFINED ENV{GIT_CONFIG_COUNT})
  set(_ninho_had_git_config_count TRUE)
  set(_ninho_git_config_count "$ENV{GIT_CONFIG_COUNT}")
else()
  set(_ninho_git_config_count 0)
endif()
set(_ninho_next_git_config_count "${_ninho_git_config_count}")
set(_ninho_added_safe_directory_indices "")
macro(_ninho_add_process_safe_directory directory)
  set(_ninho_safe_directory_index "${_ninho_next_git_config_count}")
  math(EXPR _ninho_next_git_config_count "${_ninho_next_git_config_count} + 1")
  file(TO_CMAKE_PATH "${directory}" _ninho_safe_directory)
  set(ENV{GIT_CONFIG_COUNT} "${_ninho_next_git_config_count}")
  set(ENV{GIT_CONFIG_KEY_${_ninho_safe_directory_index}} "safe.directory")
  set(ENV{GIT_CONFIG_VALUE_${_ninho_safe_directory_index}} "${_ninho_safe_directory}")
  list(APPEND _ninho_added_safe_directory_indices "${_ninho_safe_directory_index}")
endmacro()

_ninho_add_process_safe_directory("${FETCHCONTENT_BASE_DIR}/box3d-src")
if(NINHO_BUILD_GDEXTENSION)
  _ninho_add_process_safe_directory("${FETCHCONTENT_BASE_DIR}/godot_cpp-src")
endif()

FetchContent_Declare(box3d
  GIT_REPOSITORY https://github.com/erincatto/box3d.git
  GIT_TAG 8441b4a06d6d09dcfb0b0f704df4d847d1437b92
  # Keep objects isolated per preset while reusing the immutable source clone.
  BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/box3d-build"
  GIT_SHALLOW FALSE)
FetchContent_MakeAvailable(box3d)
if(MSVC)
  # Determinism is part of the integration contract. Apply the mode to the
  # fetched C target itself; options on ninho consumers do not propagate here.
  target_compile_options(box3d PRIVATE /fp:precise)
endif()
message(STATUS "Box3D 0.1.0 pinned at 8441b4a06d6d09dcfb0b0f704df4d847d1437b92")

set(JSON_BuildTests OFF CACHE BOOL "Build nlohmann/json tests" FORCE)
set(JSON_Install OFF CACHE BOOL "Install nlohmann/json" FORCE)
FetchContent_Declare(nlohmann_json
  URL https://codeload.github.com/nlohmann/json/tar.gz/9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03
  URL_HASH SHA256=0dbc5e40a01ff142e7e68c03e85247a4dcede2f592d12d3677dee3664d17975a
  DOWNLOAD_EXTRACT_TIMESTAMP FALSE
  BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/nlohmann-json-build"
)
FetchContent_MakeAvailable(nlohmann_json)
message(STATUS
  "nlohmann/json v3.11.3 pinned at 9cca280a4d0ccf0c08f47a99aa71d1b0e52f8d03")

if(NINHO_BUILD_GDEXTENSION)
  # The adapter catches kernel construction/step failures at the Godot ABI.
  # Keep MSVC's STL and compiler exception modes coherent for both godot-cpp
  # and its consumers; no exception is permitted to leave the adapter methods.
  set(GODOTCPP_DISABLE_EXCEPTIONS OFF CACHE BOOL "" FORCE)
  set(GODOTCPP_SYSTEM_HEADERS ON CACHE BOOL "" FORCE)
  FetchContent_Declare(godot_cpp
    GIT_REPOSITORY https://github.com/godotengine/godot-cpp.git
    GIT_TAG e83fd0904c13356ed1d4c3d09f8bb9132bdc6b77
    BINARY_DIR "${CMAKE_BINARY_DIR}/_deps/godot-cpp-build"
    GIT_SHALLOW FALSE)
  FetchContent_MakeAvailable(godot_cpp)
  message(STATUS "godot-cpp pinned at e83fd0904c13356ed1d4c3d09f8bb9132bdc6b77")
endif()

foreach(_ninho_safe_directory_index IN LISTS _ninho_added_safe_directory_indices)
  unset(ENV{GIT_CONFIG_KEY_${_ninho_safe_directory_index}})
  unset(ENV{GIT_CONFIG_VALUE_${_ninho_safe_directory_index}})
endforeach()
if(_ninho_had_git_config_count)
  set(ENV{GIT_CONFIG_COUNT} "${_ninho_git_config_count}")
else()
  unset(ENV{GIT_CONFIG_COUNT})
endif()
unset(_ninho_added_safe_directory_indices)
unset(_ninho_git_config_count)
unset(_ninho_had_git_config_count)
unset(_ninho_next_git_config_count)
unset(_ninho_safe_directory)
unset(_ninho_safe_directory_index)
unset(_ninho_add_process_safe_directory)
