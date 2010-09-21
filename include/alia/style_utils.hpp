#ifndef ALIA_STYLE_UTILS_HPP
#define ALIA_STYLE_UTILS_HPP

#include <alia/context.hpp>
#include <alia/style_tree.hpp>

namespace alia {

struct const_style_name_type {};
static const_style_name_type const_style_name;

struct scoped_substyle : boost::noncopyable
{
    scoped_substyle() : active_(false) {}
    scoped_substyle(context& ctx, char const* name,
        widget_state state = widget_states::NORMAL)
    { begin(ctx, name, state); }
    scoped_substyle(context& ctx, char const* name, const_style_name_type _)
    { begin(ctx, name, _); }
    ~scoped_substyle() { end(); }
    void begin(context& ctx, char const* name,
        widget_state state = widget_states::NORMAL);
    void begin(context& ctx, char const* name, const_style_name_type _);
    void end();
 private:
    bool active_;
    context* ctx_;
    style_node const* old_style_;
    primary_style_properties const* old_props_;
};

// These are the primary style properties that are stored in the context's
// pass state.

void get_style_properties(context& ctx, style_node const* style,
    primary_style_properties& props,
    widget_state state = widget_states::NORMAL);

void apply_style_properties(context& ctx,
    primary_style_properties& props);

// Some utility functions for getting common datatypes...

void get_font_property(font* result, context& ctx, style_node const* style,
    std::string const& subpath, widget_state state = widget_states::NORMAL);

void get_color_property(rgba8* result, style_node const* style,
    std::string const& subpath, widget_state state = widget_states::NORMAL);

void get_numeric_property(double* result, style_node const* style,
    std::string const& subpath, widget_state state = widget_states::NORMAL);
void get_numeric_property(int* result, style_node const* style,
    std::string const& subpath, widget_state state = widget_states::NORMAL);

// These are simple convenience functions that use the active_style associated
// with a context...

namespace {

style_node const* get_substyle(context& ctx, std::string const& subpath)
{ return get_substyle(ctx.pass_state.active_style, subpath); }

template<class T>
bool get_property(T* result, context& ctx, std::string const& subpath,
    widget_state state = widget_states::NORMAL)
{
    return get_property(result, ctx.pass_state.active_style, subpath, state);
}

void get_font_property(font* result, context& ctx,
    std::string const& subpath, widget_state state = widget_states::NORMAL)
{
    return get_font_property(result, ctx, ctx.pass_state.active_style, subpath,
        state);
}

void get_color_property(rgba8* result, context& ctx,
    std::string const& subpath, widget_state state = widget_states::NORMAL)
{
    return get_color_property(result, ctx.pass_state.active_style, subpath,
        state);
}

void get_numeric_property(double* result, context& ctx,
    std::string const& subpath, widget_state state = widget_states::NORMAL)
{
    return get_numeric_property(result, ctx.pass_state.active_style, subpath,
        state);
}
void get_numeric_property(int* result, context& ctx,
    std::string const& subpath, widget_state state = widget_states::NORMAL)
{
    return get_numeric_property(result, ctx.pass_state.active_style, subpath,
        state);
}

}

// It's generally inefficient to use the style_tree's functions directly
// within an alia UI function because of the expensive lookups. The following
// are facilities for caching the results of style queries...

struct style_versioning_data
{
    unsigned tree_version;
    style_node const* node;
};

static inline bool is_outdated(context& ctx,
    style_versioning_data& data)
{
    return data.tree_version != ctx.style_tree->version() ||
        data.node != ctx.pass_state.active_style;
}

static inline void update(context& ctx, style_versioning_data& data)
{
    data.tree_version = ctx.style_tree->version();
    data.node = ctx.pass_state.active_style;
}

template<class Data>
struct cached_style_data_node
{
    Data data;
    style_versioning_data versioning;
};

// get_cached_style_data() is similar to get_data(), but it will request
// reinitialization of the underlying data when the style_tree is updated.
template<class Data>
bool get_cached_style_data(context& ctx, Data** data)
{
    cached_style_data_node<Data>* node;
    bool needs_init = false;
    if (get_data(ctx, &node) || is_outdated(ctx, node->versioning))
    {
        update(ctx, node->versioning);
        needs_init = true;
    }
    *data = &node->data;
    return needs_init;
}

// These are analogous, but for when the style data is dependent upon a widget
// state...

struct stateful_style_versioning_data
{
    unsigned tree_version;
    style_node const* node;
    widget_state state;
};

static inline bool is_outdated(context& ctx,
    stateful_style_versioning_data& data, widget_state state)
{
    return data.tree_version != ctx.style_tree->version() ||
        data.node != ctx.pass_state.active_style || data.state != state;
}

static inline void update(context& ctx,
    stateful_style_versioning_data& data, widget_state state)
{
    data.tree_version = ctx.style_tree->version();
    data.node = ctx.pass_state.active_style;
    data.state = state;
}

template<class Data>
struct stateful_cached_style_data_node
{
    Data data;
    stateful_style_versioning_data versioning;
};

template<class Data>
bool get_cached_style_data(context& ctx, Data** data, widget_state state)
{
    stateful_cached_style_data_node<Data>* node;
    bool needs_init = false;
    if (get_data(ctx, &node) || is_outdated(ctx, node->versioning, state))
    {
        update(ctx, node->versioning, state);
        needs_init = true;
    }
    *data = &node->data;
    return needs_init;
}

}

#endif
