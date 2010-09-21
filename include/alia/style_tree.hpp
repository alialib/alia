#ifndef ALIA_STYLE_TREE_HPP
#define ALIA_STYLE_TREE_HPP

#include <string>
#include <alia/widget_state.hpp>
#include <boost/any.hpp>
#include <boost/unordered_map.hpp>
#include <boost/utility.hpp>

namespace alia {

typedef boost::unordered_map<std::string,boost::any> style_property_map;

struct style_node
{
    style_property_map properties;
    style_node* parent;
    boost::unordered_map<std::string,style_node> children;
};

class style_tree : boost::noncopyable
{
 public:
    style_tree();

    unsigned version() const { return version_; }
    void increment_version() { ++version_; }

    style_node const* root() const { return &root_; }

    void set_style(std::string const& path,
        style_property_map const& properties);

 private:
    style_node root_;
    unsigned version_;
};

std::string style_state_suffix(widget_state state);

style_node const* get_substyle(style_node const* style,
    std::string const& subpath,
    widget_state state = widget_states::NORMAL);

bool get_property(boost::any* result, style_node const* style,
    std::string const& subpath,
    widget_state state = widget_states::NORMAL);

template<class T>
bool get_property(T* result, style_node const* style,
    std::string const& subpath, widget_state state = widget_states::NORMAL)
{
    boost::any prop;
    if (get_property(&prop, style, subpath, state))
    {
        T* typed_prop = boost::any_cast<T>(&prop);
        if (typed_prop)
        {
            *result = *typed_prop;
            return true;
        }
    }
    return false;
}

}

#endif
