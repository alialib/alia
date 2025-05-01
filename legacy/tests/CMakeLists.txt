find_package(Catch2 REQUIRED)

# Add the unit test runner.
file(GLOB_RECURSE CORE_UNIT_TEST_FILES CONFIGURE_DEPENDS "unit/core/*.cpp")
add_executable(core_unit_test_runner ${CORE_UNIT_TEST_FILES})
target_link_libraries(core_unit_test_runner
    PRIVATE alia_core Catch2::Catch2WithMain)
target_include_directories(core_unit_test_runner
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/unit/core)

# Add tests that are supposed to cause compilation errors.
# Specifically, find all .cpp files in the compilation/ directory and
# generate test cases that try to compile them once with
# ALIA_TEST_COMPILATION_FAILURE #defined and once without it. The files are
# expected to compile successfully without the #define but generate a
# compilation error when the #define is provided.
file(GLOB_RECURSE COMPILATION_TEST_FILES "compilation/*.cpp")
set(COMPILATION_TEST_SCRIPT
    "${CMAKE_CURRENT_BINARY_DIR}/invoke_compilation_tests.cmake")
file(WRITE ${COMPILATION_TEST_SCRIPT} "")
get_target_property(COMPILE_DEFS core_unit_test_runner COMPILE_DEFINITIONS)
foreach(TEST_FILE ${COMPILATION_TEST_FILES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)

    # We implement these tests by creating libraries that are built from the
    # source file in question. Then we create actual CMake test cases that try
    # to build those targets.

    # This is the "control" case (which omits the error and should build).
    add_library(${TEST_NAME}_control ${TEST_FILE})
    target_link_libraries(${TEST_NAME}_control alia_core)
    set_target_properties(${TEST_NAME}_control
                          PROPERTIES
                          EXCLUDE_FROM_ALL 1
                          EXCLUDE_FROM_DEFAULT_BUILD 1)
    add_test(NAME ${TEST_NAME}_control
        COMMAND ${CMAKE_COMMAND} --build . --target ${TEST_NAME}_control
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})

    # This is the actual failure case.
    add_library(${TEST_NAME} ${TEST_FILE})
    target_link_libraries(${TEST_NAME} alia_core)
    target_compile_definitions(
        ${TEST_NAME} PRIVATE ALIA_TEST_COMPILATION_FAILURE)
    set_target_properties(${TEST_NAME} PROPERTIES EXCLUDE_FROM_ALL 1
                          EXCLUDE_FROM_DEFAULT_BUILD 1)
    add_test(NAME ${TEST_NAME}
        COMMAND ${CMAKE_COMMAND} --build . --target ${TEST_NAME}
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
    set_tests_properties(${TEST_NAME} PROPERTIES WILL_FAIL TRUE)
endforeach()

# Add a target for running the unit tests.
add_custom_target(
    core_unit_tests
    # Create a fresh 'core-unit-testing' directory within the build dir and run
    # the tests within that.
    COMMAND ${CMAKE_COMMAND} -E remove_directory core-unit-testing
    COMMAND ${CMAKE_COMMAND} -E make_directory core-unit-testing
    COMMAND ${CMAKE_COMMAND} -E chdir core-unit-testing
                             $<TARGET_FILE:core_unit_test_runner>
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS core_unit_test_runner)

# On Clang debug builds, the proper CMake test associated with the unit tests
# includes test coverage reporting.
if(IS_CLANG AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    string(REPLACE clang++ llvm-profdata LLVM_PROFDATA ${CMAKE_CXX_COMPILER})
    string(REPLACE clang++ llvm-cov LLVM_COV ${CMAKE_CXX_COMPILER})
    add_custom_target(
        core_unit_test_coverage
        COMMAND ${CMAKE_COMMAND} --build . --target core_unit_tests
        COMMAND ${LLVM_PROFDATA} merge -sparse core-unit-testing/default.profraw
                -o default.profdata
        COMMAND ${LLVM_COV} show -instr-profile=default.profdata
                $<TARGET_FILE:core_unit_test_runner>
                >${CMAKE_CURRENT_BINARY_DIR}/core_coverage.txt
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_test(
        NAME core_unit_test_coverage
        COMMAND ${CMAKE_COMMAND} --build . --target core_unit_test_coverage
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
else()
    add_test(
        NAME core_unit_tests
        COMMAND ${CMAKE_COMMAND} --build . --target core_unit_tests
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
endif()

# CMake doesn't seem to generate a test target in some cases (which I haven't
# quite figured out), so generate a custom one.
add_custom_target(
    ctest
    COMMAND ctest -C ${CMAKE_BUILD_TYPE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

# The rest of the file is
if (NOT ALIA_ENABLE_UI)
    return()
endif()

# Add the unit test runner.
file(GLOB_RECURSE UI_UNIT_TEST_FILES CONFIGURE_DEPENDS "unit/ui/*.cpp")
add_executable(ui_unit_test_runner ${UI_UNIT_TEST_FILES})
target_link_libraries(ui_unit_test_runner
    PRIVATE alia_ui Catch2::Catch2WithMain)
target_include_directories(ui_unit_test_runner
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/unit/ui)

# Add a target for running the unit tests.
add_custom_target(
    ui_unit_tests
    # Create a fresh 'ui-unit-testing' directory within the build dir and run
    # the tests with that.
    COMMAND ${CMAKE_COMMAND} -E remove_directory ui-unit-testing
    COMMAND ${CMAKE_COMMAND} -E make_directory ui-unit-testing
    COMMAND ${CMAKE_COMMAND} -E chdir ui-unit-testing
                             $<TARGET_FILE:ui_unit_test_runner>
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS ui_unit_test_runner)

# On Clang debug builds, the proper CMake test associated with the unit tests
# includes test coverage reporting.
if(IS_CLANG AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    string(REPLACE clang++ llvm-profdata LLVM_PROFDATA ${CMAKE_CXX_COMPILER})
    string(REPLACE clang++ llvm-cov LLVM_COV ${CMAKE_CXX_COMPILER})
    add_custom_target(
        ui_unit_test_coverage
        COMMAND ${CMAKE_COMMAND} --build . --target ui_unit_tests
        COMMAND ${LLVM_PROFDATA} merge -sparse ui-unit-testing/default.profraw
                -o default.profdata
        COMMAND ${LLVM_COV} show -instr-profile=default.profdata
                $<TARGET_FILE:ui_unit_test_runner>
                >${CMAKE_CURRENT_BINARY_DIR}/ui_coverage.txt
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_test(
        NAME ui_unit_test_coverage
        COMMAND ${CMAKE_COMMAND} --build . --target ui_unit_test_coverage
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
else()
    add_test(
        NAME ui_unit_tests
        COMMAND ${CMAKE_COMMAND} --build . --target ui_unit_tests
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
endif()
