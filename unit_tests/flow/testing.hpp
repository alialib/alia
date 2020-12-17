#ifndef ALIA_TESTING_FLOW_TESTER_HPP
#define ALIA_TESTING_FLOW_TESTER_HPP

#include <alia/context/interface.hpp>
#include <alia/flow/data_graph.hpp>
#include <alia/flow/events.hpp>
#include <alia/flow/object_trees.hpp>
#include <alia/system/internals.hpp>

#include <sstream>

#include <testing.hpp>

using namespace alia;

// The following define a small framework for testing data traversal mechanics.
// The idea is that we execute various traversals over test data graphs
// (generally multiple times over each graph) and log various events
// (allocating new nodes, destroying old ones, and visiting existing nodes). We
// can then check that the log matches expectations.

extern std::stringstream the_log;

// Clear the log.
// It's best to do this explicitly at the beginning of each test in case the
// previous one failed and left the log in a bad state.
void
clear_log();

// Check that the log contains the expected contents and clear it.
void
check_log(std::string const& expected_contents);

struct int_object
{
    int_object() : n(-1)
    {
    }
    int_object(int n) : n(n)
    {
    }
    int_object(int_object&& other)
    {
        n = other.n;
        moved_out = false;
        other.moved_out = true;
    }
    int_object&
    operator=(int_object&& other)
    {
        n = other.n;
        moved_out = false;
        other.moved_out = true;
        return *this;
    }
    ~int_object()
    {
        if (!moved_out)
            the_log << "destructing int;";
    }
    int n;
    bool moved_out = false;
};

template<class Context>
void
do_int(Context& ctx, int n)
{
    int_object* obj;
    if (get_data(ctx, &obj))
    {
        REQUIRE(obj->n == -1);
        obj->n = n;
        the_log << "initializing int: " << n << ";";
    }
    else
    {
        REQUIRE(obj->n == n);
        the_log << "visiting int: " << n << ";";
    }
}

template<class Context>
void
do_cached_int(Context& ctx, int n)
{
    int_object* obj;
    if (get_cached_data(ctx, &obj))
    {
        REQUIRE(obj->n == -1);
        obj->n = n;
        the_log << "initializing cached int: " << n << ";";
    }
    else
    {
        REQUIRE(obj->n == n);
        the_log << "visiting cached int: " << n << ";";
    }
}

template<class Controller>
void
do_traversal(
    data_graph& graph, Controller const& controller, bool with_gc = true)
{
    alia::system sys;
    event_traversal event;

    data_traversal data;
    scoped_data_traversal sdt(graph, data);

    timing_subsystem timing;
    timing.tick_counter = 0;

    context_storage storage;
    context ctx = make_context(&storage, sys, event, data, timing);

    if (!with_gc)
        disable_gc(data);
    controller(ctx);
}

struct test_object
{
    void
    remove()
    {
        the_log << "removing " << name << "; ";

        auto& siblings = this->parent->children;
        siblings.erase(
            std::remove(siblings.begin(), siblings.end(), this),
            siblings.end());
    }

    void
    relocate(test_object& new_parent, test_object* after, test_object* before)
    {
        the_log << "relocating " << name << " into " << new_parent.name;
        if (after)
            the_log << " after " << after->name;
        the_log << "; ";

        if (this->parent)
        {
            auto& siblings = this->parent->children;
            siblings.erase(
                std::remove(siblings.begin(), siblings.end(), this),
                siblings.end());
        }

        this->parent = &new_parent;

        auto& siblings = new_parent.children;
        std::vector<test_object*>::iterator insertion_point;
        if (after)
        {
            insertion_point = siblings.insert(
                std::find(siblings.begin(), siblings.end(), after) + 1, this);
        }
        else
        {
            insertion_point = siblings.insert(siblings.begin(), this);
        }

        ++insertion_point;
        if (before)
        {
            REQUIRE(*insertion_point == before);
        }
        else
        {
            REQUIRE(insertion_point == siblings.end());
        }
    }

    void
    stream(std::ostream& out)
    {
        out << name << "(";
        for (test_object* child : children)
        {
            child->stream(out);
            out << ";";
        }
        out << ")";
    }

    std::string
    to_string()
    {
        std::stringstream out;
        this->stream(out);
        return out.str();
    }

    std::string name;
    test_object* parent = nullptr;
    std::vector<test_object*> children;
};

ALIA_DEFINE_TAGGED_TYPE(tree_traversal_tag, tree_traversal<test_object>&)

typedef alia::extend_context_type_t<alia::context, tree_traversal_tag>
    test_context;

void
do_object(test_context ctx, readable<std::string> name);

void
do_object(test_context ctx, std::string name);

template<class Contents>
void
do_container(test_context ctx, std::string name, Contents contents)
{
    scoped_tree_node<test_object> scoped;
    tree_node<test_object>* node;
    if (get_cached_data(ctx, &node))
        node->object.name = name;
    if (is_refresh_event(ctx))
        scoped.begin(get<tree_traversal_tag>(ctx), *node);
    contents(ctx);
}

template<class Contents>
void
do_piecewise_container(test_context ctx, std::string name, Contents contents)
{
    tree_node<test_object>* node;
    if (get_cached_data(ctx, &node))
        node->object.name = name;
    if (is_refresh_event(ctx))
        refresh_tree_node(get<tree_traversal_tag>(ctx), *node);
    scoped_tree_children<test_object> scoped;
    if (is_refresh_event(ctx))
        scoped.begin(get<tree_traversal_tag>(ctx), *node);
    contents(ctx);
}

#endif
