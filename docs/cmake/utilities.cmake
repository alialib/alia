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
