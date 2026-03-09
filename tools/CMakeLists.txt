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
    CURL::libcurl
    yaml-cpp::yaml-cpp)
target_compile_features(alia_asset_builder PRIVATE cxx_std_17)
