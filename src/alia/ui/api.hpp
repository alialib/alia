#ifndef ALIA_UI_API_HPP
#define ALIA_UI_API_HPP

#include <alia/accessors.hpp>
#include <alia/color.hpp>
#include <alia/layout/api.hpp>
#include <alia/event_routing.hpp>

// This file declares all the types and functions necessary to use the UI
// library from the application end, including a standard library of widgets
// and containers.

namespace alia {

struct ui_context;

typedef void const* widget_id;
static widget_id const auto_id = 0;

// Often, widgets with internal storage will want to give the application the
// option of providing their own storage for that data. (This is useful if the
// application wants to persist that storage or needs to manipulate it directly
// in reponse to other user actions.)
// In those cases, the widget can accept an optional_storage<T> argument.
// The application can then call storage(accessor) to provide an accessor for
// the widget's storage, or it can pass in none to opt out of providing
// storage.
template<class T>
struct optional_storage : noncopyable
{
    optional_storage() {}
    optional_storage(none_type _) : storage(0) {}
    // 0 if no storage provided
    accessor<T> const* storage;
};
template<class Accessor>
struct accessor_storage : optional_storage<typename Accessor::value_type>
{
    accessor_storage(Accessor const& accessor)
      : accessor_(accessor)
    {
        this->storage = &accessor_;
    }
 private:
    Accessor accessor_;
};
template<class Accessor>
accessor_storage<Accessor> storage(Accessor const& accessor)
{ return accessor_storage<Accessor>(accessor); }

// resolve_storage(optional_storage, fallback) returns an accessor to the
// optional storage iff it's valid and to the fallback storage otherwise.
template<class T>
accessor_mux<indirect_accessor<T>,inout_accessor<T> >
resolve_storage(optional_storage<T> const& s, T* fallback)
{
    return select_accessor(s.storage != 0, ref(*s.storage), inout(fallback));
}

// STYLING

struct ui_controller : noncopyable
{
    virtual ~ui_controller() {}

    virtual void do_ui(ui_context& ctx) = 0;
};

// widget state flags
struct widget_state_flag_tag {};
typedef flag_set<widget_state_flag_tag> widget_state;
// primary state
ALIA_DEFINE_FLAG_CODE(widget_state_flag_tag, 0x01, WIDGET_NORMAL)
ALIA_DEFINE_FLAG_CODE(widget_state_flag_tag, 0x02, WIDGET_DISABLED)
ALIA_DEFINE_FLAG_CODE(widget_state_flag_tag, 0x03, WIDGET_HOT)
ALIA_DEFINE_FLAG_CODE(widget_state_flag_tag, 0x04, WIDGET_DEPRESSED)
ALIA_DEFINE_FLAG_CODE(widget_state_flag_tag, 0x05, WIDGET_SELECTED)
ALIA_DEFINE_FLAG_CODE(widget_state_flag_tag, 0x0f, WIDGET_PRIMARY_STATE_MASK)
// additional (independent) states
ALIA_DEFINE_FLAG_CODE(widget_state_flag_tag, 0x10, WIDGET_FOCUSED)

struct style_search_path;
struct dispatch_table;
struct primary_style_properties;

struct style_state
{
    style_search_path const* path;
    dispatch_table const* theme;
    primary_style_properties const* properties;
    id_interface const* id;
};

struct layout_style_info;

struct scoped_substyle : noncopyable
{
    scoped_substyle() : ctx_(0) {}
    scoped_substyle(ui_context& ctx, getter<string> const& substyle_name,
        widget_state state = WIDGET_NORMAL)
    { begin(ctx, substyle_name, state); }
    ~scoped_substyle() { end(); }

    void begin(ui_context& ctx, getter<string> const& substyle_name,
        widget_state state = WIDGET_NORMAL);
    void end();

 private:
    ui_context* ctx_;
    style_state old_state_;
    layout_style_info const* old_style_info_;
};

// CULLING

struct culling_block
{
    culling_block() {}
    culling_block(ui_context& ctx) { begin(ctx); }
    ~culling_block() { end(); }
    void begin(ui_context& ctx, layout const& layout_spec = default_layout);
    void end();
    bool is_relevant() const { return is_relevant_; }
 private:
    ui_context* ctx_;
    scoped_routing_region srr_;
    column_layout layout_;
    bool is_relevant_;
};

#define alia_if_relevant_(ctx) \
    { \
        ::alia::culling_block alia__culling_block(ctx); \
        { \
            ::alia::pass_dependent_if_block alia__if_block( \
                get_data_traversal(ctx), alia__culling_block.is_relevant()); \
            if (alia__culling_block.is_relevant()) \
            {

#define alia_if_relevant alia_if_relevant_(ctx)

// UI CACHING

struct layout_node;

struct cached_ui_block
{
    cached_ui_block() : ctx_(0) {}
    cached_ui_block(ui_context& ctx, id_interface const& id)
    { begin(ctx, id); }
    ~cached_ui_block() { end(); }
    void begin(ui_context& ctx, id_interface const& id);
    void end();
    bool is_relevant() const { return is_relevant_; }
 private:
    ui_context* ctx_;
    culling_block culling_;
    bool is_relevant_;
    layout_node** layout_next_ptr_;
};

#define alia_cached_ui_block_(ctx, id) \
    { \
        ::alia::cached_ui_block alia__cached_ui_block(ctx, id); \
        { \
            ::alia::pass_dependent_if_block alia__if_block( \
                get_data_traversal(ctx), alia__cached_ui_block.is_relevant()); \
            if (alia__cached_ui_block.is_relevant()) \
            {

#define alia_cached_ui_block(id) alia_cached_ui_block_(ctx, id)

// UI CONTEXT

struct ui_system;
struct surface;
struct ui_event;
struct ui_caching_node;

struct ui_context
{
    ui_system* system;
    data_traversal data;
    geometry_context geometry;
    layout_traversal layout;
    alia::surface* surface;
    ui_event* event;
    event_routing_traversal routing;
    style_state style;
    ui_caching_node* active_cacher;
    bool pass_aborted;
    double layer_z;
};

static inline data_traversal& get_data_traversal(ui_context& ctx)
{ return ctx.data; }
static inline layout_traversal& get_layout_traversal(ui_context& ctx)
{ return ctx.layout; }
static inline geometry_context& get_geometry_context(ui_context& ctx)
{ return ctx.geometry; }

void end_pass(ui_context& ctx);

struct untyped_ui_value
{
    virtual ~untyped_ui_value() {}
};

template<class T>
struct typed_ui_value : untyped_ui_value
{
    T value;
};

template<class T>
struct control_result
{
    bool changed;
    T new_value;
    // allows use within if statements without other unintended conversions
    typedef bool control_result::* unspecified_bool_type;
    operator unspecified_bool_type() const
    { return changed ? &control_result::changed : 0; }
};

// COMMON FLAGS

struct ui_flag_tag {};
typedef flag_set<ui_flag_tag> ui_flag_set;
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x000004, NO_FOCUS_INDICATOR)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x000008, DISABLED)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x000010, ROUNDED)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x000020, HORIZONTAL)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x000040, VERTICAL)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x000080, COMMAND_LIST)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x000100, FLIPPED)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x000200, PREPEND)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x000400, APPEND)
// expandables
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x001000, INITIALLY_EXPANDED)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x002000, INITIALLY_COLLAPSED)
// text controls
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x010000, PASSWORD)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x020000, SINGLE_LINE)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x040000, MULTILINE)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x080000, NO_PANEL)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x100000, ALWAYS_EDITING)
// panels
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x200000, NO_INTERNAL_PADDING)
ALIA_DEFINE_FLAG_CODE(ui_flag_tag, 0x400000, NO_CLICK_DETECTION)

// DISPLAYS - non-interactive widgets

void do_separator(ui_context& ctx, layout const& layout_spec = default_layout);

void do_color(ui_context& ctx, getter<rgba8> const& color,
    layout const& layout_spec = default_layout);

void do_progress_bar(ui_context& ctx, getter<double> const& progress,
    layout const& layout_spec = default_layout);

void do_bullet(ui_context& ctx, layout const& layout_spec = default_layout);

struct bulleted_list : noncopyable
{
    bulleted_list() {}
    bulleted_list(ui_context& ctx, layout const& layout_spec = default_layout)
    { begin(ctx, layout_spec); }
    ~bulleted_list() { end(); }
    void begin(ui_context& ctx, layout const& layout_spec = default_layout);
    void end();
 private:
    friend struct bulleted_item;
    ui_context* ctx_;
    grid_layout grid_;
};

struct bulleted_item : noncopyable
{
    bulleted_item() {}
    bulleted_item(bulleted_list& list,
        layout const& layout_spec = default_layout)
    { begin(list, layout_spec); }
    ~bulleted_item() { end(); }
    void begin(bulleted_list& list,
        layout const& layout_spec = default_layout);
    void end();
 private:
    grid_row row_;
};

// TEXT

void do_text(ui_context& ctx, getter<string> const& text,
    layout const& layout_spec = default_layout);

#define ALIA_DECLARE_STRING_CONVERSIONS(T) \
    bool from_string(T* value, string const& str, string* message); \
    string to_string(T value);

ALIA_DECLARE_STRING_CONVERSIONS(int)
ALIA_DECLARE_STRING_CONVERSIONS(unsigned)
ALIA_DECLARE_STRING_CONVERSIONS(float)
ALIA_DECLARE_STRING_CONVERSIONS(double)

struct cached_string_conversion
{
    cached_string_conversion() : valid(false) {}
    bool valid;
    owned_id id;
    string text;
};

struct cached_string_conversion_accessor : accessor<string>
{
    cached_string_conversion_accessor(cached_string_conversion* cache)
      : cache_(cache)
    {}
    bool is_gettable() const { return cache_->valid; }
    string const& get() const { return cache_->text; };
    id_interface const& id() const { return cache_->id.get(); }
    bool is_settable() const { return false; }
    void set(string const& value) const {}
 private:
    cached_string_conversion* cache_;
};

template<class T>
cached_string_conversion_accessor
as_text(ui_context& ctx, getter<T> const& value)
{
    cached_string_conversion* cache;
    get_cached_data(ctx, &cache);
    if (!cache->valid || !cache->id.matches(value.id()))
    {
        if (value.is_gettable())
        {
            cache->text = to_string(get(value));
            cache->valid = true;
        }
        else
            cache->valid = false;
        cache->id.store(value.id());
    }
    return cached_string_conversion_accessor(cache);
}

template<class T>
void do_text(ui_context& ctx, getter<T> const& value,
    layout const& layout_spec = default_layout)
{
    do_text(ctx, as_text(ctx, value), layout_spec);
}

cached_string_conversion_accessor
format_number(ui_context& ctx, char const* format,
    getter<double> const& number);

void do_paragraph(ui_context& ctx, getter<string> const& text,
    layout const& layout_spec = default_layout);

void do_label(ui_context& ctx, getter<string> const& text,
    layout const& layout_spec = default_layout);

bool do_link(
    ui_context& ctx,
    getter<string> const& text,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

void draw_text(ui_context& ctx, getter<string> const& text,
    vector<2,double> const& position);

void do_layout_dependent_text(ui_context& ctx, getter<string> const& text,
    layout const& layout_spec);

// BUTTONS

// text button

typedef bool button_result;

button_result
do_button(
    ui_context& ctx,
    getter<string> const& text,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

// icon button

typedef bool icon_button_result;

enum icon_type
{
    REMOVE_ICON,
    EXPAND_ICON,
    SHRINK_ICON,
};

icon_button_result
do_icon_button(
    ui_context& ctx,
    icon_type icon,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

// CONTROLS

// check box

struct check_box_result : control_result<bool> {};

check_box_result
do_check_box(
    ui_context& ctx,
    accessor<bool> const& value,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

check_box_result
do_check_box(
    ui_context& ctx,
    accessor<bool> const& value,
    getter<string> const& text,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

// radio button

struct radio_button_result : control_result<bool> {};

radio_button_result
do_radio_button(
    ui_context& ctx,
    accessor<bool> const& value,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

radio_button_result
do_radio_button(
    ui_context& ctx,
    accessor<bool> const& value,
    getter<string> const& text,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

template<class Index>
struct indexed_accessor : regular_accessor<bool>
{
    indexed_accessor(
        accessor<Index> const& selected_value,
        getter<Index> const& this_value)
      : selected_value_(selected_value), this_value_(this_value)
    {}
    bool is_gettable() const
    { return selected_value_.is_gettable() && this_value_.is_gettable(); }
    bool const& get() const { return lazy_getter_.get(*this); }
    bool is_settable() const
    { return selected_value_.is_settable() && this_value_.is_gettable(); }
    void set(bool const& value) const
    { selected_value_.set(this_value_.get()); }
 private:
    friend struct lazy_getter<bool>;
    bool generate() const
    { return selected_value_.get() == this_value_.get(); }
    accessor<Index> const& selected_value_;
    getter<Index> const& this_value_;
    lazy_getter<bool> lazy_getter_;
};

template<class Index>
indexed_accessor<Index>
make_indexed_accessor(
    accessor<Index> const& selected_value,
    getter<Index> const& this_value)
{
    return indexed_accessor<Index>(selected_value, this_value);
}

template<class Index>
radio_button_result
do_radio_button(
    ui_context& ctx,
    accessor<Index> const& selected_value,
    getter<Index> const& this_value,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id)
{
    return do_radio_button(ctx,
        make_indexed_accessor(selected_value, this_value),
        layout_spec, id);
}

template<class Index>
radio_button_result
do_radio_button(
    ui_context& ctx,
    accessor<Index> const& selected_value,
    getter<Index> const& this_value,
    getter<string> const& text,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id)
{
    return do_radio_button(ctx,
        make_indexed_accessor(selected_value, this_value),
        text, layout_spec, id);
}

// node expander

struct node_expander_result : control_result<bool> {};

node_expander_result do_node_expander(
    ui_context& ctx,
    accessor<bool> const& value,
    layout const& layout_spec = default_layout,
    widget_id id = auto_id);

// slider
// accepted flags:
// HORIZONTAL, VERTICAL (mutually exclusive, default is HORIZONTAL)
struct slider_result : control_result<double>
{};
slider_result
do_slider(ui_context& ctx, accessor<double> const& value,
    double minimum, double maximum, double step = 0,
    layout const& layout_spec = default_layout, ui_flag_set flags = NO_FLAGS);

// CONTAINERS

struct bordered_box : noncopyable
{
 public:
    bordered_box() {}
    // accepted flags: HORIZONTAL, VERTICAL - controls layout of children
    bordered_box(
        ui_context& ctx, layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS)
    { begin(ctx, layout_spec, flags); }
    ~bordered_box() { end(); }
    void begin(
        ui_context& ctx, layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS);
    void end();
    layout_box region() const { return box_.region(); }
 private:
    linear_layout box_;
};

struct panel : noncopyable
{
 public:
    panel() : ctx_(0) {}
    panel(
        ui_context& ctx, getter<string> const& style,
        layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS,
        widget_id id = auto_id,
        widget_state state = WIDGET_NORMAL)
    { begin(ctx, style, layout_spec, flags, id, state); }
    ~panel() { end(); }
    void begin(
        ui_context& ctx, getter<string> const& style,
        layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS,
        widget_id id = auto_id,
        widget_state state = WIDGET_NORMAL);
    void end();
    layout_box outer_region() const;
    layout_box inner_region() const { return inner_.region(); }
 private:
    ui_context* ctx_;
    column_layout outer_;
    scoped_substyle substyle_;
    linear_layout inner_;
    ui_flag_set flags_;
};

class clickable_panel : noncopyable
{
 public:
    clickable_panel() {}
    clickable_panel(
        ui_context& ctx, layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS, widget_id id = auto_id)
    { begin(ctx, layout_spec, flags, id); }
    void begin(
        ui_context& ctx, layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS, widget_id id = auto_id);
    void end() { panel_.end(); }
    layout_box outer_region() const { return panel_.outer_region(); }
    layout_box inner_region() const { return panel_.inner_region(); }
    bool clicked() const { return clicked_; }
 private:
    panel panel_;
    bool clicked_;
};

struct scrollable_layout_container;

struct scrollable_region : noncopyable
{
    scrollable_region() : ctx_(0) {}
    scrollable_region(
        ui_context& ctx,
        layout const& layout_spec = default_layout,
        unsigned scrollable_axes = 1 | 2,
        widget_id id = auto_id)
    { begin(ctx, layout_spec, scrollable_axes, id); }
    ~scrollable_region() { end(); }

    void begin(
        ui_context& ctx,
        layout const& layout_spec = default_layout,
        unsigned scrollable_axes = 1 | 2,
        widget_id id = auto_id);
    void end();

 private:
    ui_context* ctx_;
    scrollable_layout_container* container_;
    widget_id id_;
    scoped_clip_region scr_;
    scoped_transformation transform_;
    scoped_layout_container slc_;
    scoped_routing_region srr_;
};

struct scrollable_panel : noncopyable
{
 public:
    scrollable_panel() {}
    scrollable_panel(
        ui_context& ctx, getter<string> const& style,
        unsigned scrollable_axes = 1 | 2,
        layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS)
    { begin(ctx, style, scrollable_axes, layout_spec, flags); }
    ~scrollable_panel() { end(); }
    void begin(
        ui_context& ctx, getter<string> const& style,
        unsigned scrollable_axes = 1 | 2,
        layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS);
    void end();
 private:
    column_layout outer_;
    scoped_substyle substyle_;
    scrollable_region region_;
    linear_layout inner_;
};

struct tree_node : noncopyable
{
    tree_node() {}
    ~tree_node() { end(); }

    tree_node(
        ui_context& ctx,
        layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS,
        optional_storage<bool> const& expanded = optional_storage<bool>(none),
        widget_id expander_id = auto_id)
    { begin(ctx, layout_spec, flags, expanded, expander_id); }

    void begin(
        ui_context& ctx,
        layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS,
        optional_storage<bool> const& expanded = optional_storage<bool>(none),
        widget_id expander_id = auto_id);

    void end_header();

    bool do_children();

    node_expander_result const& expander_result() const
    { return expander_result_; }

    void end();

 private:
    ui_context* ctx_;

    grid_layout grid_;
    grid_row row_;
    column_layout column_;
    row_layout label_region_;

    bool do_children_;
    node_expander_result expander_result_;
};

// POPUPS

struct popup
{
    popup() : ctx_(0) {}
    popup(ui_context& ctx, widget_id id,
        layout_vector const& lower_bound,
        layout_vector const& upper_bound)
    { begin(ctx, id, lower_bound, upper_bound); }
    ~popup() { end(); }
    void begin(ui_context& ctx, widget_id id,
        layout_vector const& lower_bound,
        layout_vector const& upper_bound);
    void end();
    bool user_closed();
 private:
    ui_context* ctx_;
    double old_layer_z_;
    widget_id id_, background_id_;
    floating_layout layout_;
    scoped_transformation transform_;
};

// DROP DOWNS

struct ddl_data;

struct untyped_drop_down_list : noncopyable
{
 public:
    untyped_drop_down_list() : ctx_(0) {}
    ~untyped_drop_down_list() { end(); }

    untyped_ui_value const*
    begin(ui_context& ctx, layout const& layout_spec, ui_flag_set flags);
    void end();

    bool do_list();

 private:
    friend struct untyped_ddl_item;

    ui_context* ctx_;
    ddl_data* data_;
    widget_id id_;
    panel container_;
    row_layout contents_;

    bool do_list_, make_selection_visible_;
    popup popup_;
    bordered_box list_border_;
    scrollable_panel list_panel_;
    int list_index_;
};

template<class Index>
struct drop_down_list : noncopyable
{
 public:
    drop_down_list() : changed_(false) {}
    drop_down_list(ui_context& ctx, accessor<Index> const& selection,
        layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS)
    { begin(ctx, selection, layout_spec, flags); }
    ~drop_down_list() { end(); }

    void begin(ui_context& ctx, accessor<Index> const& selection,
        layout const& layout_spec = default_layout,
        ui_flag_set flags = NO_FLAGS)
    {
        if (selection.is_gettable())
            selection_ = get(selection);
        else
            selection_ = none;

        untyped_ui_value const* new_value =
            list_.begin(ctx, layout_spec, flags);
        if (new_value)
        {
            typed_ui_value<Index> const* v =
                dynamic_cast<typed_ui_value<Index> const*>(new_value);
            // This should only fail if somehow an event with the wrong value
            // type is somehow sent to this widget.
            assert(v);
            if (v)
            {
                set(selection, v->value);
                changed_ = true;
            }
        }
    }
    void end() { list_.end(); }

    bool do_list() { return list_.do_list(); }

    bool changed() const { return changed_; }

 private:
    template<class Index>
    friend struct ddl_item;

    untyped_drop_down_list list_;
    optional<Index> selection_;
    bool changed_;
};

struct untyped_ddl_item : noncopyable
{
 public:
    untyped_ddl_item() : list_(0) {}
    ~untyped_ddl_item() { end(); }
    bool begin(untyped_drop_down_list& list, bool is_selected);
    void end();
    void select(untyped_ui_value* value);
    bool is_selected() const { return selected_; }
 private:
    untyped_drop_down_list* list_;
    bool selected_;
    panel panel_;
};

template<class Index>
struct ddl_item : noncopyable
{
 public:
    ddl_item() {}
    ddl_item(drop_down_list<Index>& list, Index const& index)
    { begin(list, index); }
    ~ddl_item() { end(); }

    void begin(drop_down_list<Index>& list, Index const& index)
    {
        if (item_.begin(list.list_,
            list.selection_ ? get(list.selection_) == index : false))
        {
            typed_ui_value<Index>* v = new typed_ui_value<Index>;
            v->value = index;
            item_.select(v);
        }
    }
    void end() { item_.end(); }

    bool is_selected() const { return item_.is_selected(); }

 private:
    untyped_ddl_item item_;
    bool selected_;
};

// TEXT CONTROL

enum text_control_event_type
{
    TEXT_CONTROL_NO_EVENT,
    TEXT_CONTROL_ENTER_PRESSED,
    TEXT_CONTROL_FOCUS_LOST,
    TEXT_CONTROL_INVALID_VALUE,
    TEXT_CONTROL_EDIT_CANCELED,
};

template<class T>
struct text_control_result : control_result<T>
{
    text_control_event_type event;
};

text_control_result<string>
do_text_control(
    ui_context& ctx,
    accessor<string> const& value,
    layout const& layout_spec = default_layout,
    ui_flag_set flags = NO_FLAGS,
    widget_id id = auto_id,
    optional<size_t> const& length_limit = none);

struct text_control_string_conversion
{
    text_control_string_conversion() : valid(false) {}
    bool valid;
    owned_id id;
    string text;
    // associated error message (if text doesn't parse)
    string message;
};

template<class T>
text_control_result<T>
do_text_control(
    ui_context& ctx,
    accessor<T> const& accessor,
    layout const& layout_spec = default_layout,
    ui_flag_set flags = NO_FLAGS,
    widget_id id = auto_id,
    optional<size_t> const& length_limit = none)
{
    layout spec = add_default_alignment(layout_spec, LEFT, BASELINE_Y);
    column_layout c(ctx, spec);

    text_control_string_conversion* data;
    get_data(ctx, &data);
    if (ctx.event->type == REFRESH_EVENT)
    {
        bool valid = accessor.is_gettable();
        if (data->valid != valid || valid && !data->id.matches(accessor.id()))
        {
            // The external value has changed.
            data->valid = valid;
            data->text = valid ? to_string(get(accessor)) : "";
            data->message = "";
            data->id.store(accessor.id());
        }
    }

    text_control_result<string> r = do_text_control(
        ctx, inout(&data->text), spec, flags, id, length_limit);
    alia_if(!data->message.empty())
    {
        do_paragraph(ctx, in(data->message));
    }
    alia_end

    text_control_result<T> result;
    switch (r.event)
    {
     case TEXT_CONTROL_FOCUS_LOST:
     case TEXT_CONTROL_ENTER_PRESSED:
      {
        T new_value;
        if (from_string(&new_value, data->text, &data->message))
        {
            data->message = "";
            result.event = r.event;
            result.changed = true;
            result.new_value = new_value;
            accessor.set(new_value);
        }
        else
        {
            result.event = TEXT_CONTROL_INVALID_VALUE;
            result.changed = false;
        }
        break;
      }
     case TEXT_CONTROL_EDIT_CANCELED:
        result.event = TEXT_CONTROL_EDIT_CANCELED;
        result.changed = false;
        data->text = accessor.is_gettable() ? to_string(get(accessor)) : "";
        data->message = "";
        break;
     case TEXT_CONTROL_NO_EVENT:
     default:
        result.event = TEXT_CONTROL_NO_EVENT;
        result.changed = false;
        break;
    }
    return result;
}

bool do_draggable_separator(ui_context& ctx, accessor<int> const& width,
    layout const& layout_spec = default_layout, ui_flag_set flags = NO_FLAGS,
    widget_id id = auto_id);

// resizable_content is a container with a draggable separator for controlling
// the size of its contents.
// accepted flags:
// HORIZONTAL, VERTICAL - the orientation of the separator (default: VERTICAL)
// PREPEND, APPEND - is the separator appended or preprended (default: APPEND)
struct resizable_content : noncopyable
{
    resizable_content() {}
    resizable_content(ui_context& ctx, accessor<int> const& size,
        ui_flag_set flags = NO_FLAGS)
    { begin(ctx, size, flags); }
    ~resizable_content() { end(); }
    void begin(ui_context& ctx, accessor<int> const& size,
        ui_flag_set flags = NO_FLAGS);
    void end();
 private:
    ui_context* ctx_;
    widget_id id_;
    ui_flag_set flags_;
    int size_;
    linear_layout layout_;
};

}

#endif
