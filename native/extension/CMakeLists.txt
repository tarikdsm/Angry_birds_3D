add_library(ninho_physics_extension SHARED
  src/box3d_world_node.cpp
  src/register_types.cpp
)
target_include_directories(ninho_physics_extension PRIVATE include)
target_link_libraries(ninho_physics_extension PRIVATE ninho::physics godot::cpp)
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

if(MSVC)
  # The kernel reports invalid inputs with Status but can throw for construction
  # and catastrophic Box3D failures. /EHsc keeps those catchable inside this DLL;
  # every Godot-facing operation converts them to physics_fault before returning.
  target_compile_options(ninho_physics_extension PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:/W4;/permissive-;/fp:precise;/utf-8;/EHsc;/external:anglebrackets;/external:W0>"
  )
endif()

if(BUILD_TESTING)
  add_executable(ninho_extension_adapter_tests
    "${PROJECT_SOURCE_DIR}/native/tests/test_main.cpp"
    tests/adapter_helpers_tests.cpp
  )
  target_include_directories(ninho_extension_adapter_tests PRIVATE
    include
    "${PROJECT_SOURCE_DIR}/native/tests"
  )
  target_link_libraries(ninho_extension_adapter_tests PRIVATE ninho::physics godot::cpp)
  target_compile_features(ninho_extension_adapter_tests PRIVATE cxx_std_20)
  if(MSVC)
    target_compile_options(ninho_extension_adapter_tests PRIVATE
      "$<$<COMPILE_LANGUAGE:CXX>:/W4;/permissive-;/fp:precise;/utf-8;/EHsc;/external:anglebrackets;/external:W0>"
    )
  endif()
  add_test(NAME gdextension_adapter_helpers COMMAND ninho_extension_adapter_tests)
endif()
