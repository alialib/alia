#ifndef ALIA_NATIVE_ARTIST_HPP
#define ALIA_NATIVE_ARTIST_HPP

#ifdef WIN32
    #include <alia/native/win32/artist.hpp>
    #define ALIA_HAS_NATIVE_ARTIST
#else
    #include <alia/generic_artist.hpp>
    namespace alia { namespace native {
        typedef alia::generic_artist artist;
    }}
#endif

#endif
