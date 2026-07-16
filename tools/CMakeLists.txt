# Fetch a pinned Slang SDK (provides slangc) into the build tree.
function(alia_fetch_slang_sdk)
    set(ALIA_SLANG_VERSION "2026.13.1" CACHE STRING "Slang SDK version")
    set(ALIA_SLANG_ROOT "${CMAKE_BINARY_DIR}/slang-sdk" CACHE PATH
        "Slang SDK extract directory")

    if(WIN32)
        set(_slang_archive
            "slang-${ALIA_SLANG_VERSION}-windows-x86_64.zip")
    elseif(APPLE)
        set(_slang_archive
            "slang-${ALIA_SLANG_VERSION}-macos-x86_64.zip")
    else()
        set(_slang_archive
            "slang-${ALIA_SLANG_VERSION}-linux-x86_64.zip")
    endif()

    set(_slang_url
        "https://github.com/shader-slang/slang/releases/download/v${ALIA_SLANG_VERSION}/${_slang_archive}")
    set(_slangc "${ALIA_SLANG_ROOT}/bin/slangc${CMAKE_EXECUTABLE_SUFFIX}")

    if(NOT EXISTS "${_slangc}")
        message(STATUS "Downloading Slang SDK ${ALIA_SLANG_VERSION}...")
        file(MAKE_DIRECTORY "${ALIA_SLANG_ROOT}")
        set(_zip "${ALIA_SLANG_ROOT}/${_slang_archive}")
        file(DOWNLOAD "${_slang_url}" "${_zip}"
            SHOW_PROGRESS
            STATUS _dl_status)
        list(GET _dl_status 0 _dl_code)
        if(NOT _dl_code EQUAL 0)
            list(GET _dl_status 1 _dl_msg)
            message(FATAL_ERROR "Failed to download Slang SDK: ${_dl_msg}")
        endif()
        file(ARCHIVE_EXTRACT INPUT "${_zip}" DESTINATION "${ALIA_SLANG_ROOT}")
    endif()

    if(NOT EXISTS "${_slangc}")
        message(FATAL_ERROR "slangc not found at ${_slangc}")
    endif()
    set(ALIA_SLANGC "${_slangc}" PARENT_SCOPE)
endfunction()

alia_fetch_slang_sdk()

# Asset builder uses FreeType (FetchContent, GPOS kerning enabled) and msdf-atlas-gen (FetchContent).
set(MSDF_ATLAS_BUILD_STANDALONE OFF CACHE BOOL "Do not build msdf-atlas-gen standalone" FORCE)
set(MSDF_ATLAS_NO_ARTERY_FONT ON CACHE BOOL "Disable Artery Font" FORCE)
set(MSDF_ATLAS_USE_SKIA OFF CACHE BOOL "Do not use Skia" FORCE)
set(MSDF_ATLAS_USE_VCPKG OFF CACHE BOOL "Do not use vcpkg for msdf-atlas-gen deps" FORCE)
set(MSDFGEN_DISABLE_PNG ON CACHE BOOL "Disable PNG (asset builder only needs TTF/MSDF)" FORCE)
# Match the rest of the project / vcpkg (dynamic CRT /MD) to avoid LNK2038 runtime library mismatch.
set(MSDF_ATLAS_DYNAMIC_RUNTIME ON CACHE BOOL "Use dynamic runtime" FORCE)

include(FetchContent)

# FreeType with GPOS kerning enabled (so FT_Get_Kerning uses GPOS 'kern' feature for modern fonts).
FetchContent_Declare(freetype
    GIT_REPOSITORY https://git.savannah.gnu.org/git/freetype/freetype2.git
    GIT_TAG VER-2-14-2
    PATCH_COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_SOURCE_DIR}/patch_freetype_gpos.cmake
)
FetchContent_MakeAvailable(freetype)

# Create Freetype::Freetype alias so msdfgen skips find_package (it checks "if(NOT TARGET Freetype::Freetype)").
add_library(Freetype::Freetype ALIAS freetype)

FetchContent_Declare(msdf-atlas-gen
    GIT_REPOSITORY https://github.com/Chlumsky/msdf-atlas-gen.git
    GIT_TAG v1.4
    GIT_SUBMODULES msdfgen
)
FetchContent_MakeAvailable(msdf-atlas-gen)

find_package(CURL REQUIRED)
find_package(yaml-cpp REQUIRED)

add_executable(alia_asset_builder asset_builder.cpp)
target_link_libraries(alia_asset_builder PRIVATE
    msdfgen::msdfgen
    msdf-atlas-gen::msdf-atlas-gen
    Freetype::Freetype
    CURL::libcurl
    yaml-cpp::yaml-cpp)
target_compile_features(alia_asset_builder PRIVATE cxx_std_17)

add_executable(alia_shader_builder shader_builder.cpp)
target_compile_features(alia_shader_builder PRIVATE cxx_std_17)

# Expose slangc path to the parent project.
set(ALIA_SLANGC "${ALIA_SLANGC}" PARENT_SCOPE)
