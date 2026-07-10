add_library(ninho_physics_kernel STATIC
  src/radial_gravity.cpp
)
add_library(ninho::physics ALIAS ninho_physics_kernel)

target_include_directories(ninho_physics_kernel PUBLIC
  "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
)
target_link_libraries(ninho_physics_kernel PRIVATE box3d::box3d)
target_compile_features(ninho_physics_kernel PUBLIC cxx_std_20)

if(MSVC)
  target_compile_options(ninho_physics_kernel PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:/W4;/permissive-;/fp:precise>"
  )
endif()
