#ifndef ALIA_NATIVE_WIN32_ASCII_TEXT_RENDERER_HPP
#define ALIA_NATIVE_WIN32_ASCII_TEXT_RENDERER_HPP

#include <boost/scoped_array.hpp>
#include <alia/ascii_text_renderer.hpp>
#include <alia/font.hpp>

#pragma comment (lib, "gdi32.lib")

namespace alia { namespace native {

class ascii_text_renderer : public alia::ascii_text_renderer
{
 public:
    ascii_text_renderer(surface& surface, font const& font);
 private:
    image<uint8> image_;
    boost::scoped_array<int> widths_;
    boost::scoped_array<int> trailing_space_;
};

}}

#endif
