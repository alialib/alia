#ifndef ALIA_FLOW_OBJECT_TREES_HPP
#define ALIA_FLOW_OBJECT_TREES_HPP

#include <alia/common.hpp>
#include <alia/flow/events.hpp>
#include <alia/id.hpp>

namespace alia {

template<class Object>
struct tree_node : noncopyable
{
    Object object;

    ~tree_node()
    {
        if (prev_)
        {
            this->remove_from_list();
            object.remove();
        }
        if (children_)
        {
            // The parent is being destructed before the children, so just
            // wipe all their tracking pointers.
            tree_node* child = children_;
            while (child)
            {
                tree_node* next = child->next_;
                child->prev_ = nullptr;
                child->next_ = nullptr;
                child = next;
            }
        }
    }

    void
    remove_from_list()
    {
        if (next_)
            next_->prev_ = prev_;
        if (prev_)
            *prev_ = next_;
    }

    void
    insert_into_list(tree_node** prev, tree_node* next)
    {
        prev_ = prev;
        *prev_ = this;
        next_ = next;
        if (next)
            next->prev_ = &this->next_;
    }

    // prev_ points to whatever pointer anchors this node in the tree.
    // If this node is the first child amongst its siblings, then this will
    // point back to its parent's children_ pointer.
    // Otherwise, this points to the previous sibling's next_ pointer.
    tree_node** prev_ = nullptr;

    // next_ points to the next sibling in this node's sibling list (if any).
    tree_node* next_ = nullptr;

    // children_ points to this node's first child (if any).
    tree_node* children_ = nullptr;
};

template<class Object>
struct tree_traversal
{
    tree_node<Object>* last_sibling = nullptr;
    tree_node<Object>** next_ptr = nullptr;
    tree_node<Object>* active_parent = nullptr;
};

template<class Object>
void
check_for_movement(tree_traversal<Object>& traversal, tree_node<Object>& node)
{
    tree_node<Object>* expected_node = *traversal.next_ptr;
    if (expected_node != &node)
    {
        node.remove_from_list();
        node.object.relocate(
            traversal.active_parent->object,
            traversal.last_sibling ? &traversal.last_sibling->object : nullptr,
            expected_node ? &expected_node->object : nullptr);
        node.insert_into_list(traversal.next_ptr, expected_node);
    }
}

template<class Object>
void
depart_node(tree_traversal<Object>& traversal, tree_node<Object>& object)
{
    traversal.last_sibling = &object;
    traversal.next_ptr = &object.next_;
}

template<class Object>
void
refresh_tree_node(tree_traversal<Object>& traversal, tree_node<Object>& object)
{
    check_for_movement(traversal, object);
    depart_node(traversal, object);
}

template<class Object>
void
activate_parent_node(
    tree_traversal<Object>& traversal, tree_node<Object>& object)
{
    traversal.active_parent = &object;
    traversal.last_sibling = nullptr;
    traversal.next_ptr = &object.children_;
}

template<class Object>
void
cap_sibling_list(tree_traversal<Object>& traversal)
{
    if (*traversal.next_ptr)
    {
        tree_node<Object>* expected_node = *traversal.next_ptr;
        do
        {
            expected_node->object.remove();
            expected_node->remove_from_list();
            expected_node->prev_ = nullptr;
            expected_node = expected_node->next_;
        } while (expected_node);
        *traversal.next_ptr = nullptr;
    }
}

template<class Object>
struct scoped_tree_node
{
    scoped_tree_node() : traversal_(nullptr)
    {
    }
    scoped_tree_node(
        tree_traversal<Object>& traversal, tree_node<Object>& node)
    {
        begin(traversal, node);
    }
    ~scoped_tree_node()
    {
        end();
    }

    void
    begin(tree_traversal<Object>& traversal, tree_node<Object>& node)
    {
        traversal_ = &traversal;
        node_ = &node;
        parent_ = traversal.active_parent;
        check_for_movement(traversal, node);
        activate_parent_node(traversal, node);
    }

    void
    end()
    {
        if (traversal_)
        {
            cap_sibling_list(*traversal_);
            traversal_->active_parent = parent_;
            depart_node(*traversal_, *node_);
            traversal_ = nullptr;
        }
    }

 private:
    tree_traversal<Object>* traversal_;
    tree_node<Object>* parent_;
    tree_node<Object>* node_;
};

template<class Object>
struct scoped_tree_children
{
    scoped_tree_children() : traversal_(nullptr)
    {
    }
    scoped_tree_children(
        tree_traversal<Object>& traversal, tree_node<Object>& parent)
    {
        begin(traversal, parent);
    }
    ~scoped_tree_children()
    {
        end();
    }

    void
    begin(tree_traversal<Object>& traversal, tree_node<Object>& parent)
    {
        traversal_ = &traversal;
        old_traversal_state_ = traversal;
        activate_parent_node(traversal, parent);
    }

    void
    end()
    {
        if (traversal_)
        {
            cap_sibling_list(*traversal_);
            *traversal_ = old_traversal_state_;
            traversal_ = nullptr;
        }
    }

 private:
    tree_traversal<Object>* traversal_;
    tree_traversal<Object> old_traversal_state_;
};

// scoped_tree_root is used to activate an independent root node in the middle
// of a tree traversal.
template<class Object>
struct scoped_tree_root
{
    scoped_tree_root() : traversal_(nullptr)
    {
    }
    scoped_tree_root(
        tree_traversal<Object>& traversal, tree_node<Object>& new_root)
    {
        begin(traversal, new_root);
    }
    ~scoped_tree_root()
    {
        end();
    }

    void
    begin(tree_traversal<Object>& traversal, tree_node<Object>& new_root)
    {
        traversal_ = &traversal;
        old_traversal_state_ = traversal;
        activate_parent_node(traversal, new_root);
    }

    void
    end()
    {
        if (traversal_)
        {
            cap_sibling_list(*traversal_);
            *traversal_ = old_traversal_state_;
            traversal_ = nullptr;
        }
    }

 private:
    tree_traversal<Object>* traversal_;
    tree_traversal<Object> old_traversal_state_;
};

template<class Object, class Content>
void
traverse_object_tree(
    tree_traversal<Object>& traversal,
    tree_node<Object>& root,
    Content&& content)
{
    activate_parent_node(traversal, root);
    content();
    cap_sibling_list(traversal);
}

template<class Object>
struct tree_caching_data
{
    captured_id content_id;
    tree_node<Object>* subtree_head = nullptr;
    tree_node<Object>** subtree_tail = nullptr;
    tree_node<Object>* last_sibling = nullptr;
};

template<class Object>
struct scoped_tree_cacher
{
    scoped_tree_cacher() : traversal_(nullptr)
    {
    }
    scoped_tree_cacher(
        tree_traversal<Object>& traversal,
        tree_caching_data<Object>& data,
        bool content_traversal_required)
    {
        begin(traversal, data, content_traversal_required);
    }
    ~scoped_tree_cacher()
    {
        if (!exception_detector_.detect())
            end();
    }

    void
    begin(
        tree_traversal<Object>& traversal,
        tree_caching_data<Object>& data,
        bool content_traversal_required)
    {
        traversal_ = &traversal;
        data_ = &data;
        content_traversal_required_ = content_traversal_required;

        if (content_traversal_required)
        {
            // We're going to traverse the content, so record where we started
            // inserting it into the tree.
            predecessor_ = traversal.next_ptr;
        }
        else if (data.subtree_head)
        {
            // There's no need to actually traverse the content, but we do have
            // cached content, so we need to splice it in...

            // Check to see if we're inserting this where we expected.
            if (data.subtree_head->prev_ == traversal.next_ptr)
            {
                // If so, the objects are already in place, so we just need to
                // update the traversal state to skip over them.
                *traversal.next_ptr = data.subtree_head;
                traversal.next_ptr = data.subtree_tail;
                traversal.last_sibling = data.last_sibling;
            }
            else
            {
                // If not, we need to relocate all the top-level cached objects
                // to the new position in the object tree...

                // Remove the whole cached subtree from wherever it is in the
                // object tree.
                {
                    tree_node<Object>** prev = data.subtree_head->prev_;
                    tree_node<Object>* next = *data.subtree_tail;
                    if (next)
                        next->prev_ = prev;
                    if (prev)
                        *prev = next;
                }

                // Relocate all the actual objects.
                tree_node<Object>* last_sibling = traversal.last_sibling;
                {
                    tree_node<Object>* expected_node = *traversal.next_ptr;
                    Object* node_after
                        = expected_node ? &expected_node->object : nullptr;
                    tree_node<Object>* current_node = data.subtree_head;
                    tree_node<Object>** next_ptr = nullptr;
                    while (next_ptr != data.subtree_tail)
                    {
                        current_node->object.relocate(
                            traversal.active_parent->object,
                            last_sibling ? &last_sibling->object : nullptr,
                            node_after);
                        last_sibling = current_node;
                        next_ptr = &current_node->next_;
                        current_node = current_node->next_;
                    }
                }

                // Splice the subtree into its new place in the object tree.
                {
                    tree_node<Object>* expected_node = *traversal.next_ptr;

                    data.subtree_head->prev_ = traversal.next_ptr;
                    *traversal.next_ptr = data.subtree_head;

                    last_sibling->next_ = expected_node;
                    if (expected_node)
                        expected_node->prev_ = &last_sibling->next_;
                }

                // Update the traversal state.
                traversal.last_sibling = data.last_sibling;
                traversal.next_ptr = data.subtree_tail;
            }
        }
    }

    void
    end()
    {
        if (traversal_)
        {
            // If the subtree was traversed, record the head and tail of it
            // so we can splice it in on future passes.
            if (content_traversal_required_)
            {
                data_->subtree_head = *predecessor_;
                data_->subtree_tail = traversal_->next_ptr;
                data_->last_sibling = traversal_->last_sibling;
            }

            traversal_ = nullptr;
        }
    }

 private:
    tree_traversal<Object>* traversal_;
    tree_caching_data<Object>* data_;
    bool content_traversal_required_;
    tree_node<Object>** predecessor_;
    uncaught_exception_detector exception_detector_;
};

template<class Object, class Content>
auto
implement_alia_content_caching(
    context ctx,
    tree_traversal<Object>& traversal,
    bool content_traversal_required,
    Content content)
{
    tree_caching_data<Object>* data;
    get_data(ctx, &data);
    return [=, &traversal] {
        scoped_tree_cacher<Object> cacher(
            traversal, *data, content_traversal_required);
        content();
    };
}

} // namespace alia

#endif
