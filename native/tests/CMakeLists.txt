add_executable(ninho_physics_tests
  test_main.cpp
  radial_gravity_tests.cpp
  world_lifecycle_tests.cpp
  capability_tests.cpp
  projectile_ccd_tests.cpp
  pile_stability_tests.cpp
  determinism_tests.cpp
  scenario_capability_tests.cpp
)

target_include_directories(ninho_physics_tests PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(ninho_physics_tests PRIVATE ninho::physics)
target_compile_features(ninho_physics_tests PRIVATE cxx_std_20)

if(MSVC)
  target_compile_options(ninho_physics_tests PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:/W4;/permissive-;/fp:precise;/utf-8>"
  )
endif()

add_test(NAME radial COMMAND ninho_physics_tests --filter radial)
add_test(NAME world COMMAND ninho_physics_tests --filter world)
add_test(NAME contracts COMMAND ninho_physics_tests --filter contract)
add_test(NAME capability COMMAND ninho_physics_tests --filter capability)
add_test(NAME projectile_ccd COMMAND ninho_physics_tests --filter "bullet contacts pile")
add_test(NAME projectile_gate COMMAND ninho_physics_tests --filter "projectile gate")
add_test(NAME radial_fall_scenario COMMAND ninho_physics_tests --filter "radial fall")
add_test(NAME pile_stability COMMAND ninho_physics_tests --filter "radial pile")
add_test(NAME mass_ratio_scenario COMMAND ninho_physics_tests --filter "mass ratio pile")
add_test(NAME stress_protocol COMMAND ninho_physics_tests --filter "stress uses")
add_test(NAME determinism COMMAND ninho_physics_tests --filter "same scenario")
add_test(NAME canonical_hash COMMAND ninho_physics_tests --filter "canonical state hash")
add_test(NAME scenario_json COMMAND ninho_physics_tests --filter "scenario JSON")
add_test(NAME scenario_invalid_kind COMMAND ninho_physics_tests --filter "unknown enum")
add_test(NAME scenario_capabilities COMMAND ninho_physics_tests --filter "capability matrix")
add_test(NAME scenario_watchdog COMMAND ninho_physics_tests --filter "scenario watchdog")

find_package(Python3 3.11 REQUIRED COMPONENTS Interpreter)
add_test(
  NAME spike_json_smoke
  COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/json_smoke.py"
          "${CMAKE_BINARY_DIR}/native/spike/ninho_physics_spike${CMAKE_EXECUTABLE_SUFFIX}"
)

add_test(NAME runner_rejects_no_match
  COMMAND ninho_physics_tests --filter __ninho_missing__)
add_test(NAME runner_rejects_missing_filter_value
  COMMAND ninho_physics_tests --filter)
add_test(NAME runner_rejects_empty_filter_value
  COMMAND ninho_physics_tests --filter "")
add_test(NAME runner_rejects_unknown_option
  COMMAND ninho_physics_tests --unknown)
add_test(NAME runner_rejects_extra_arguments
  COMMAND ninho_physics_tests --filter radial extra)

set_tests_properties(
  runner_rejects_no_match
  runner_rejects_missing_filter_value
  runner_rejects_empty_filter_value
  runner_rejects_unknown_option
  runner_rejects_extra_arguments
  PROPERTIES WILL_FAIL TRUE
)

set_tests_properties(
  projectile_ccd
  projectile_gate
  radial_fall_scenario
  pile_stability
  mass_ratio_scenario
  stress_protocol
  determinism
  canonical_hash
  scenario_json
  scenario_invalid_kind
  scenario_capabilities
  scenario_watchdog
  PROPERTIES TIMEOUT 65
)

# This is one real process containing six scenarios repeated twice. The internal
# watchdog still enforces 60 seconds per scenario; this aggregate timeout only
# bounds the complete 12-run CLI protocol.
set_tests_properties(spike_json_smoke PROPERTIES TIMEOUT 150)
