cmake_minimum_required(VERSION 3.16)
project(loxperm LANGUAGES C)

option(LOXPERM_WIDE_MASK "Build with 64-bit masks (up to 64 conditions)" OFF)

add_library(loxperm INTERFACE)
target_include_directories(loxperm INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_compile_features(loxperm INTERFACE c_std_99)

if (LOXPERM_WIDE_MASK)
    target_compile_definitions(loxperm INTERFACE LOXPERM_WIDE_MASK=1)
endif()

if (MSVC)
    target_compile_options(loxperm INTERFACE /W4)
else()
    target_compile_options(loxperm INTERFACE -Wall -Wextra -Wpedantic)
endif()

add_executable(loxperm_pump_start examples/pump_start.c)
target_link_libraries(loxperm_pump_start PRIVATE loxperm)

add_executable(loxperm_interlock_first_out examples/interlock_first_out.c)
target_link_libraries(loxperm_interlock_first_out PRIVATE loxperm)

add_executable(loxperm_tests tests/test_loxperm.c)
target_link_libraries(loxperm_tests PRIVATE loxperm)

enable_testing()
add_test(NAME loxperm_tests COMMAND loxperm_tests)

