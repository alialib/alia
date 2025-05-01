if (ALIA_ENABLE_UI)
    find_package(glad CONFIG REQUIRED)

    add_executable(test_example test.cpp runner.cpp assets.cpp) # Add WIN32?
    target_link_libraries(test_example PUBLIC
        alia_glfw alia_ui unofficial::skia::skia shcore glad::glad)

    add_executable(shader_gallery shader_gallery.cpp runner.cpp assets.cpp) # Add WIN32?
    target_link_libraries(shader_gallery PUBLIC
        alia_glfw alia_ui unofficial::skia::skia shcore glad::glad)
endif()
