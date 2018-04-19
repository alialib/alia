cmake_minimum_required(VERSION 2.8.12)

set(CMAKE_CXX_STANDARD 14)

enable_testing()

# Set up fips.
get_filename_component(FIPS_ROOT_DIR "../fips" ABSOLUTE)
include("${FIPS_ROOT_DIR}/cmake/fips.cmake")
set(FIPS_EXCEPTIONS ON)
set(FIPS_RTTI ON)
fips_setup()
fips_project(alia)

# Run Conan to install external C++ libraries.
# Conan and fips disagree on various build options, so we need to override
# some of Conan's defaults.
set(CONAN_OPTIONS)
if (FIPS_MSVC)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CONAN_OPTIONS
            -s compiler=Visual\ Studio -s build_type=Debug -s compiler.runtime=MTd)
    else()
        set(CONAN_OPTIONS
            -s compiler=Visual\ Studio -s build_type=Release -s compiler.runtime=MT)
    endif()
elseif(FIPS_GCC)
    if (CMAKE_BUILD_TYPE STREQUAL "Debug")
        set(CONAN_OPTIONS
            -s compiler=gcc -s compiler.libcxx=libstdc++11 -s build_type=Debug -o Boost:fPIC=True)
    else()
        set(CONAN_OPTIONS
            -s compiler=gcc -s compiler.libcxx=libstdc++11 -s build_type=Release -o Boost:fPIC=True)
    endif()
endif()
execute_process(
    COMMAND conan install ${PROJECT_SOURCE_DIR} ${CONAN_OPTIONS} -e CONAN_IMPORT_PATH=${FIPS_PROJECT_DEPLOY_DIR} --build missing
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    RESULT_VARIABLE CONAN_RESULT)
if(NOT ${CONAN_RESULT} EQUAL 0)
    message(FATAL_ERROR "Conan failed.")
endif()

# And now set up CMake to use those libraries.
# Note that Conan seems to insert flags that don't make sense and cause warnings.
set(ORIGINAL_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
include(${PROJECT_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup()
set(CMAKE_CXX_FLAGS "${ORIGINAL_CXX_FLAGS}")

# Register Conan's include directories with fips.
fips_include_directories(${CONAN_INCLUDE_DIRS}
    "$<$<CONFIG:Release>:${CONAN_INCLUDE_DIRS_RELEASE}>"
    "$<$<CONFIG:Debug>:${CONAN_INCLUDE_DIRS_DEBUG}>")
# And remember the libs that it wants to link against.
set(EXTERNAL_LIBS ${CONAN_LIBS})

# If we're running on AppVeyor, we omit Boost from Conan and just use the one
# that AppVeyor supplies. (This saves a lot of time and frustration.)
if(DEFINED ENV{APPVEYOR})
    find_package(Boost REQUIRED COMPONENTS program_options regex)
    fips_include_directories(${Boost_INCLUDE_DIRS})
    list(APPEND EXTERNAL_LIBS ${Boost_LIBRARIES})
endif()

fips_include_directories(${PROJECT_SOURCE_DIR}/src)

# MACRO - Add the given linker options on anything that gets linked.
macro(add_link_options )
    string(REPLACE ";" " " OPTIONS "${ARGV}")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${OPTIONS}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OPTIONS}")
    set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} ${OPTIONS}")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${OPTIONS}")
endmacro()

# MACRO - Add the given linker options for executables.
macro(add_exe_link_options )
    string(REPLACE ";" " " OPTIONS "${ARGV}")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OPTIONS}")
endmacro()

# Enable a high level of compiler warnings and treat them as errors.
if(FIPS_GCC)
    add_compile_options(-Wall -Werror)
    # Disable warnings that are too strict.
    # Warnings about suggested parentheses occur naturally when using Catch.
    add_compile_options(-Wno-parentheses)
    # unused function parameters
    add_compile_options(-Wno-unused-function)
elseif(FIPS_MSVC)
    # First strip out the old warning level.
    string(REPLACE "/W3" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
    add_compile_options(/W4 /WX)
    # Disable warnings that are too strict.
    # "unreferenced formal parameter"
    add_compile_options(/wd4100)
    # "declaration hides previous local declaration"
    add_compile_options(/wd4456)
    # "unreferenced local function has been removed"
    add_compile_options(/wd4505)
    # warnings about functions that are potentially insecure
    add_definitions(/D_CRT_SECURE_NO_WARNINGS)
    # Also suppress linker warnings about missing .pdb files that seem to inevitably creep in.
    add_link_options(/ignore:4099)
endif()

# Enable "big objects" for Visual C++ and try to speed up builds.
if(FIPS_MSVC)
    add_compile_options(/bigobj)
    add_exe_link_options(/Debug:FASTLINK)
endif()

# Set build options for instrumenting test coverage.
if(FIPS_GCC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Enabling gcov support")
    add_compile_options(--coverage -fno-inline -fno-inline-small-functions -fno-default-inline -fprofile-arcs -ftest-coverage)
    add_exe_link_options(--coverage)
endif()

# Add the alia library.
fips_begin_module(alia)
    fips_src(src/alia)
    fips_dir(.)
fips_end_module()
target_link_libraries(alia ${EXTERNAL_LIBS})

# Add the unit test runner.
fips_begin_app(unit_test_runner cmdline)
    fips_deps(alia)
    fips_src(unit_tests)
fips_end_app()

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
    fips_begin_lib(${TEST_NAME}_control)
        fips_deps(alia)
        fips_files(${TEST_FILE})
    fips_end_lib()
    set_target_properties(${TEST_NAME}_control PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
    add_test(NAME ${TEST_NAME}_control
        COMMAND ${CMAKE_COMMAND} --build . --target ${TEST_NAME}_control
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})

    # This is the actual failure case.
    fips_begin_lib(${TEST_NAME})
        fips_deps(alia)
        fips_files(${TEST_FILE})
    fips_end_lib()
    target_compile_definitions(
        ${TEST_NAME} PRIVATE ALIA_TEST_COMPILATION_FAILURE)
    set_target_properties(${TEST_NAME} PROPERTIES EXCLUDE_FROM_ALL 1 EXCLUDE_FROM_DEFAULT_BUILD 1)
    add_test(NAME ${TEST_NAME}
        COMMAND ${CMAKE_COMMAND} --build . --target ${TEST_NAME}
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
    set_tests_properties(${TEST_NAME} PROPERTIES WILL_FAIL TRUE)
endforeach()

# Add a target for running the unit tests.
add_custom_target(
    unit_tests
    # Create a fresh 'unit-testing' directory within the build dir and run the
    # tests with that. (Some of them perform file I/O.)
    COMMAND ${CMAKE_COMMAND} -E remove_directory unit-testing
    COMMAND ${CMAKE_COMMAND} -E make_directory unit-testing
    COMMAND ${CMAKE_COMMAND} -E chdir unit-testing ${CMAKE_COMMAND} -E env ALIA_DEPLOY_DIR=${FIPS_PROJECT_DEPLOY_DIR} ${FIPS_PROJECT_DEPLOY_DIR}/unit_test_runner
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    DEPENDS unit_test_runner)

# On Linux debug builds, the proper CMake test associated with the unit tests
# includes test coverage reporting.
if(FIPS_GCC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_custom_target(
        unit_test_coverage
        COMMAND lcov --directory . --zerocounters
        COMMAND ${CMAKE_COMMAND} --build . --target unit_tests
        COMMAND lcov --directory . --capture --output-file raw.info
        COMMAND lcov --extract raw.info '${PROJECT_SOURCE_DIR}/src/alia/*' --output-file filtered.info
        COMMAND ${CMAKE_COMMAND} -E copy filtered.info ${PROJECT_SOURCE_DIR}/.lcov.info
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

# CMake doesn't seem to generate a test target for MSVC.
if(FIPS_MSVC)
    add_custom_target(
        ctest
        COMMAND ctest -C ${CMAKE_BUILD_TYPE}
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
endif()
