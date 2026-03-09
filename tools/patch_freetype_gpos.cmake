# Patch FreeType ftoption.h to enable TT_CONFIG_OPTION_GPOS_KERNING.
# Invoked by FetchContent PATCH_COMMAND with working directory = freetype source.
# Get CWD (FetchContent sets it to the content source).
if(CMAKE_HOST_WIN32)
  execute_process(COMMAND cmd /c cd OUTPUT_VARIABLE SRC_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
else()
  execute_process(COMMAND pwd OUTPUT_VARIABLE SRC_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()
set(FTOPTION_H "${SRC_DIR}/include/freetype/config/ftoption.h")
if(NOT EXISTS "${FTOPTION_H}")
  message(FATAL_ERROR "File not found: ${FTOPTION_H}")
endif()
file(READ "${FTOPTION_H}" CONTENT)
string(REPLACE "/* #define TT_CONFIG_OPTION_GPOS_KERNING */" "#define TT_CONFIG_OPTION_GPOS_KERNING" CONTENT "${CONTENT}")
file(WRITE "${FTOPTION_H}" "${CONTENT}")
message(STATUS "Patched ${FTOPTION_H}: enabled TT_CONFIG_OPTION_GPOS_KERNING")
