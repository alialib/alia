#ifndef ALIA_RENDERERS_GL_SHADERS_H
#define ALIA_RENDERERS_GL_SHADERS_H

#include <alia/abi/prelude.h>

ALIA_EXTERN_C_BEGIN

// OpenGL shader program name, valid in the current context.
typedef unsigned int alia_gl_shader_program;

alia_gl_shader_program
alia_gl_create_shader_program(
    char const* vertex_shader_source, char const* fragment_shader_source);

ALIA_EXTERN_C_END

#endif /* ALIA_RENDERERS_GL_SHADERS_H */
