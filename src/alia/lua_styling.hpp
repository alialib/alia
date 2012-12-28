#ifndef ALIA_LUA_STYLING_HPP
#define ALIA_LUA_STYLING_HPP

#include <alia/ui/api.hpp>

namespace alia {

struct style_tree;

void read_lua_style_file(style_tree* tree, string const& path);

}

#endif
