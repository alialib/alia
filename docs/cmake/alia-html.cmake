include(FetchContent)

message(STATUS "Fetching alia-html")

FetchContent_Declare(alia-html
  GIT_REPOSITORY https://github.com/alialib/alia-html
  GIT_TAG 0.3.0
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(alia-html)

# Set various other Emscripten options...
string(APPEND CMAKE_CXX_FLAGS
    " -s EXTRA_EXPORTED_RUNTIME_METHODS='[\"UTF8ToString\"]'")
string(APPEND CMAKE_CXX_FLAGS
    " -s ENVIRONMENT=web")
string(APPEND CMAKE_CXX_FLAGS
    " -s WASM=1 --bind")

# Allow the use of Emscripten's Fetch API.
string(APPEND CMAKE_CXX_FLAGS " -s FETCH=1")
# Enable exceptions.
string(APPEND CMAKE_CXX_FLAGS " -s DISABLE_EXCEPTION_CATCHING=0")

# "Deploy" the given file to the given directory.
# In normal development, this creates a symlink of the file for more rapid
# iteration.
# In CI, it copies the file.
function(alia_deploy file_path destination_directory)
    if(DEFINED ENV{CI})
        file(COPY
            ${file_path}
            DESTINATION ${destination_directory})
    else()
        get_filename_component(file_name "${file_path}" NAME)
        file(CREATE_LINK
            ${file_path}
            ${destination_directory}/${file_name}
            SYMBOLIC)
    endif()
endfunction()

# "Deploy" a built file to the given directory.
# In normal development, this creates a symlink of the file for more rapid
# iteration.
# In CI, it sets up a post-build step to copy the file.
function(alia_post_build_deploy target file_path destination_directory)
    get_filename_component(file_name "${file_path}" NAME)
    if(DEFINED ENV{CI})
        add_custom_command(TARGET ${target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy ${file_path}
                ${destination_directory}/${file_name})
    else()
        file(CREATE_LINK ${file_path}
               ${destination_directory}/${file_name}
               SYMBOLIC)
    endif()
endfunction()
