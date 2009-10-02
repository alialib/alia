#ifndef ALIA_INTERLEAVED_TREE_HPP
#define ALIA_INTERLEAVED_TREE_HPP

#include <alia/linear_layout.hpp>

namespace alia {

// accepted flags: INITIALLY_EXPANDED

class interleaved_tree
{
 public:
    interleaved_tree() : active_(false) {}
    interleaved_tree(context& ctx) {}
    void begin(context& ctx);
    void end();
 private:
    context* ctx_;
    bool active_;
};

class interleaved_tree_node
{
 public:
    interleaved_tree_node() : active_(false) {}
    interleaved_tree_node(interleaved_tree& tree,
        unsigned flags = 0, layout const& layout_spec = default_layout)
    { begin(tree, flags, layout_spec); }

    void begin(interleaved_tree& tree, unsigned flags = 0,
        layout const& layout_spec = default_layout);

    bool do_children();

    void end();

 private:
    interleaved_tree* tree_;
    row_layout row_;
    bool active_, do_children_;
};

class interleaved_tree_contents
{
    
};

}

#endif
