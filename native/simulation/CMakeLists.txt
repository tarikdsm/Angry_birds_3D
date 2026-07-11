add_library(ninho_simulation_kernel STATIC
  src/content.cpp
  src/canonical_state.cpp
  src/session.cpp
  src/session_builder.cpp
  src/launch_system.cpp
  src/gravity_field_ability_system.cpp
  src/damage_system.cpp
)
add_library(ninho::simulation ALIAS ninho_simulation_kernel)

target_include_directories(ninho_simulation_kernel PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)
target_link_libraries(ninho_simulation_kernel
  PUBLIC ninho::physics
  PRIVATE nlohmann_json::nlohmann_json
)
target_compile_features(ninho_simulation_kernel PUBLIC cxx_std_20)
if(BUILD_TESTING)
  target_compile_definitions(ninho_simulation_kernel PRIVATE NINHO_ENABLE_TEST_FACADES=1)
endif()

if(MSVC)
  target_compile_options(ninho_simulation_kernel PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:/W4;/permissive-;/fp:precise;/utf-8>"
  )
endif()

if(BUILD_TESTING)
  add_executable(ninho_simulation_tests
    tests/test_main.cpp
    tests/kernel_contract_tests.cpp
    tests/content_tests.cpp
    tests/session_tests.cpp
    tests/launch_fsm_tests.cpp
    tests/gravity_field_ability_tests.cpp
    tests/damage_anchor_tests.cpp
  )

  target_include_directories(ninho_simulation_tests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/tests"
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${PROJECT_SOURCE_DIR}/native/kernel/src"
  )
  target_link_libraries(ninho_simulation_tests PRIVATE
    ninho::simulation
    nlohmann_json::nlohmann_json
  )
  target_compile_definitions(ninho_simulation_tests PRIVATE
    NINHO_SOURCE_DIR="${PROJECT_SOURCE_DIR}"
    NINHO_ENABLE_TEST_FACADES=1
  )
  target_compile_features(ninho_simulation_tests PRIVATE cxx_std_20)

  if(MSVC)
    target_compile_options(ninho_simulation_tests PRIVATE
      "$<$<COMPILE_LANGUAGE:CXX>:/W4;/permissive-;/fp:precise;/utf-8>"
    )
  endif()

  add_test(
    NAME launch_fsm
    COMMAND ninho_simulation_tests --filter "launch fsm"
  )
  add_test(
    NAME gravity_field_ability
    COMMAND ninho_simulation_tests --filter "gravity field ability"
  )
  add_test(
    NAME damage_anchor
    COMMAND ninho_simulation_tests --filter "damage anchor"
  )
  add_test(
    NAME simulation_contracts
    COMMAND ninho_simulation_tests --filter "simulation contracts"
  )
  add_test(
    NAME content_manifest
    COMMAND ninho_simulation_tests --filter content
  )
  add_test(
    NAME session_bootstrap
    COMMAND ninho_simulation_tests --filter "session bootstrap"
  )
  add_test(
    NAME session_restart
    COMMAND ninho_simulation_tests --filter "session restart"
  )
  add_test(
    NAME session_reconfigure
    COMMAND ninho_simulation_tests --filter "session reconfigure"
  )
  add_test(
    NAME simulation_runner_rejects_no_match
    COMMAND ninho_simulation_tests --filter __ninho_simulation_missing__
  )
  add_test(
    NAME simulation_runner_rejects_missing_filter_value
    COMMAND ninho_simulation_tests --filter
  )
  add_test(
    NAME simulation_runner_rejects_empty_filter_value
    COMMAND ninho_simulation_tests --filter ""
  )
  add_test(
    NAME simulation_runner_rejects_unknown_option
    COMMAND ninho_simulation_tests --unknown
  )
  add_test(
    NAME simulation_runner_rejects_extra_arguments
    COMMAND ninho_simulation_tests --filter "simulation contracts" extra
  )

  set_tests_properties(
    simulation_runner_rejects_no_match
    simulation_runner_rejects_missing_filter_value
    simulation_runner_rejects_empty_filter_value
    simulation_runner_rejects_unknown_option
    simulation_runner_rejects_extra_arguments
    PROPERTIES WILL_FAIL TRUE
  )
endif()
