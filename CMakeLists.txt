cmake_minimum_required(VERSION 3.16)
project(loxperm VERSION 0.1.0 LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

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

function(loxperm_enable_project_warnings target_name)
    if (MSVC)
        target_compile_options(${target_name} PRIVATE /W4)
        if (LOXPERM_WARNINGS_AS_ERRORS)
            target_compile_options(${target_name} PRIVATE /WX)
        endif()
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
        if (LOXPERM_WARNINGS_AS_ERRORS)
            target_compile_options(${target_name} PRIVATE -Werror)
        endif()
    endif()
endfunction()

function(loxperm_add_compile_check_test test_name language standard required_definitions should_compile)
    set(test_root "${CMAKE_CURRENT_BINARY_DIR}/compat/${test_name}")
    file(MAKE_DIRECTORY "${test_root}")
    file(TO_CMAKE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/include" loxperm_include_dir)

    if (language STREQUAL "C")
        set(check_module "CheckCSourceCompiles")
        set(check_command "check_c_source_compiles")
        set(source_code "#include <loxperm/loxperm.h>\nint main(void) { return 0; }\n")
        set(standard_var "C_STANDARD")
    else()
        set(check_module "CheckCXXSourceCompiles")
        set(check_command "check_cxx_source_compiles")
        set(source_code "#include <loxperm/loxperm.h>\nint main() { loxperm_chain_t c; return loxperm_chain_init(&c, nullptr, 0) == LOXPERM_OK ? 0 : 1; }\n")
        set(standard_var "CXX_STANDARD")
    endif()

    file(WRITE "${test_root}/CMakeLists.txt" "cmake_minimum_required(VERSION 3.16)\nproject(${test_name} LANGUAGES ${language})\ninclude(${check_module})\nset(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)\nset(CMAKE_REQUIRED_INCLUDES \"${loxperm_include_dir}\")\nset(CMAKE_REQUIRED_DEFINITIONS ${required_definitions})\nset(CMAKE_${standard_var} ${standard})\nset(CMAKE_${standard_var}_REQUIRED ON)\nset(CMAKE_${standard_var}_EXTENSIONS OFF)\n${check_command}(\"${source_code}\" LOXPERM_COMPILES)\nif(${should_compile})\n  if(NOT LOXPERM_COMPILES)\n    message(FATAL_ERROR \"expected compile success\")\n  endif()\nelse()\n  if(LOXPERM_COMPILES)\n    message(FATAL_ERROR \"expected compile failure\")\n  endif()\nendif()\n")

    add_test(NAME ${test_name} COMMAND ${CMAKE_COMMAND} -S "${test_root}" -B "${test_root}/build")
endfunction()

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
    loxperm_enable_project_warnings(loxperm_pump_start)
    loxperm_apply_sanitizers(loxperm_pump_start)

    add_executable(loxperm_interlock_first_out examples/interlock_first_out.c)
    target_link_libraries(loxperm_interlock_first_out PRIVATE loxperm)
    loxperm_enable_project_warnings(loxperm_interlock_first_out)
    loxperm_apply_sanitizers(loxperm_interlock_first_out)
endif()

if (LOXPERM_BUILD_TESTS)
    add_executable(loxperm_tests tests/test_loxperm.c)
    target_link_libraries(loxperm_tests PRIVATE loxperm)
    loxperm_enable_project_warnings(loxperm_tests)
    loxperm_apply_sanitizers(loxperm_tests)

    add_executable(loxperm_tests_wide_mask tests/test_loxperm_wide_mask.c)
    target_compile_definitions(loxperm_tests_wide_mask PRIVATE LOXPERM_WIDE_MASK=1)
    target_link_libraries(loxperm_tests_wide_mask PRIVATE loxperm)
    loxperm_enable_project_warnings(loxperm_tests_wide_mask)
    loxperm_apply_sanitizers(loxperm_tests_wide_mask)

    enable_testing()
    add_test(NAME loxperm_tests COMMAND loxperm_tests)
    add_test(NAME loxperm_tests_wide_mask COMMAND loxperm_tests_wide_mask)

    loxperm_add_compile_check_test(loxperm_config_c_1 C 99 "-DLOXPERM_MAX_CONDITIONS=1" ON)
    loxperm_add_compile_check_test(loxperm_config_c_32 C 99 "-DLOXPERM_MAX_CONDITIONS=32" ON)
    loxperm_add_compile_check_test(loxperm_config_c_64 C 99 "-DLOXPERM_WIDE_MASK=1;-DLOXPERM_MAX_CONDITIONS=64" ON)
    loxperm_add_compile_check_test(loxperm_config_c_0 C 99 "-DLOXPERM_MAX_CONDITIONS=0" OFF)
    loxperm_add_compile_check_test(loxperm_config_c_33 C 99 "-DLOXPERM_MAX_CONDITIONS=33" OFF)
    loxperm_add_compile_check_test(loxperm_config_c_65 C 99 "-DLOXPERM_WIDE_MASK=1;-DLOXPERM_MAX_CONDITIONS=65" OFF)

    loxperm_add_compile_check_test(loxperm_cpp11_include CXX 11 "" ON)
    loxperm_add_compile_check_test(loxperm_cpp17_include CXX 17 "" ON)
    loxperm_add_compile_check_test(loxperm_cpp20_include CXX 20 "" ON)
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
