#ifndef ALIA_INDIE_RENDERING_HPP
#define ALIA_INDIE_RENDERING_HPP

#include <alia/indie/common.hpp>

#include <include/core/SkCanvas.h>

namespace alia { namespace indie {

struct render_node
{
    virtual void
    render(SkCanvas& canvas)
        = 0;

    render_node* next = nullptr;
};

struct render_container : render_node
{
    render_node* children = nullptr;
};

void
render_children(render_container& container);

struct render_traversal
{
    render_node** next_ptr = nullptr;
};

void
add_render_node(render_traversal& traversal, render_node* node);

struct scoped_render_container
{
    ~scoped_render_container()
    {
        end();
    }

    void
    begin(render_traversal& traversal, render_container* container);

    void
    end();

 private:
    render_traversal* traversal_ = nullptr;
    render_container* container_;
};

}} // namespace alia::indie

#endif
