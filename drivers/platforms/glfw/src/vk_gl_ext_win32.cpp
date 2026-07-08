#include "vk_gl_ext_win32.h"

#include <GLFW/glfw3.h>

#include <cstdio>
#include <cstring>

namespace {

template<typename T>
T
load_gl(char const* name)
{
    return reinterpret_cast<T>(glfwGetProcAddress(name));
}

bool
has_extension(char const* name)
{
    GLint count = 0;
    glGetIntegerv(GL_NUM_EXTENSIONS, &count);
    for (GLint i = 0; i < count; ++i)
    {
        char const* ext = reinterpret_cast<char const*>(glGetStringi(GL_EXTENSIONS, i));
        if (ext && strcmp(ext, name) == 0)
            return true;
    }
    return false;
}

bool
has_memory_extensions()
{
    return has_extension("GL_EXT_memory_object")
        && has_extension("GL_EXT_memory_object_win32");
}

bool
has_semaphore_extensions()
{
    return has_extension("GL_EXT_semaphore")
        && has_extension("GL_EXT_semaphore_win32");
}

} // namespace

bool
alia_vk_gl_ext_win32_load_memory(alia_vk_gl_ext_win32* ext)
{
    if (!ext)
        return false;

    if (!has_memory_extensions())
    {
        std::fprintf(
            stderr,
            "[alia vk_present] GL memory interop extensions unavailable\n");
        return false;
    }

    ext->CreateMemoryObjectsEXT
        = load_gl<PFNGLCREATEMEMORYOBJECTSEXTPROC>("glCreateMemoryObjectsEXT");
    ext->DeleteMemoryObjectsEXT
        = load_gl<PFNGLDELETEMEMORYOBJECTSEXTPROC>("glDeleteMemoryObjectsEXT");
    ext->ImportMemoryWin32HandleEXT = load_gl<PFNGLIMPORTMEMORYWIN32HANDLEEXTPROC>(
        "glImportMemoryWin32HandleEXT");
    ext->TextureStorageMem2DEXT
        = load_gl<PFNGLTEXTURESTORAGEMEM2DEXTPROC>("glTextureStorageMem2DEXT");

    if (!ext->CreateMemoryObjectsEXT || !ext->DeleteMemoryObjectsEXT
        || !ext->ImportMemoryWin32HandleEXT || !ext->TextureStorageMem2DEXT)
    {
        std::fprintf(
            stderr, "[alia vk_present] failed to load GL memory interop entry points\n");
        return false;
    }

    return true;
}

bool
alia_vk_gl_ext_win32_load_semaphore(alia_vk_gl_ext_win32* ext)
{
    if (!ext)
        return false;

    if (!has_semaphore_extensions())
        return false;

    ext->CreateSemaphoresEXT
        = load_gl<PFNGLCREATESEMAPHORESEXTPROC>("glCreateSemaphoresEXT");
    ext->DeleteSemaphoresEXT
        = load_gl<PFNGLDELETESEMAPHORESEXTPROC>("glDeleteSemaphoresEXT");
    ext->ImportSemaphoreWin32HandleEXT
        = load_gl<PFNGLIMPORTSEMAPHOREWIN32HANDLEEXTPROC>(
            "glImportSemaphoreWin32HandleEXT");
    ext->SemaphoreParameterui64vEXT
        = load_gl<PFNGLSEMAPHOREPARAMETERUI64VEXTPROC>(
            "glSemaphoreParameterui64vEXT");
    ext->SignalSemaphoreEXT
        = load_gl<PFNGLSIGNALSEMAPHOREEXTPROC>("glSignalSemaphoreEXT");

    return ext->CreateSemaphoresEXT && ext->DeleteSemaphoresEXT
        && ext->ImportSemaphoreWin32HandleEXT && ext->SemaphoreParameterui64vEXT
        && ext->SignalSemaphoreEXT;
}

bool
alia_vk_gl_ext_win32_load(alia_vk_gl_ext_win32* ext)
{
    if (!ext)
        return false;

    *ext = {};
    if (!alia_vk_gl_ext_win32_load_memory(ext))
        return false;
    if (!alia_vk_gl_ext_win32_load_semaphore(ext))
        return false;

    ext->available = true;
    return true;
}
