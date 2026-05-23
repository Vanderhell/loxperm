cmake_minimum_required(VERSION 3.16)
project(loxperm VERSION 0.1.0 LANGUAGES C)

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

option(LOXPERM_BUILD_TESTS "Build loxperm unit tests" ON)
option(LOXPERM_BUILD_EXAMPLES "Build loxperm examples" ON)
option(LOXPERM_WIDE_MASK "Build with 64-bit masks (up to 64 conditions)" OFF)
option(LOXPERM_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(LOXPERM_ENABLE_SANITIZERS "Enable ASan/UBSan where supported" OFF)

add_library(loxperm INTERFACE)
add_library(loxperm::loxperm ALIAS loxperm)
target_include_directories(loxperm INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_compile_features(loxperm INTERFACE c_std_99)

if (LOXPERM_WIDE_MASK)
    target_compile_definitions(loxperm INTERFACE LOXPERM_WIDE_MASK=1)
endif()

if (MSVC)
    target_compile_options(loxperm INTERFACE /W4)
    if (LOXPERM_WARNINGS_AS_ERRORS)
        target_compile_options(loxperm INTERFACE /WX)
    endif()
else()
    target_compile_options(loxperm INTERFACE -Wall -Wextra -Wpedantic)
    if (LOXPERM_WARNINGS_AS_ERRORS)
        target_compile_options(loxperm INTERFACE -Werror)
    endif()
endif()

function(loxperm_apply_sanitizers target_name)
    if (NOT LOXPERM_ENABLE_SANITIZERS)
        return()
    endif()
    if (MSVC)
        message(STATUS "LOXPERM_ENABLE_SANITIZERS is ON but MSVC sanitizers are not configured")
        return()
    endif()
    if (CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target_name} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer)
        target_link_options(${target_name} PRIVATE -fsanitize=address,undefined)
    endif()
endfunction()

if (LOXPERM_BUILD_EXAMPLES)
    add_executable(loxperm_pump_start examples/pump_start.c)
    target_link_libraries(loxperm_pump_start PRIVATE loxperm)
    loxperm_apply_sanitizers(loxperm_pump_start)

    add_executable(loxperm_interlock_first_out examples/interlock_first_out.c)
    target_link_libraries(loxperm_interlock_first_out PRIVATE loxperm)
    loxperm_apply_sanitizers(loxperm_interlock_first_out)
endif()

if (LOXPERM_BUILD_TESTS)
    add_executable(loxperm_tests tests/test_loxperm.c)
    target_link_libraries(loxperm_tests PRIVATE loxperm)
    loxperm_apply_sanitizers(loxperm_tests)

    add_executable(loxperm_tests_wide_mask tests/test_loxperm_wide_mask.c)
    target_compile_definitions(loxperm_tests_wide_mask PRIVATE LOXPERM_WIDE_MASK=1)
    target_link_libraries(loxperm_tests_wide_mask PRIVATE loxperm)
    loxperm_apply_sanitizers(loxperm_tests_wide_mask)

    enable_testing()
    add_test(NAME loxperm_tests COMMAND loxperm_tests)
    add_test(NAME loxperm_tests_wide_mask COMMAND loxperm_tests_wide_mask)
endif()

install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

install(TARGETS loxperm
    EXPORT loxpermTargets
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

set(LOXPERM_INSTALL_CMAKEDIR "${CMAKE_INSTALL_LIBDIR}/cmake/loxperm")

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/loxpermConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/loxpermConfig.cmake"
    INSTALL_DESTINATION "${LOXPERM_INSTALL_CMAKEDIR}"
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/loxpermConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/loxpermConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/loxpermConfigVersion.cmake"
    DESTINATION "${LOXPERM_INSTALL_CMAKEDIR}"
)

install(EXPORT loxpermTargets
    FILE loxpermTargets.cmake
    NAMESPACE loxperm::
    DESTINATION "${LOXPERM_INSTALL_CMAKEDIR}"
)
