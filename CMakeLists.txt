cmake_minimum_required(VERSION 3.22)
project(ninho_orbital VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(NINHO_BUILD_GDEXTENSION "Build the Godot presentation adapter" OFF)
include(cmake/Dependencies.cmake)
include(CTest)
add_subdirectory(native/kernel)
add_subdirectory(native/spike)
if(NINHO_BUILD_GDEXTENSION)
  add_subdirectory(native/extension)
endif()
if(BUILD_TESTING)
  add_subdirectory(native/tests)
endif()

if(NINHO_BUILD_GDEXTENSION AND BUILD_TESTING)
  find_program(NINHO_DUMPBIN_EXECUTABLE dumpbin REQUIRED)
  set(_ninho_gdextension_dll
    "${PROJECT_SOURCE_DIR}/game/bin/ninho_physics.windows.${GODOTCPP_TARGET}.x86_64.dll")
  add_test(
    NAME gdextension_binary_exists
    COMMAND "${CMAKE_COMMAND}"
      "-DDLL_PATH=${_ninho_gdextension_dll}"
      "-DDUMPBIN_EXECUTABLE=${NINHO_DUMPBIN_EXECUTABLE}"
      -P "${PROJECT_SOURCE_DIR}/cmake/VerifyGDExtension.cmake"
  )
  unset(_ninho_gdextension_dll)
endif()
