#ifndef ALIA_NATIVE_ASCII_TEXT_RENDERER_HPP
#define ALIA_NATIVE_ASCII_TEXT_RENDERER_HPP

#ifdef WIN32
    #include <alia/native/win32/ascii_text_renderer.hpp>
    #define ALIA_HAS_NATIVE_TEXT
#else
    #include <alia/generic_text_renderer.hpp>
    namespace alia { namespace native {
        typedef alia::generic_text_renderer ascii_text_renderer;
    }}
#endif

#endif
