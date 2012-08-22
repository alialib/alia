#include <alia/lua_styling.hpp>
#include <alia/ui_system.hpp>

extern "C" {
    #include <lua.h>
    #include <lauxlib.h>
    #include <lualib.h>
}

namespace alia {

// TODO: Make this thread-safe!

static style_tree* the_alia_style_tree = 0;

static int set_style(lua_State* L)
{
    if (!the_alia_style_tree)
        luaL_error(L, "no alia context set");

    if (lua_gettop(L) != 2)
        luaL_error(L, "usage: set_style(style, properties)");

    char const* style = luaL_checkstring(L, 1);

    luaL_checktype(L, 2, LUA_TTABLE);

    alia::property_map properties;

    // Iterate through the key/value pairs in the table.
    lua_pushnil(L); // first key
    while (lua_next(L, 2))
    {
        // Now the key is at index -2 and the value is at index -1.
        char const* key = luaL_checkstring(L, -2);
        if (lua_isboolean(L, -1))
            properties[key] = lua_toboolean(L, -1) != 0 ? "true" : "false";
        else if (lua_isnumber(L, -1) || lua_isstring(L, -1))
            properties[key] = lua_tostring(L, -1);
        else
            luaL_error(L, "invalid property value type");
        // Pop the value.
        lua_pop(L, 1);
        // The key is replaced by the next call to lua_next().
    }

    set_style(*the_alia_style_tree, style, properties);

    return 0;
}

static luaL_Reg the_api[] =
{
    { "style", &set_style },
    { 0, 0 }
};

void read_lua_style_file(style_tree* tree, string const& path)
{
    lua_State *L = lua_open();
    luaL_register(L, "alia", the_api);
    *tree = style_tree();
    the_alia_style_tree = tree;
    bool failed = false;
    string error_msg;
    if (luaL_dofile(L, path.c_str()))
    {
        failed = true;
        error_msg = lua_tostring(L, -1);
    }
    lua_close(L);
    if (failed)
        throw exception(path + ": " + error_msg);
}

}
