# Detect the compiler and set IS_CLANG, IS_GCC, and IS_MSVC accordingly.
macro(alia_detect_compiler)
    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        set(IS_CLANG true)
    else()
        set(IS_CLANG false)
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(IS_GCC true)
    else()
        set(IS_GCC false)
    endif()
    if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        set(IS_MSVC true)
    else()
        set(IS_MSVC false)
    endif()
endmacro()

# Detect if Emscripten is active.
# Note that the Emscripten toolchain file defines EMSCRIPTEN as a CMake
# variable, but this allows us to detect it before the toolchain is active.
macro(alia_detect_emscripten)
    if(DEFINED ENV{EMSDK})
        set(IS_EMSCRIPTEN true)
    else()
        set(IS_EMSCRIPTEN false)
    endif()
endmacro()

# Set the (global) warning flags to what we want for alia development.
# Generally, this enables a high level of warnings and treats them as errors,
# but it also disables warnings that are broken or otherwise not useful.
function(alia_set_development_warning_flags)
    if(IS_GCC)
        add_compile_options(-Wall -Werror)
        # TODO: Remove these.
        add_compile_options(-Wno-deprecated-declarations -Wno-deprecated-copy)
    elseif(IS_MSVC)
        # First strip out the old warning level.
        string(REPLACE "/W3" "" SANITIZED_CXX_FLAGS ${CMAKE_CXX_FLAGS})
        add_compile_options(/W4 /WX)
        # Disable "unreferenced local function has been removed".
        # (As far as I can tell, this warning seems to be broken.)
        add_compile_options(/wd4505)
        # Disable "unreachable code".
        # (Again, as far as I can tell, this seems to be broken.)
        add_compile_options(/wd4702)
    elseif(IS_CLANG)
        add_compile_options(-Wall -Werror)
    endif()
endfunction()
