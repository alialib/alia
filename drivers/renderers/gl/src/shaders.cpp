#include <alia/renderers/gl/shaders.h>

#if defined(__EMSCRIPTEN__)
#include <GLES3/gl3.h>
#else
#include <glad/glad.h>
#endif

#include <cstdio>

namespace {

GLuint
compile_shader(GLenum type, char const* source)
{
#ifdef __EMSCRIPTEN__
    char const* header
        = "#version 300 es\nprecision highp float;\n#define EMSCRIPTEN 1\n";
#else
    char const* header = "#version 330 core\n";
#endif
    char const* sources[] = {header, source};
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 2, sources, nullptr);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char info_log[512];
        glGetShaderInfoLog(shader, 512, nullptr, info_log);
        std::fprintf(stderr, "Shader compilation error: %s\n", info_log);
    }
    return shader;
}

} // namespace

extern "C" {

alia_gl_shader_program
alia_gl_create_shader_program(
    char const* vertex_shader_source, char const* fragment_shader_source)
{
    GLuint vertex_shader
        = compile_shader(GL_VERTEX_SHADER, vertex_shader_source);
    GLuint fragment_shader
        = compile_shader(GL_FRAGMENT_SHADER, fragment_shader_source);
    GLuint shader_program = glCreateProgram();
    glAttachShader(shader_program, vertex_shader);
    glAttachShader(shader_program, fragment_shader);
    glLinkProgram(shader_program);
    glDeleteShader(vertex_shader);
    glDeleteShader(fragment_shader);
    return shader_program;
}

} // extern "C"
