#ifndef ALIA_OPENGL_UTILS_HPP
#define ALIA_OPENGL_UTILS_HPP

#include <alia/exception.hpp>
#include <alia/image_interface.hpp>
#include <alia/opengl/gl.hpp>
#include <alia/typedefs.hpp>

namespace alia { namespace opengl {

// Check if any OpenGL errors have occurred, and if so, throw an
// opengl_error exception.
void check_errors();

bool is_power_of_two(unsigned n);

GLenum get_gl_format(pixel_format fmt, unsigned flags);

GLenum get_internal_format(unsigned flags, GLenum image_format);

void copy_subimage(uint8* dst, uint8 const* src, int width, int height,
    int dst_stride, int src_stride, int src_step, int pixel_size);

}}

#endif
