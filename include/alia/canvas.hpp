#ifndef ALIA_CANVAS_HPP
#define ALIA_CANVAS_HPP

#include <alia/scoped_state.hpp>
#include <alia/layout_interface.hpp>
#include <alia/accessor.hpp>
#include <alia/box.hpp>
#include <alia/input_defs.hpp>
#include <alia/flags.hpp>
#include <alia/grid_layout.hpp>

namespace alia {

// The zoom level for a canvas can either be set directly using a numeric
// value or specified dynamically by supplying a function that calculates the
// zoom level as a function of scene size and canvas size.
// Note that the units of the zoom level are canvas pixels per scene unit.

typedef double (*zoom_function)(vector2i const& canvas_size,
    vector2d const& scene_size);

// The following functions are suitable for use as a dynamic zoom level...

// Calculate the zoom level that will fit the entire scene in the canvas.
double zoom_to_fit_scene(vector2i const& canvas_size,
    vector2d const& scene_size);

// Calculate the zoom level that will vertically fit the scene in the canvas.
double zoom_to_fit_scene_height(vector2i const& canvas_size,
    vector2d const& scene_size);

// Calculate the zoom level that will horizontally fit the scene in the canvas.
double zoom_to_fit_scene_width(vector2i const& canvas_size,
    vector2d const& scene_size);

// Calculate the zoom level so that the scene will just barely fill the canvas.
double zoom_to_fill_canvas(vector2i const& canvas_size,
    vector2d const& scene_size);

struct zoom_level
{
    zoom_level() : function(&zoom_to_fit_scene) {}
    zoom_level(zoom_function fn) : function(fn) {}
    zoom_level(double level) : function(0), level(level) {}

    void operator=(zoom_function fn) { function = fn; }
    void operator=(double level)
    {
        this->level = level;
        function = zoom_function();
    }

    double evaluate(vector2i const& canvas_size,
        vector2d const& scene_size) const
    {
        if (function)
            return function(canvas_size, scene_size);
        else
            return level;
    }

    bool is_function() const { return function ? true : false; }

    zoom_function function;
    // if the function is null, this is the zoom level
    double level;
};

class canvas;

class camera
{
 public:
    virtual ~camera() {}
    virtual point2d get_position(canvas const& canvas) const = 0;
    virtual void set_position(canvas const& canvas, point2d const& p) = 0;
    virtual double get_zoom_level(canvas const& canvas) const = 0;
    virtual void set_zoom_level(canvas const& canvas, zoom_level zoom) = 0;
    virtual void check_bounds(canvas const& canvas) = 0;
};

class default_camera : public camera
{
 public:
    default_camera() : position_(0, 0), min_zoom_(0.), max_zoom_(0.),
        constrained_(false), initialized_(false) {}
    bool is_initialized() const { return initialized_; }
    void initialize(point2d const& p, zoom_level zoom)
    { position_ = p; zoom_ = zoom; initialized_ = true; }
    point2d get_position(canvas const& canvas) const;
    void set_position(canvas const& canvas, point2d const& p);
    double get_zoom_level(canvas const& canvas) const;
    void set_zoom_level(canvas const& canvas, zoom_level zoom);
    void set_min_zoom(zoom_level zoom) { min_zoom_ = zoom; }
    double get_min_zoom(canvas const& canvas) const;
    void set_max_zoom(zoom_level zoom) { max_zoom_ = zoom; }
    double get_max_zoom(canvas const& canvas) const;
    void set_constrained(bool constrained) { constrained_ = constrained; }
    void check_bounds(canvas const& canvas);
 private:
    point2d position_;
    zoom_level zoom_, min_zoom_, max_zoom_;
    bool constrained_, initialized_;
};

static flag_set const FLIP_X(CUSTOM_FLAG_0_CODE);
static flag_set const FLIP_Y(CUSTOM_FLAG_1_CODE);

class canvas : boost::noncopyable
{
 public:
    canvas() : active_(false) {}
    canvas(context& ctx, box2d const& scene_box, camera* camera = 0,
        flag_set flags = NO_FLAGS, layout const& layout_spec = default_layout)
    { begin(ctx, scene_box, camera, flags, layout_spec); }
    ~canvas() { end(); }

    void begin(context& ctx, box2d const& scene_box, camera* camera = 0,
        flag_set flags = NO_FLAGS, layout const& layout_spec = default_layout);
    void end();

    context& context() const { return *ctx_; }

    region_id id() const { return id_; }
    box2i const& region() const;

    box2d const& scene_box() const { return scene_box_; }

    camera& camera() const { return *camera_; }

    point2d get_camera_position() const
    { return camera().get_position(*this); }
    void set_camera_position(point2d const& p) const
    { camera().set_position(*this, p); }

    double get_zoom_level() const
    { return camera().get_zoom_level(*this); }
    void set_zoom_level(zoom_level zoom) const
    { camera().set_zoom_level(*this, zoom); }

    bool flip_x() const { return (flags_ & FLIP_X) != 0; }
    bool flip_y() const { return (flags_ & FLIP_Y) != 0; }

    void set_scene_coordinates();
    void set_canvas_coordinates();

 private:
    alia::context* ctx_;
    struct data;
    data* data_;
    alia::camera* camera_;
    region_id id_;
    box2d scene_box_;
    flag_set flags_;
    bool active_;
    scoped_transformation st_;
    scoped_clip_region scr_;
};

point2d canvas_to_scene(canvas& c, point2d const& p);
point2d scene_to_canvas(canvas& c, point2d const& p);

void zoom_to_box(canvas& canvas, box2d const& box);

// panning tool - This allows the user to drag the scene around using the
// specified mouse button.
void apply_panning_tool(canvas& canvas, mouse_button button);

void apply_zoom_wheel_tool(canvas& canvas, unsigned mods = 0,
    double factor = 1.1);

void draw_checker_background(canvas& canvas, rgba8 const& color1,
    rgba8 const& color2, double spacing);

// zoom box tool - This tool allows the user to zoom in to a rectangular area
// of the scene by dragging the mouse and drawing a box.
typedef point2d zoom_box_tool_data;
void apply_zoom_box_tool(canvas& canvas, mouse_button button,
    rgba8 const& color, line_style const& style = line_style(1, solid_line),
    zoom_box_tool_data* data = 0);

struct zoom_drag_tool_data
{
    point2d start_point_in_scene, starting_camera_position;
    point2i start_point_on_canvas;
    double starting_zoom;
    vector2d zoom_out_translation;
};
void apply_zoom_drag_tool(canvas& canvas, mouse_button button,
    zoom_drag_tool_data* data = 0);

void draw_grid_lines_for_axis(canvas& canvas, box2d const& box,
    rgba8 const& color, line_style const& style, double spacing,
    unsigned axis, unsigned skip = 0);

void draw_grid_lines(canvas& canvas, box2d const& box, rgba8 const& color,
    line_style const& style, double spacing, unsigned axis, unsigned skip = 0);

static flag_set const DRAW_BORDER(CUSTOM_FLAG_0_CODE);
int get_ruler_width(context& ctx, flag_set flags);
void draw_side_ruler(
    canvas& canvas, box2i const& region,
    rgba8 const& bg_color, rgba8 const& fg_color,
    double scale, flag_set flags);

void apply_double_click_reset_tool(canvas& canvas, mouse_button button,
    zoom_level const& default_zoom_level);

struct side_rulers : boost::noncopyable
{
 public:
    side_rulers() : active_(false) {}
    side_rulers(context& ctx, flag_set flags,
        layout const& layout_spec = default_layout)
    { begin(ctx, flags, layout_spec); }
    ~side_rulers() { end(); }
    void set_scale(unsigned axis, double scale);
    void set_units(std::string const& units);
    void begin(context& ctx, flag_set flags,
        layout const& layout_spec = default_layout);
    void set_canvas(canvas& canvas);
    void end();
 private:
    void reserve_space(flag_set side, unsigned index);
    void do_corner();
    void do_ruler_row(flag_set side);
    void draw_ruler(flag_set flags, unsigned index);
    context* ctx_;
    grid_layout grid_;
    grid_row row_;
    flag_set flags_;
    canvas* canvas_;
    box2i regions_[2];
    double scales_[2];
    std::string units_;
    bool active_;
};

//struct side_labels : boost::noncopyable
//{
//    side_labels() : active_(false) {}
//    side_labels(context& ctx, flag_set flags,
//        layout const& layout_spec = default_layout)
//    { begin(ctx, flags, layout_spec); }
//    ~side_labels() { end(); }
//    void 
//};

}

#endif
