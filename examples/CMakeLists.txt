# Add the test example.
if (ALIA_ENABLE_UI)
    add_executable(test_example test.cpp runner.cpp)
    target_link_libraries(test_example PUBLIC alia_glfw alia_ui unofficial::skia::skia)
endif()

# Downloads this wrapper library and tree-sitter.
# Makes them available via the `cpp-tree-sitter` CMake library target.
CPMAddPackage(
  NAME cpp-tree-sitter
  GIT_REPOSITORY https://github.com/nsumner/cpp-tree-sitter.git
  GIT_TAG v0.0.1
)

# Downloads a tree-sitter grammar from github and makes it available as a
# cmake library target.
add_grammar_from_repo(tree-sitter-cpp
  https://github.com/tree-sitter/tree-sitter-cpp.git
  0.23.4
)

# Add the factors example.
if (ALIA_ENABLE_UI)
    add_executable(factors factors.cpp runner.cpp)
    target_link_libraries(factors PUBLIC
        alia_glfw alia_ui
        unofficial::skia::skia
        tree-sitter-cpp
        cpp-tree-sitter)
endif()
