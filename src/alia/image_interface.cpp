#include <alia/image_interface.hpp>

namespace alia {

unsigned get_channel_count(pixel_format fmt)
{
    switch (fmt)
    {
     case RGB:
        return 3;
     case RGBA:
        return 4;
     case GRAY:
     default:
        return 1;
    }
}

}
