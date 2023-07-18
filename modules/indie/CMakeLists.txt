find_package(glfw3 3.3 CONFIG REQUIRED)
find_package(unofficial-skia CONFIG REQUIRED)

include(FetchContent)
FetchContent_Declare(dmilos_color
   GIT_REPOSITORY https://github.com/dmilos/color.git
   GIT_TAG 84dc0512cb5fcf6536d79f0bee2530e678c01b03)
FetchContent_Populate(dmilos_color)

# Set up the alia-indie library.
file(GLOB_RECURSE ALIA_INDIE_SOURCES ${ALIA_GLOB_OPTIONS} "src/*.cpp")
add_library(alia_indie STATIC ${ALIA_INDIE_SOURCES})
target_include_directories(alia_indie PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")
target_link_libraries(alia_indie PUBLIC
    alia_core
    glfw
    unofficial::skia::skia
    unofficial::skia::skparagraph
    unofficial::skia::skshaper
    unofficial::skia::skunicode)
target_include_directories(alia_indie PUBLIC "${dmilos_color_SOURCE_DIR}/src")
target_precompile_headers(alia_indie PUBLIC
    src/alia/indie.hpp)

# Add the test example.
add_executable(test_example examples/test.cpp examples/runner.cpp)
target_link_libraries(test_example PRIVATE alia_indie)

if (NOT ALIA_ENABLE_TESTING)
    return()
endif()

find_package(Catch2 REQUIRED)

# Add the unit test runner.
file(GLOB_RECURSE UNIT_TEST_FILES CONFIGURE_DEPENDS "unit_tests/*.cpp")
add_executable(indie_unit_test_runner ${UNIT_TEST_FILES})
target_link_libraries(indie_unit_test_runner
    PRIVATE alia_indie Catch2::Catch2WithMain)
target_include_directories(indie_unit_test_runner
    PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/unit_tests)

# Add a target for running the unit tests.
add_custom_target(
    indie_unit_tests
    # Create a fresh 'unit-testing' directory within the build dir and run the
    # tests with that.
    COMMAND ${CMAKE_COMMAND} -E remove_directory unit-testing
    COMMAND ${CMAKE_COMMAND} -E make_directory unit-testing
    COMMAND ${CMAKE_COMMAND} -E chdir unit-testing
                             $<TARGET_FILE:indie_unit_test_runner>
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS indie_unit_test_runner)

# On Clang debug builds, the proper CMake test associated with the unit tests
# includes test coverage reporting.
if(IS_CLANG AND CMAKE_BUILD_TYPE STREQUAL "Debug")
    string(REPLACE clang++ llvm-profdata LLVM_PROFDATA ${CMAKE_CXX_COMPILER})
    string(REPLACE clang++ llvm-cov LLVM_COV ${CMAKE_CXX_COMPILER})
    add_custom_target(
        indie_unit_test_coverage
        COMMAND ${CMAKE_COMMAND} --build . --target indie_unit_tests
        COMMAND ${LLVM_PROFDATA} merge -sparse unit-testing/default.profraw
                -o default.profdata
        COMMAND ${LLVM_COV} show -instr-profile=default.profdata
                $<TARGET_FILE:indie_unit_test_runner>
                >${CMAKE_CURRENT_SOURCE_DIR}/coverage.txt
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    add_test(
        NAME indie_unit_test_coverage
        COMMAND ${CMAKE_COMMAND} --build . --target indie_unit_test_coverage
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
else()
    add_test(
        NAME indie_unit_tests
        COMMAND ${CMAKE_COMMAND} --build . --target indie_unit_tests
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR})
endif()
