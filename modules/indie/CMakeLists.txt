# Set up the alia-indie library.
file(GLOB_RECURSE ALIA_INDIE_SOURCES ${ALIA_GLOB_OPTIONS} "src/*.cpp")
add_library(alia_indie STATIC ${ALIA_INDIE_SOURCES})
target_link_libraries(alia_indie
    PUBLIC alia_core)
target_include_directories(alia_indie PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/src")

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
    COMMAND ${CMAKE_COMMAND} -E chdir unit-testing ${CMAKE_COMMAND}
                             -E env ALIA_DEPLOY_DIR=${CMAKE_CURRENT_BINARY_DIR}
                             $<TARGET_FILE:indie_unit_test_runner>
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS indie_unit_test_runner)
