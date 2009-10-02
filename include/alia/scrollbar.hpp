#ifndef ALIA_SCROLLBAR_HPP
#define ALIA_SCROLLBAR_HPP

#include <alia/forward.hpp>
#include <alia/artist.hpp>
#include <alia/input_utils.hpp>
#include <alia/region.hpp>

namespace alia {

struct scrollbar_data
{
    scrollbar_data() : axis(-1), physical_position(0) {}
    int axis, physical_position, drag_start_delta;
    //alia::timer timer;
    artist_data_ptr background_data[2];
    region_data background_id_data[2];
    artist_data_ptr thumb_data;
    region_data thumb_id_data;
    artist_data_ptr button_data[2];
    region_data button_id_data[2];
};

void refresh_scrollbar_data(context& ctx, scrollbar_data& data);

int do_scrollbar(context& ctx, scrollbar_data& data,
    box2i const& area, int axis, int content_size, int window_size,
    int position, int line_increment, int page_increment = -1);

}

#endif
