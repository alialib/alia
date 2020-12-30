cmake_minimum_required(VERSION 3.14)
project(alia)

include(FetchContent)

# Include Catch.
FetchContent_Declare(
  catch
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v2.13.0
)
FetchContent_MakeAvailable(catch)

# Detect the compiler.
if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    set(IS_CLANG true)
else()
    set(IS_CLANG false)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(IS_GCC true)
else()
    set(IS_GCC false)
endif()
if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(IS_MSVC true)
else()
    set(IS_MSVC false)
endif()

# Decide if we should be using C++14 or C++17.
if(IS_GCC)
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0)
        set(CMAKE_CXX_STANDARD 14)
    else()
        set(CMAKE_CXX_STANDARD 17)
    endif()
elseif(IS_CLANG)
    if (CMAKE_CXX_COMPILER_VERSION VERSION_LESS 5.0)
        set(CMAKE_CXX_STANDARD 14)
    else()
        set(CMAKE_CXX_STANDARD 17)
    endif()
else()
    set(CMAKE_CXX_STANDARD 14)
endif()

# Enable a high level of compiler warnings and treat them as errors.
if(IS_GCC)
    add_compile_options(-Wall -Werror)
    # TODO: Remove these.
    add_compile_options(-Wno-deprecated-declarations -Wno-deprecated-copy)
elseif(IS_MSVC)
    # First strip out the old warning level.
    string(REPLACE "/W3" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    add_compile_options(/W4 /WX)
    # Disable "unreferenced local function has been removed".
    # (As far as I can tell, this warning seems to be broken.)
    add_compile_options(/wd4505)
elseif(IS_CLANG)
    add_compile_options(-Wall -Werror)
endif()

# Set build options for instrumenting test coverage.
enable_testing()
if(IS_CLANG AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Enabling gcov support")
    add_compile_options(-DLLVM_USE_LINKER=gold -fprofile-instr-generate
                        -fcoverage-mapping)
    string(APPEND CMAKE_EXE_LINKER_FLAGS
           " -fprofile-instr-generate -fcoverage-mapping")
endif()

# Add the alia library.
include_directories(src)
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "src/*.cpp")
add_library(alia STATIC ${SRC_FILES})

# Add the unit test runner.
include_directories(src)
file(GLOB_RECURSE UNIT_TEST_FILES CONFIGURE_DEPENDS "unit_tests/*.cpp")
add_executable(unit_test_runner ${UNIT_TEST_FILES})
target_link_libraries(unit_test_runner alia Catch2::Catch2)
target_include_directories(unit_test_runner
    PRIVATE ${PROJECT_SOURCE_DIR}/unit_tests)

# Create another version of the unit tests that run against the single-header
# version of the library.
# (Note that this comes as an empty test and requires some external setup to
# run properly. This is normally only done within Travis.)
file(GLOB_RECURSE SINGLE_HEADER_TEST_FILES CONFIGURE_DEPENDS
     "single_header_tests/*.cpp")
add_executable(single_header_tester ${SINGLE_HEADER_TEST_FILES})
target_link_libraries(single_header_tester alia Catch2::Catch2)
target_include_directories(single_header_tester
    PRIVATE ${PROJECT_SOURCE_DIR}/single_header_tests)

# Add tests that are supposed to cause compilation errors.
# Specifically, find all .cpp files in the compilation_tests/ directory and
# generate test cases that try to compile them once with
# ALIA_TEST_COMPILATION_FAILURE #defined and once without it. The files are
# expected to compile successfully without the #define but generate a
# compilation error when the #define is provided.
file(GLOB_RECURSE COMPILATION_TEST_FILES "compilation_tests/*.cpp")
set(COMPILATION_TEST_SCRIPT
    "${PROJECT_BINARY_DIR}/invoke_compilation_tests.cmake")
file(WRITE ${COMPILATION_TEST_SCRIPT} "")
get_target_property(COMPILE_DEFS unit_test_runner COMPILE_DEFINITIONS)
foreach(TEST_FILE ${COMPILATION_TEST_FILES})
    get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)

    # We implement these tests by creating libraries that are built from the
    # source file in question. Then we create actual CMake test cases that try
    # to build those targets.

    # This is the "control" case (which omits the error and should build).
    add_library(${TEST_NAME}_control ${TEST_FILE})
    target_link_libraries(${TEST_NAME}_control alia)
    set_target_properties(${TEST_NAME}_control
                          PROPERTIES
                          EXCLUDE_FROM_ALL 1
                          EXCLUDE_FROM_DEFAULT_BUILD 1)
    add_test(NAME ${TEST_NAME}_control
        COMMAND ${CMAKE_COMMAND} --build . --target ${TEST_NAME}_control
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})

    # This is the actual failure case.
    add_library(${TEST_NAME} ${TEST_FILE})
    target_link_libraries(${TEST_NAME} alia)
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
                             -E env ALIA_DEPLOY_DIR=${PROJECT_BINARY_DIR}
                             $<TARGET_FILE:unit_test_runner>
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    DEPENDS unit_test_runner)

# Add a second target for running the unit tests against the single-header
# version of the library.
add_custom_target(
    single_header_tests
    # Create a fresh 'single-header-testing' directory within the build dir and
    # run the tests with that.
    COMMAND ${CMAKE_COMMAND} -E remove_directory single-header-testing
    COMMAND ${CMAKE_COMMAND} -E make_directory single-header-testing
    COMMAND ${CMAKE_COMMAND} -E chdir single-header-testing ${CMAKE_COMMAND}
                             -E env ALIA_DEPLOY_DIR=${PROJECT_BINARY_DIR}
                             $<TARGET_FILE:single_header_tester>
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    DEPENDS single_header_tester)

# On Clang debug builds, the proper CMake test associated with the unit tests
# includes test coverage reporting.
if(IS_CLANG AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    string(REPLACE clang llvm-profdata LLVM_PROFDATA $ENV{CC})
    string(REPLACE clang llvm-cov LLVM_COV $ENV{CC})
    add_custom_target(
        unit_test_coverage
        COMMAND ${CMAKE_COMMAND} --build . --target unit_tests
        COMMAND ${LLVM_PROFDATA} merge -sparse unit-testing/default.profraw
                -o default.profdata
        COMMAND ${LLVM_COV} show -instr-profile=default.profdata
                $<TARGET_FILE:unit_test_runner>
                >${PROJECT_SOURCE_DIR}/coverage.txt
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
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

# Add the single header tests as a test.
add_test(
    NAME single_header_tests
    COMMAND ${CMAKE_COMMAND} --build . --target single_header_tests
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR})

# CMake doesn't seem to generate a test target in some cases (which I haven't
# quite figured out), so generate a custom one.
add_custom_target(
    ctest
    COMMAND ctest -C ${CMAKE_BUILD_TYPE}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
