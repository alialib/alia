add_library(alia_shell STATIC
    src/shell.cpp
    src/fonts.cpp)
target_include_directories(alia_shell PUBLIC
    include
    ${ALIA_SOURCE_DIR}/core/src)
target_link_libraries(alia_shell PUBLIC alia_core)
