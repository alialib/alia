#ifndef ALIA_PLATFORMS_GLFW_VK_GL_EXT_WIN32_H
#define ALIA_PLATFORMS_GLFW_VK_GL_EXT_WIN32_H

#include <glad/glad.h>

#include <windows.h>

#ifndef GL_HANDLE_TYPE_OPAQUE_WIN32_EXT
#define GL_HANDLE_TYPE_OPAQUE_WIN32_EXT 0x9587
#endif
#ifndef GL_SEMAPHORE_TYPE_EXT
#define GL_SEMAPHORE_TYPE_EXT 0x9593
#endif
#ifndef GL_SEMAPHORE_TYPE_BINARY_EXT
#define GL_SEMAPHORE_TYPE_BINARY_EXT 0x9594
#endif

typedef void(APIENTRY* PFNGLCREATETEXTURESPROC)(GLenum, GLsizei, GLuint*);
typedef void(APIENTRY* PFNGLCREATEMEMORYOBJECTSEXTPROC)(GLsizei, GLuint*);
typedef void(APIENTRY* PFNGLDELETEMEMORYOBJECTSEXTPROC)(GLsizei, GLuint const*);
typedef void(APIENTRY* PFNGLIMPORTMEMORYWIN32HANDLEEXTPROC)(
    GLuint, GLuint64, GLenum, HANDLE);
typedef void(APIENTRY* PFNGLTEXTURESTORAGEMEM2DEXTPROC)(
    GLuint, GLsizei, GLenum, GLsizei, GLsizei, GLuint, GLuint64);
typedef void(APIENTRY* PFNGLCREATESEMAPHORESEXTPROC)(GLsizei, GLuint*);
typedef void(APIENTRY* PFNGLDELETESEMAPHORESEXTPROC)(GLsizei, GLuint const*);
typedef void(APIENTRY* PFNGLIMPORTSEMAPHOREWIN32HANDLEEXTPROC)(
    GLuint, GLuint64, GLenum, HANDLE);
typedef void(APIENTRY* PFNGLSEMAPHOREPARAMETERUI64VEXTPROC)(
    GLuint, GLenum, GLuint64 const*);
typedef void(APIENTRY* PFNGLSIGNALSEMAPHOREEXTPROC)(
    GLuint, GLsizei, GLuint const*, GLsizei, GLuint const*);

struct alia_vk_gl_ext_win32
{
    bool available = false;

    PFNGLCREATEMEMORYOBJECTSEXTPROC CreateMemoryObjectsEXT = nullptr;
    PFNGLDELETEMEMORYOBJECTSEXTPROC DeleteMemoryObjectsEXT = nullptr;
    PFNGLIMPORTMEMORYWIN32HANDLEEXTPROC ImportMemoryWin32HandleEXT = nullptr;
    PFNGLTEXTURESTORAGEMEM2DEXTPROC TextureStorageMem2DEXT = nullptr;
    PFNGLCREATESEMAPHORESEXTPROC CreateSemaphoresEXT = nullptr;
    PFNGLDELETESEMAPHORESEXTPROC DeleteSemaphoresEXT = nullptr;
    PFNGLIMPORTSEMAPHOREWIN32HANDLEEXTPROC ImportSemaphoreWin32HandleEXT
        = nullptr;
    PFNGLSEMAPHOREPARAMETERUI64VEXTPROC SemaphoreParameterui64vEXT = nullptr;
    PFNGLSIGNALSEMAPHOREEXTPROC SignalSemaphoreEXT = nullptr;
};

bool
alia_vk_gl_ext_win32_load(alia_vk_gl_ext_win32* ext);

bool
alia_vk_gl_ext_win32_load_memory(alia_vk_gl_ext_win32* ext);

bool
alia_vk_gl_ext_win32_load_semaphore(alia_vk_gl_ext_win32* ext);

#endif /* ALIA_PLATFORMS_GLFW_VK_GL_EXT_WIN32_H */
