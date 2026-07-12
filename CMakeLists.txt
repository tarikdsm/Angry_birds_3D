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
option(NINHO_ENABLE_TEST_FACADES "Compile internal test-only facades" "${BUILD_TESTING}")
if(BUILD_TESTING AND NOT NINHO_ENABLE_TEST_FACADES)
  message(FATAL_ERROR "BUILD_TESTING requires NINHO_ENABLE_TEST_FACADES=ON")
endif()
if(NINHO_ENABLE_TEST_FACADES AND NOT BUILD_TESTING)
  message(FATAL_ERROR "NINHO_ENABLE_TEST_FACADES must be OFF when BUILD_TESTING is OFF")
endif()

if(BUILD_TESTING)
  set(_ninho_build_testing_json true)
else()
  set(_ninho_build_testing_json false)
endif()
if(NINHO_ENABLE_TEST_FACADES)
  set(_ninho_test_facades_json true)
  set(_ninho_test_facade_definition_json "present")
else()
  set(_ninho_test_facades_json false)
  set(_ninho_test_facade_definition_json "absent")
endif()
file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/ninho-build-contract.json" CONTENT
  "{\"schema\":\"ninho.native-build-contract.v1\",\"configuration\":\"${CMAKE_BUILD_TYPE}\",\"build_testing\":${_ninho_build_testing_json},\"test_facades\":${_ninho_test_facades_json},\"NINHO_ENABLE_TEST_FACADES\":\"${_ninho_test_facade_definition_json}\"}\n")
unset(_ninho_build_testing_json)
unset(_ninho_test_facades_json)
unset(_ninho_test_facade_definition_json)
add_subdirectory(native/kernel)
add_subdirectory(native/simulation)
add_subdirectory(native/spike)
if(NINHO_BUILD_GDEXTENSION)
  string(RANDOM LENGTH 32 ALPHABET 0123456789abcdef NINHO_GDEXTENSION_BUILD_TOKEN)
  set(NINHO_GDEXTENSION_STAMP_PATH
    "${CMAKE_BINARY_DIR}/native/extension/ninho_physics_extension.stamp")
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
      "-DSTAMP_PATH=${NINHO_GDEXTENSION_STAMP_PATH}"
      "-DEXPECTED_TOKEN=${NINHO_GDEXTENSION_BUILD_TOKEN}"
      "-DEXPECTED_CONFIG=${CMAKE_BUILD_TYPE}"
      "-DEXPECTED_TARGET=${GODOTCPP_TARGET}"
      "-DEXPECTED_DLL_NAME=ninho_physics.windows.${GODOTCPP_TARGET}.x86_64.dll"
      "-DDESCRIPTOR_PATH=${PROJECT_SOURCE_DIR}/game/bin/ninho_physics.gdextension"
      "-DDUMPBIN_EXECUTABLE=${NINHO_DUMPBIN_EXECUTABLE}"
      -P "${PROJECT_SOURCE_DIR}/cmake/VerifyGDExtension.cmake"
  )
  unset(_ninho_gdextension_dll)
endif()

unset(NINHO_GDEXTENSION_BUILD_TOKEN)
unset(NINHO_GDEXTENSION_STAMP_PATH)
