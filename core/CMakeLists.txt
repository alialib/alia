# Add the core library.
include_directories(src)
if (ALIA_IS_MAIN_PROJECT)
    set(ALIA_GLOB_OPTIONS "CONFIGURE_DEPENDS")
else()
    set(ALIA_GLOB_OPTIONS "")
endif()
file(GLOB_RECURSE ALIA_CORE_SRC_FILES ${ALIA_GLOB_OPTIONS} "src/*.cpp")
add_library(alia_core STATIC ${ALIA_CORE_SRC_FILES})
target_include_directories(alia_core PUBLIC src)
target_compile_features(alia_core PUBLIC cxx_std_17)

if (NOT ALIA_ENABLE_TESTING)
    return()
endif()

find_package(Catch2 REQUIRED)

# Add the unit test runner.
file(GLOB_RECURSE UNIT_TEST_FILES CONFIGURE_DEPENDS "unit_tests/*.cpp")
add_executable(unit_test_runner ${UNIT_TEST_FILES})
target_link_libraries(unit_test_runner
    PRIVATE alia_core Catch2::Catch2WithMain)
target_include_directories(unit_test_runner
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/unit_tests)

# Add tests that are supposed to cause compilation errors.
# Specifically, find all .cpp files in the compilation_tests/ directory and
# generate test cases that try to compile them once with
# ALIA_TEST_COMPILATION_FAILURE #defined and once without it. The files are
# expected to compile successfully without the #define but generate a
# compilation error when the #define is provided.
file(GLOB_RECURSE COMPILATION_TEST_FILES "compilation_tests/*.cpp")
set(COMPILATION_TEST_SCRIPT
    "${CMAKE_CURRENT_BINARY_DIR}/invoke_compilation_tests.cmake")
file(WRITE ${COMPILATION_TEST_SCRIPT} "")
get_target_property(COMPILE_DEFS unit_test_runner COMPILE_DEFINITIONS)
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
    unit_tests
    # Create a fresh 'unit-testing' directory within the build dir and run the
    # tests with that.
    COMMAND ${CMAKE_COMMAND} -E remove_directory unit-testing
    COMMAND ${CMAKE_COMMAND} -E make_directory unit-testing
    COMMAND ${CMAKE_COMMAND} -E chdir unit-testing ${CMAKE_COMMAND}
                             -E env ALIA_DEPLOY_DIR=${CMAKE_CURRENT_BINARY_DIR}
                             $<TARGET_FILE:unit_test_runner>
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS unit_test_runner)

# On Clang debug builds, the proper CMake test associated with the unit tests
# includes test coverage reporting.
if(IS_CLANG AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    string(REPLACE clang++ llvm-profdata LLVM_PROFDATA ${CMAKE_CXX_COMPILER})
    string(REPLACE clang++ llvm-cov LLVM_COV ${CMAKE_CXX_COMPILER})
    add_custom_target(
        unit_test_coverage
        COMMAND ${CMAKE_COMMAND} --build . --target unit_tests
        COMMAND ${LLVM_PROFDATA} merge -sparse unit-testing/default.profraw
                -o default.profdata
        COMMAND ${LLVM_COV} show -instr-profile=default.profdata
                $<TARGET_FILE:unit_test_runner>
                >${CMAKE_CURRENT_SOURCE_DIR}/coverage.txt
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_test(
        NAME unit_test_coverage
        COMMAND ${CMAKE_COMMAND} --build . --target unit_test_coverage
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
else()
    add_test(
        NAME unit_tests
        COMMAND ${CMAKE_COMMAND} --build . --target unit_tests
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
endif()

# CMake doesn't seem to generate a test target in some cases (which I haven't
# quite figured out), so generate a custom one.
add_custom_target(
    ctest
    COMMAND ctest -C ${CMAKE_BUILD_TYPE}
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
