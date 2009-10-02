#ifndef ALIA_OPENGL_GL_HPP
#define ALIA_OPENGL_GL_HPP

#include <GL/glew.h>

#ifdef _MSC_VER

#include <GL/wglew.h>

#pragma comment (lib, "glew32.lib")

#endif

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif
#ifdef DOUBLE_CLICK
#undef DOUBLE_CLICK
#endif
#ifdef KEY_EXECUTE
#undef KEY_EXECUTE
#endif

#endif
