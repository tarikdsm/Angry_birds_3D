add_executable(ninho_physics_tests
  test_main.cpp
  radial_gravity_tests.cpp
)

target_include_directories(ninho_physics_tests PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(ninho_physics_tests PRIVATE ninho::physics)
target_compile_features(ninho_physics_tests PRIVATE cxx_std_20)

if(MSVC)
  target_compile_options(ninho_physics_tests PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:/W4;/permissive-;/fp:precise>"
  )
endif()

add_test(NAME radial COMMAND ninho_physics_tests --filter radial)

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
