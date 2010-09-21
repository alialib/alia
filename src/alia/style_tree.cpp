#include <alia/style_tree.hpp>
#include <alia/exception.hpp>

namespace alia {

style_tree::style_tree()
{
    root_.parent = 0;
    version_ = 0;
}

// This resolves a subpath within a style node.
// It finds the style that corresponds to the branch portion of the path
// (everything up to the last '/' character), and assigns it to *branch.
// The remainder of the path is stored in *leaf_name.
// If everything is successful, it returns true. Otherwise, it returns false.
// Note that the branch must already exist in the tree for this to succeed.
static bool resolve_subpath(
    style_node** branch, std::string* leaf_name,
    style_node* node, std::string const& subpath)
{
    std::size_t first_slash = subpath.find('/');
    if (first_slash == std::string::npos)
    {
        *branch = node;
        *leaf_name = subpath;
        return true;
    }

    std::string child_name = subpath.substr(0, first_slash),
        rest_of_path = subpath.substr(first_slash + 1);

    // This is a little too permissive, since it would accept paths like
    // 'a///b', but's probably fine.
    if (child_name.empty())
        return resolve_subpath(branch, leaf_name, node, rest_of_path);

    boost::unordered_map<std::string,style_node>::iterator i =
        node->children.find(child_name);
    if (i == node->children.end())
        return false;

    return resolve_subpath(branch, leaf_name, &i->second, rest_of_path);
}

void style_tree::set_style(
    std::string const& path, style_property_map const& properties)
{
    style_node* parent;
    std::string child_name;
    if (!resolve_subpath(&parent, &child_name, &root_, path))
        throw exception("invalid style path: " + path);

    if (!child_name.empty())
    {
        style_node& child = parent->children[child_name];
        child.properties = properties;
        child.parent = parent;
    }
    else
        parent->properties = properties;

    ++version_;
}

std::string style_state_suffix(widget_state state)
{
    std::string suffix;
    if ((state & widget_states::FOCUSED) != 0)
        suffix += "_focused";
    switch (state & widget_states::PRIMARY_STATE_MASK)
    {
     case widget_states::DISABLED:
        suffix += "_disabled";
        break;
     case widget_states::HOT:
        suffix += "_hot";
        break;
     case widget_states::DEPRESSED:
        suffix += "_depressed";
        break;
     case widget_states::SELECTED:
        suffix += "_selected";
        break;
     case widget_states::NORMAL:
     default:
        break;
    }
    return suffix;
}

style_node const* find_substyle(
    style_node const* style, std::string const& subpath)
{
    // Add a trailing slash so that resolve_subpath evaluates the full path
    // and considers the leaf portion to be empty.
    std::string extended_subpath = subpath + "/";

    // Start at the specified style and then search upward until the substyle
    // is found.
    // TODO: There should be another version of resolve_subpath() for const
    // access, at which point this cast can be removed.
    style_node* node = const_cast<style_node*>(style);
    while (node)
    {
        style_node* substyle;
        std::string leaf;
        if (resolve_subpath(&substyle, &leaf, node, extended_subpath))
        {
            assert(leaf.empty());
            return substyle;
        }
        node = node->parent;
    }

    // TODO: complain in development builds
    return style;
}

style_node const* find_immediate_substyle(
    style_node const* style, std::string const& subpath)
{
    // Add a trailing slash so that resolve_subpath evaluates the full path
    // and considers the leaf portion to be empty.
    std::string extended_subpath = subpath + "/";

    // TODO: const resolve_subpath()
    style_node* node = const_cast<style_node*>(style);

    style_node* substyle;
    std::string leaf;
    if (resolve_subpath(&substyle, &leaf, node, extended_subpath))
    {
        assert(leaf.empty());
        return substyle;
    }
    else
        return style;
}

bool check_for_state_substyle(
    style_node const** substyle, style_node const* style, widget_state state)
{
    std::string state_name = style_state_suffix(state);
    if (state_name.empty())
        return false;
    state_name = state_name.substr(1); // get rid of initial '_'

    style_node const* child = find_immediate_substyle(style, state_name);
    if (child != style)
    {
        *substyle = child;
        return true;
    }
    else
        return false;
}

style_node const* get_substyle(
    style_node const* style, std::string const& subpath,
    widget_state state)
{
    style_node const* branch = find_substyle(style, subpath);

    if (state != widget_states::NORMAL)
    {
        style_node const* result;
        if (check_for_state_substyle(&result, branch, state))
            return result;
        if ((state & widget_states::PRIMARY_STATE_MASK) ==
            widget_states::DISABLED)
        {
            if (check_for_state_substyle(&result, branch,
                widget_states::DISABLED))
            {
                return result;
            }
        }
        if ((state & widget_states::FOCUSED) != 0)
        {
            if (check_for_state_substyle(&result, branch,
                widget_states::FOCUSED))
            {
                return result;
            }
            if (check_for_state_substyle(&result, branch,
                state & ~widget_states::FOCUSED))
            {
                return result;
            }
        }
    }

    return branch;
}

static bool check_for_property(boost::any* result, style_node const* style,
    std::string const& name, widget_state state)
{
    std::string full_name = name + style_state_suffix(state);
    style_property_map::const_iterator i = style->properties.find(full_name);
    if (i != style->properties.end())
    {
        *result = i->second;
        return true;
    }
    return false;
}

bool get_property(boost::any* result, style_node const* style,
    std::string const& subpath, widget_state state)
{
    while (style)
    {
        style_node* substyle;
        std::string name;
        // TODO: const resolve_subpath()
        if (resolve_subpath(&substyle, &name, const_cast<style_node*>(style),
            subpath))
        {
            if (check_for_property(result, substyle, name, state))
                return true;
            if ((state & widget_states::PRIMARY_STATE_MASK) ==
                widget_states::DISABLED)
            {
                if (check_for_property(result, substyle, name,
                    widget_states::DISABLED))
                {
                    return true;
                }
            }
            if ((state & widget_states::FOCUSED) != 0)
            {
                if (check_for_property(result, substyle, name,
                    widget_states::FOCUSED))
                {
                    return true;
                }
                if (check_for_property(result, substyle, name,
                    state & ~widget_states::FOCUSED))
                {
                    return true;
                }
            }
            if (check_for_property(result, substyle, name,
                widget_states::NORMAL))
            {
                return true;
            }
        }
        style = style->parent;
    }
    return false;
}

}
