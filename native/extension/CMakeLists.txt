add_library(ninho_physics_extension SHARED
  src/adapter_helpers.cpp
  src/box3d_world_node.cpp
  src/orbital_session_node.cpp
  src/register_types.cpp
)
target_include_directories(ninho_physics_extension PRIVATE include)
target_link_libraries(ninho_physics_extension PRIVATE ninho::physics ninho::simulation godot::cpp)
target_compile_features(ninho_physics_extension PRIVATE cxx_std_20)

get_target_property(GODOTCPP_SUFFIX godot-cpp GODOTCPP_SUFFIX)
if(NOT GODOTCPP_SUFFIX)
  message(FATAL_ERROR "godot-cpp did not expose GODOTCPP_SUFFIX")
endif()
set_target_properties(ninho_physics_extension PROPERTIES
  PREFIX ""
  OUTPUT_NAME "ninho_physics${GODOTCPP_SUFFIX}"
  RUNTIME_OUTPUT_DIRECTORY "$<1:${PROJECT_SOURCE_DIR}/game/bin>"
)
add_custom_command(
  TARGET ninho_physics_extension POST_BUILD
  BYPRODUCTS "${NINHO_GDEXTENSION_STAMP_PATH}"
  COMMAND "${CMAKE_COMMAND}"
    "-DDLL_PATH=$<TARGET_FILE:ninho_physics_extension>"
    "-DSTAMP_PATH=${NINHO_GDEXTENSION_STAMP_PATH}"
    "-DTOKEN=${NINHO_GDEXTENSION_BUILD_TOKEN}"
    "-DCONFIG=$<CONFIG>"
    "-DTARGET=${GODOTCPP_TARGET}"
    "-DDLL_NAME=$<TARGET_FILE_NAME:ninho_physics_extension>"
    -P "${PROJECT_SOURCE_DIR}/cmake/WriteGDExtensionStamp.cmake"
  VERBATIM
)

if(MSVC)
  # The kernel reports invalid inputs with Status but can throw for construction
  # and catastrophic Box3D failures. /EHsc keeps those catchable inside this DLL;
  # every Godot-facing operation converts them to physics_fault before returning.
  target_compile_options(ninho_physics_extension PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:/W4;/permissive-;/fp:precise;/utf-8;/EHsc>"
  )
endif()

if(BUILD_TESTING)
  add_executable(ninho_extension_adapter_tests
    "${PROJECT_SOURCE_DIR}/native/tests/test_main.cpp"
    src/adapter_helpers.cpp
    src/orbital_session_node.cpp
    tests/adapter_helpers_tests.cpp
    tests/orbital_session_adapter_tests.cpp
    tests/registration_contract_tests.cpp
    tests/session_frame_batch_tests.cpp
  )
  target_include_directories(ninho_extension_adapter_tests PRIVATE
    include
    "${PROJECT_SOURCE_DIR}/native/tests"
    "${PROJECT_SOURCE_DIR}/native/simulation/src"
  )
  target_link_libraries(ninho_extension_adapter_tests PRIVATE
    ninho::physics
    ninho::simulation
    godot::cpp
  )
  target_compile_definitions(ninho_extension_adapter_tests PRIVATE
    NINHO_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
    NINHO_ENABLE_TEST_FACADES=1
  )
  target_compile_features(ninho_extension_adapter_tests PRIVATE cxx_std_20)
  if(MSVC)
    target_compile_options(ninho_extension_adapter_tests PRIVATE
      "$<$<COMPILE_LANGUAGE:CXX>:/W4;/permissive-;/fp:precise;/utf-8;/EHsc>"
    )
  endif()
  add_test(NAME gdextension_adapter_helpers COMMAND ninho_extension_adapter_tests)

  set(_ninho_godot_executable
    "${PROJECT_SOURCE_DIR}/.tools/godot/Godot_v4.5.1-stable_win64.exe")
  if(EXISTS "${_ninho_godot_executable}")
    add_test(
      NAME gdextension_project_import
      COMMAND "${CMAKE_COMMAND}"
        "-DCACHE_DIRECTORY=${PROJECT_SOURCE_DIR}/game/.godot"
        -P "${PROJECT_SOURCE_DIR}/cmake/PrepareGDExtensionTest.cmake"
    )
    add_test(
      NAME gdextension_vertical_slice
      COMMAND "${_ninho_godot_executable}"
        --headless
        --path "${PROJECT_SOURCE_DIR}/game"
        --script res://tests/orbital_session_node_smoke.gd
    )
    set_tests_properties(gdextension_project_import PROPERTIES
      FIXTURES_SETUP ninho_gdextension_import)
    set_tests_properties(gdextension_vertical_slice PROPERTIES
      FIXTURES_REQUIRED ninho_gdextension_import
      PASS_REGULAR_EXPRESSION "ORBITAL_SESSION_NODE_SMOKE_OK")
  endif()
  unset(_ninho_godot_executable)
endif()
