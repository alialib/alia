# Font atlas generation helpers for Alia and consumer apps.
#
# Requires (set by Alia's top-level CMakeLists before including this file):
#   ALIA_SOURCE_DIR
#   ALIA_ENABLE_ASSET_PIPELINE
#
# Optional:
#   ALIA_HOST_ASSET_BUILDER - absolute path to a native alia_asset_builder
#     (required under Emscripten; ignored when the in-tree tool target exists)
#   ALIA_FONT_CACHE_DIR - download/cache directory for the asset builder

if(NOT DEFINED ALIA_SOURCE_DIR)
    message(FATAL_ERROR "cmake/fonts.cmake requires ALIA_SOURCE_DIR")
endif()

set(ALIA_FONT_MANIFEST_UI_ICONS
    "${ALIA_SOURCE_DIR}/shell/assets/ui_icons.yaml"
    CACHE FILEPATH "Stock UI icon font manifest (merged first into atlases)")
set(ALIA_FONT_MANIFEST_TYPOGRAPHY
    "${ALIA_SOURCE_DIR}/shell/assets/fonts.yaml"
    CACHE FILEPATH "Stock typography font manifest")

if(NOT ALIA_FONT_CACHE_DIR)
    set(ALIA_FONT_CACHE_DIR "${CMAKE_BINARY_DIR}/font_cache"
        CACHE PATH "Font download/cache directory for alia_asset_builder")
endif()

# Resolve the asset builder command and dependency for add_custom_command.
function(alia_resolve_asset_builder out_cmd out_dep)
    if(ALIA_ENABLE_ASSET_PIPELINE AND TARGET alia_asset_builder)
        set(${out_cmd} "$<TARGET_FILE:alia_asset_builder>" PARENT_SCOPE)
        set(${out_dep} alia_asset_builder PARENT_SCOPE)
        return()
    endif()

    if(EMSCRIPTEN)
        if(NOT ALIA_HOST_ASSET_BUILDER)
            set(_alia_host_build_dir "${ALIA_SOURCE_DIR}/build/Release")
            if(CMAKE_HOST_WIN32)
                set(ALIA_HOST_ASSET_BUILDER
                    "${_alia_host_build_dir}/tools/alia_asset_builder.exe"
                    CACHE FILEPATH
                    "Native alia_asset_builder used when cross-compiling")
            else()
                set(ALIA_HOST_ASSET_BUILDER
                    "${_alia_host_build_dir}/tools/alia_asset_builder"
                    CACHE FILEPATH
                    "Native alia_asset_builder used when cross-compiling")
            endif()
        endif()
        if(NOT EXISTS "${ALIA_HOST_ASSET_BUILDER}")
            message(FATAL_ERROR
                "Native alia_asset_builder not found at:\n"
                "  ${ALIA_HOST_ASSET_BUILDER}\n"
                "Build the tool on the host first "
                "(e.g. scripts/build.bat alia_asset_builder), then reconfigure "
                "with -DALIA_HOST_ASSET_BUILDER=<path> if needed.")
        endif()
        set(${out_cmd} "${ALIA_HOST_ASSET_BUILDER}" PARENT_SCOPE)
        set(${out_dep} "${ALIA_HOST_ASSET_BUILDER}" PARENT_SCOPE)
        return()
    endif()

    message(FATAL_ERROR
        "Font asset generation requires ALIA_ENABLE_ASSET_PIPELINE "
        "(native) or Emscripten with ALIA_HOST_ASSET_BUILDER.")
endfunction()

# Emit a custom command that writes alia_fonts.h / alia_fonts.cpp from the
# given YAML manifests (merged in order).
function(alia_generate_font_assets out_h out_cpp)
    set(_manifests ${ARGN})
    if(NOT _manifests)
        message(FATAL_ERROR "alia_generate_font_assets requires manifest paths")
    endif()

    alia_resolve_asset_builder(_builder_cmd _builder_dep)

    get_filename_component(_out_dir "${out_h}" DIRECTORY)
    file(MAKE_DIRECTORY "${_out_dir}")

    add_custom_command(
        OUTPUT "${out_h}" "${out_cpp}"
        COMMAND ${_builder_cmd}
            ${_manifests}
            "${out_h}" "${out_cpp}"
            --cache-dir "${ALIA_FONT_CACHE_DIR}"
        DEPENDS ${_builder_dep} ${_manifests}
        COMMENT "Generating font assets (${out_h})"
        VERBATIM)

    set_source_files_properties("${out_cpp}" PROPERTIES GENERATED TRUE)
endfunction()

# Create a STATIC library target that compiles a generated atlas.
#
#   alia_add_font_assets(<target>
#     MANIFESTS <yaml> [<yaml>...]
#     [GEN_DIR <dir>])   # default: ${CMAKE_CURRENT_BINARY_DIR}/gen/<target>
#
# Consumers typically pass stock manifests first, then app overrides:
#   alia_add_font_assets(my_fonts
#     MANIFESTS
#       "${ALIA_FONT_MANIFEST_UI_ICONS}"
#       "${ALIA_FONT_MANIFEST_TYPOGRAPHY}"
#       "${CMAKE_CURRENT_SOURCE_DIR}/fonts.yaml")
#
function(alia_add_font_assets target_name)
    cmake_parse_arguments(ARG "" "GEN_DIR" "MANIFESTS" ${ARGN})
    if(NOT ARG_MANIFESTS)
        message(FATAL_ERROR
            "alia_add_font_assets(${target_name}) requires MANIFESTS")
    endif()
    if(ARG_UNPARSED_ARGUMENTS)
        message(FATAL_ERROR
            "alia_add_font_assets(${target_name}): unexpected arguments: "
            "${ARG_UNPARSED_ARGUMENTS}")
    endif()

    if(NOT ARG_GEN_DIR)
        set(ARG_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/gen/${target_name}")
    endif()

    set(_out_h "${ARG_GEN_DIR}/alia_fonts.h")
    set(_out_cpp "${ARG_GEN_DIR}/alia_fonts.cpp")
    alia_generate_font_assets("${_out_h}" "${_out_cpp}" ${ARG_MANIFESTS})

    add_library(${target_name} STATIC "${_out_cpp}")
    target_include_directories(${target_name} PUBLIC
        "${ARG_GEN_DIR}"
        "${ALIA_SOURCE_DIR}/core/include")
    target_link_libraries(${target_name} PUBLIC alia_core)

    if(NOT TARGET ${target_name}_gen)
        add_custom_target(${target_name}_gen DEPENDS "${_out_h}" "${_out_cpp}")
    endif()
    add_dependencies(${target_name} ${target_name}_gen)
endfunction()
