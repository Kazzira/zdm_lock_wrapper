enable_testing()
add_executable(
  zdm_lock_wrapper_tests
  "${CMAKE_CURRENT_SOURCE_DIR}/tests/unit_tests/lock_wrapper.test.cpp"
)

find_package(Catch2 REQUIRED CONFIG)

target_link_libraries(
  zdm_lock_wrapper_tests
  PUBLIC
  Catch2::Catch2WithMain
  zdm_lock_wrapper
)
