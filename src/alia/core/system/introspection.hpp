#ifndef ALIA_CORE_SYSTEM_INTROSPECTION_HPP
#define ALIA_CORE_SYSTEM_INTROSPECTION_HPP

#include <stacktrace>
#include <string>

namespace alia {

struct introspection_node
{
    std::string name;
    std::stacktrace trace;
    introspection_node* next_sibling;
    introspection_node* first_child;
};

struct introspection_traversal
{
    introspection_node** next_ptr;
};

inline void
add_introspection_node(
    introspection_traversal& traversal, introspection_node* node)
{
    *traversal.next_ptr = node;
    traversal.next_ptr = &node->next_sibling;
    *traversal.next_ptr = nullptr;
    node->first_child = nullptr;
}

struct scoped_introspection_node
{
    scoped_introspection_node(
        introspection_traversal& traversal, introspection_node* node)
    {
        traversal_ = &traversal;
        node_ = node;
        *traversal.next_ptr = node;
        traversal.next_ptr = &node->next_sibling;
        *traversal.next_ptr = nullptr;
    }

    ~scoped_introspection_node()
    {
        traversal_->next_ptr = &node_->next_sibling;
    }

 private:
    introspection_traversal* traversal_;
    introspection_node* node_;
};

} // namespace alia

#endif
