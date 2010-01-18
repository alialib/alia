#include <alia/opengl/utils.hpp>
#include <alia/surface.hpp>

namespace alia { namespace opengl {

void check_errors()
{
    //// We only report one error for the life of the program.  Otherwise, once
    //// one error occurs, you generally end up with an infinite loop of error
    //// boxes.
    //static bool already_reported_error = false;
    //if (already_reported_error)
    //    return;

    //// Check for an error.
    //GLenum err = glGetError();

    //// If there's no error, abort.
    //if (err == GL_NO_ERROR)
    //    return;

    //// Decode the error.
    //const char *s;
    //switch (err)
    //{
    // case GL_INVALID_ENUM:
    //    s = "GL_INVALID_ENUM";
    //    break;
    // case GL_INVALID_VALUE:
    //    s = "GL_INVALID_VALUE";
    //    break;
    // case GL_INVALID_OPERATION:
    //    s = "GL_INVALID_OPERATION";
    //    break;
    // case GL_STACK_OVERFLOW:
    //    s = "GL_STACK_OVERFLOW";
    //    break;
    // case GL_STACK_UNDERFLOW:
    //    s = "GL_STACK_UNDERFLOW";
    //    break;
    // case GL_OUT_OF_MEMORY:
    //    s = "GL_OUT_OF_MEMORY";
    //    break;
    // default:
    //    s = "GL_UNKNOWN_ERROR";
    //}

    //already_reported_error = true;
    //throw exception(
    //    std::string("An OpenGL error has occurred:\n\n") +
    //    s + "\n\n"
    //    "You can continue using the program, but you may notice display"
    //    " problems.");
}

bool is_power_of_two(unsigned n)
{
    return (n & (n - 1)) == 0 && n != 0;
}

unsigned get_next_power_of_two(unsigned n)
{
    unsigned i = 1;
    while (i < n)
        i <<= 1;
    return i;
}

GLenum get_gl_format(pixel_format fmt, unsigned flags)
{
    switch (fmt)
    {
     case RGB:
        return GL_RGB;
     case RGBA:
        return GL_RGBA;
     case GRAY:
     default:
        return (flags & surface::ALPHA_IMAGE) ? GL_ALPHA : GL_LUMINANCE;
    }
}

GLenum get_internal_format(unsigned flags, GLenum image_format)
{
    switch (flags & 0xf)
    {
     case surface::RGB_IMAGE:
        return GL_RGB;
     case surface::RGBA_IMAGE:
        return GL_RGBA;
     case surface::ALPHA_IMAGE:
        return GL_ALPHA;
     case surface::GRAY_IMAGE:
        return GL_LUMINANCE;
     default:
        return image_format;
    }
}

void copy_subimage(uint8* dst, uint8 const* src, int width, int height,
    int dst_stride, int src_stride, int src_step, int pixel_size)
{
    if (src_step != pixel_size)
    {
        for (int i = 0; i < height; ++i)
        {
            uint8 const* s = src;
            uint8* d_end = dst + width * pixel_size;
            for (uint8* d = dst; d != d_end; d += pixel_size, s += src_step)
            {
                for (int j = 0; j < pixel_size; ++j)
                    *(d + j) = *(s + j);
            }
            src += src_stride;
            dst += dst_stride;
        }
    }
    else
    {
        for (int i = 0; i < height; ++i)
        {
            memcpy(dst, src, width);
            src += src_stride;
            dst += dst_stride;
        }
    }
}

}}
