add_executable(ninho_physics_spike main.cpp)
target_include_directories(ninho_physics_spike PRIVATE
  "${PROJECT_SOURCE_DIR}/native/kernel/src"
)
target_link_libraries(ninho_physics_spike PRIVATE ninho::physics)
target_compile_features(ninho_physics_spike PRIVATE cxx_std_20)
target_compile_definitions(ninho_physics_spike PRIVATE
  NINHO_BUILD_TYPE="${CMAKE_BUILD_TYPE}"
)

if(MSVC)
  target_compile_options(ninho_physics_spike PRIVATE
    "$<$<COMPILE_LANGUAGE:CXX>:/W4;/permissive-;/fp:precise;/utf-8>"
  )
endif()
