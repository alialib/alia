#ifndef ALIA_FORWARD_HPP
#define ALIA_FORWARD_HPP

namespace alia {

struct context;

class canvas;

class controller;

class artist;

struct data_node;
struct data_block;

class id_interface;
struct id_ref;
struct owned_id;
template<class Value>
class typed_id;

#define ALIA_DEFINE_GEOMETRY_TYPEDEFS(type) \
    typedef type<0, int> type##0i; \
    typedef type<1, int> type##1i; \
    typedef type<2, int> type##2i; \
    typedef type<3, int> type##3i; \
    typedef type<4, int> type##4i; \
    typedef type<0, unsigned> type##0u; \
    typedef type<1, unsigned> type##1u; \
    typedef type<2, unsigned> type##2u; \
    typedef type<3, unsigned> type##3u; \
    typedef type<4, unsigned> type##4u; \
    typedef type<0, float> type##0f; \
    typedef type<1, float> type##1f; \
    typedef type<2, float> type##2f; \
    typedef type<3, float> type##3f; \
    typedef type<4, float> type##4f; \
    typedef type<0, double> type##0d; \
    typedef type<1, double> type##1d; \
    typedef type<2, double> type##2d; \
    typedef type<3, double> type##3d; \
    typedef type<4, double> type##4d;

template<unsigned N, typename T>
struct point;
ALIA_DEFINE_GEOMETRY_TYPEDEFS(point)

template<unsigned N, typename T>
struct vector;
ALIA_DEFINE_GEOMETRY_TYPEDEFS(vector)

template<unsigned N, typename T>
struct box;
ALIA_DEFINE_GEOMETRY_TYPEDEFS(box)

struct font;
struct font_metrics;

class surface;

struct event;

struct key_event_info;

class layout_logic;
struct layout_data;
struct layout_object_data;

struct region_data;
typedef region_data* region_id;
static region_id const auto_id = 0;

class menu_context;
class menu_controller;

class grid_layout;

}

#endif
