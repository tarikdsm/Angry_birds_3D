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
