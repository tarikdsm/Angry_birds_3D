add_library(ninho_physics_kernel STATIC
  src/physics_world.cpp
  src/radial_gravity.cpp
  src/scenario.cpp
  src/box3d_replay_conformance.cpp
  src/box3d_allocator_probe.cpp
)
add_library(ninho::physics ALIAS ninho_physics_kernel)

target_include_directories(ninho_physics_kernel PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)
target_link_libraries(ninho_physics_kernel PRIVATE box3d::box3d)
target_compile_features(ninho_physics_kernel PUBLIC cxx_std_20)
target_compile_definitions(ninho_physics_kernel PRIVATE
  NINHO_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
)

if(WIN32)
  target_link_libraries(ninho_physics_kernel PRIVATE Psapi)
endif()

if(MSVC)
  target_compile_options(ninho_physics_kernel PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:/W4;/permissive-;/fp:precise;/utf-8>"
  )
endif()
