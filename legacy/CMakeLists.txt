cmake_dependent_option(
    ALIA_BUILD_DOCS
    "Build the docs"
    ${ALIA_IS_MAIN_PROJECT}
    "IS_EMSCRIPTEN"
    OFF)

cmake_dependent_option(
    ALIA_ENABLE_HTML
    "Enable integration with HTML5"
    ${ALIA_IS_MAIN_PROJECT}
    "IS_EMSCRIPTEN"
    OFF)
if(ALIA_ENABLE_HTML)
  list(APPEND VCPKG_MANIFEST_FEATURES "html")
endif()

# If alia is the main project, set some global development flags.
if (ALIA_IS_MAIN_PROJECT)
    alia_set_development_warning_flags()

    if (IS_MSVC)
        # Use EditAndContinue-style debug info for MSVC.
        # TODO: Use CMAKE_MSVC_DEBUG_INFORMATION_FORMAT once it's more widely
        # supported:
        # set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT
        #     "$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>")
        # string(REGEX REPLACE "/Zi" "/ZI"
        #     CMAKE_CXX_FLAGS_RELWITHDEBINFO
        #     "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
        string(REGEX REPLACE "/Zi" "/ZI"
            CMAKE_CXX_FLAGS_DEBUG
            "${CMAKE_CXX_FLAGS_DEBUG}")
        # Enable profiling.
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /PROFILE")
        # Use full optimizations for profiling.
        string(REGEX REPLACE "/Ob1" "/Ob2"
            CMAKE_CXX_FLAGS_RELWITHDEBINFO
            "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")
        # Use absolute paths for error messages.
        add_compile_options(/FC)
    endif()
endif()

# If we're testing, set the build options for instrumenting test coverage.
if(ALIA_ENABLE_TESTING)
    enable_testing()
    if(IS_CLANG AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        message(STATUS "Enabling gcov support")
        add_compile_options(-DLLVM_USE_LINKER=gold -fprofile-instr-generate
                            -fcoverage-mapping)
        string(APPEND CMAKE_EXE_LINKER_FLAGS
            " -fprofile-instr-generate -fcoverage-mapping")
    endif()
endif()

# Add the core library.
include_directories(src)
if (ALIA_IS_MAIN_PROJECT)
    set(ALIA_GLOB_OPTIONS "CONFIGURE_DEPENDS")
else()
    set(ALIA_GLOB_OPTIONS "")
endif()
file(GLOB_RECURSE ALIA_CORE_SRC_FILES ${ALIA_GLOB_OPTIONS}
    "src/alia/core/*.cpp")
add_library(alia_core STATIC ${ALIA_CORE_SRC_FILES})
target_include_directories(alia_core PUBLIC src)
target_compile_features(alia_core PUBLIC cxx_std_23)

if (IS_EMSCRIPTEN)
    target_link_options(alia_ui PUBLIC
        "-sFULL_ES3"
        "-sTOTAL_MEMORY=64MB")

    # TODO...
    # target_compile_options(alia_ui PUBLIC "-fwasm-exceptions")
    # target_link_options(alia_ui PUBLIC "-fwasm-exceptions")
    # Restore pre-3.1.27 stack size for Emscripten.
    target_link_options(alia_ui PUBLIC "-sSTACK_SIZE=5MB")

    # target_compile_options(alia_core PUBLIC "-fwasm-exceptions")
    # target_link_options(alia_core PUBLIC "-fwasm-exceptions")
    target_link_options(alia_core PUBLIC "-sSTACK_SIZE=5MB")

    # Set various other Emscripten options...
    target_link_options(alia_ui PUBLIC
        "-sENVIRONMENT=web")
    target_link_options(alia_ui PUBLIC
        "-sWASM=1")
    target_link_options(alia_ui PUBLIC
        "--bind")
    target_link_options(alia_ui PUBLIC
        "-sMIN_WEBGL_VERSION=2")
    target_link_options(alia_ui PUBLIC
        "-sMAX_WEBGL_VERSION=2")

    target_link_options(alia_ui PUBLIC
        "-Oz"
        "-sELIMINATE_DUPLICATE_FUNCTIONS=1")

    add_library(alia_emscripten_ui STATIC "src/alia/ui/backends/emscripten.cpp")
    target_include_directories(alia_emscripten_ui PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/src")
    target_link_libraries(alia_emscripten_ui PUBLIC alia_ui)
    target_link_options(alia_emscripten_ui PUBLIC "-sERROR_ON_UNDEFINED_SYMBOLS=0")

    set(CMAKE_EXECUTABLE_SUFFIX ".html")
endif()

# Add alia/HTML.
if (ALIA_ENABLE_HTML)
    add_subdirectory("integrations/html")
endif()

# Add the docs.
if (ALIA_BUILD_DOCS)
    add_subdirectory(docs)
endif()
