#include <alia/lua_styling.hpp>
#include <alia/style_tree.hpp>

extern "C" {
    #include <lua.h>
    #include <lauxlib.h>
    #include <lualib.h>
}

namespace alia {

// TODO: Make this thread-safe!

static alia::context* the_alia_context = 0;

static alia::rgba8 parse_color_value(lua_State* L, char const* value)
{
    uint8 digits[8];
    bool has_alpha;
    for (int i = 0; ; ++i)
    {
        char c = value[i];
        if (c == '\0')
        {
            switch (i)
            {
             case 6:
                has_alpha = false;
                break;
             case 8:
                has_alpha = true;
                break;
             default:
                luaL_error(L, "invalid hex color constant");
            }
            break;
        }
        else if (i >= 8)
            luaL_error(L, "invalid hex color constant");
        else if (c >= '0' && c <= '9')
            digits[i] = uint8(c - '0');
        else if (c >= 'a' && c <= 'f')
            digits[i] = 10 + uint8(c - 'a');
        else if (c >= 'A' && c <= 'F')
            digits[i] = 10 + uint8(c - 'A');
        else
            luaL_error(L, "invalid hex color constant");
    }

    alia::rgba8 color;
    color.r = (digits[0] << 4) + digits[1];
    color.g = (digits[2] << 4) + digits[3];
    color.b = (digits[4] << 4) + digits[5];
    color.a = has_alpha ? ((digits[6] << 4) + digits[7]) : 0xff;
    return color;
}

static int set_style(lua_State* L)
{
    if (!the_alia_context)
        luaL_error(L, "no alia context set");

    if (lua_gettop(L) != 2)
        luaL_error(L, "usage: set_style(style, properties)");

    char const* style = luaL_checkstring(L, 1);

    luaL_checktype(L, 2, LUA_TTABLE);

    alia::style_property_map properties;

    // Iterate through the key/value pairs in the table.
    lua_pushnil(L); // first key
    while (lua_next(L, 2))
    {
        // Now the key is at index -2 and the value is at index -1.
        char const* key = luaL_checkstring(L, -2);
        if (lua_isboolean(L, -1))
            properties[key] = lua_toboolean(L, -1) != 0 ? true : false;
        else if (lua_isnumber(L, -1))
            properties[key] = double(lua_tonumber(L, -1));
        else if (lua_isstring(L, -1))
        {
            char const* value = lua_tostring(L, -1);
            if (value[0] == '#')
                properties[key] = parse_color_value(L, value + 1);
            else
                properties[key] = std::string(value);
        }
        else
            luaL_error(L, "invalid property value type");
        // Pop the value.
        lua_pop(L, 1);
        // The key is replaced by the next call to lua_next().
    }

    the_alia_context->style_tree->set_style(style, properties);

    return 0;
}

static luaL_Reg the_api[] =
{
    { "style", &set_style },
    { 0, 0 }
};

void read_lua_style_file(alia::context& ctx, std::string const& path)
{
    lua_State *L = lua_open();
    luaL_register(L, "alia", the_api);
    the_alia_context = &ctx;
    bool failed = false;
    std::string error_msg;
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
