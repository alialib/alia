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

    tree_node** prev_ = nullptr;
    tree_node* next_ = nullptr;
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
            traversal.last_sibling ? &traversal.last_sibling->object : nullptr);
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
    scoped_tree_node(tree_traversal<Object>& traversal, tree_node<Object>& node)
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
    tree_node<Object>** predecessor = nullptr;
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
        id_interface const& content_id,
        bool force_update)
    {
        begin(traversal, data, content_id, force_update);
    }
    ~scoped_tree_cacher()
    {
        if (!std::uncaught_exception())
            end();
    }

    void
    begin(
        tree_traversal<Object>& traversal,
        tree_caching_data<Object>& data,
        id_interface const& content_id,
        bool force_update)
    {
        traversal_ = &traversal;
        data_ = &data;
        content_traversal_required_ = force_update;

        // If the content ID changes, we know we have to refresh the contents.
        if (!data.content_id.matches(content_id))
            content_traversal_required_ = true;

        // Check to see that we're inserting this where it's expected.
        // If not, traverse the contents. (It's not technically necessary to do
        // this. We could instead move the cached contents here, but this is
        // easier and fine for now.)
        if (data.predecessor != traversal.next_ptr)
            content_traversal_required_ = true;

        if (content_traversal_required_)
        {
            // If we're updating the contents, capture the content ID now (while
            // it's still valid) so we can store it in end().
            content_id_.capture(content_id);

            // Also record the current value of the tree traversal's next_ptr.
            predecessor_ = traversal.next_ptr;
        }
        else
        {
            // And if we're not updating the contents, just splice it in.
            *traversal.next_ptr = data.subtree_head;
            traversal.next_ptr = data.subtree_tail;
            traversal.last_sibling = data.last_sibling;
        }
    }

    void
    end()
    {
        if (traversal_)
        {
            // If the subtree was traversed, record the head and tail of it so
            // we can splice it in on future passes.
            if (content_traversal_required_)
            {
                data_->predecessor = predecessor_;
                data_->subtree_head = *predecessor_;
                data_->subtree_tail = traversal_->next_ptr;
                data_->last_sibling = traversal_->last_sibling;
                data_->content_id = std::move(content_id_);
            }

            traversal_ = nullptr;
        }
    }

    bool
    content_traversal_required() const
    {
        return content_traversal_required_;
    }

 private:
    tree_traversal<Object>* traversal_;
    tree_caching_data<Object>* data_;
    bool content_traversal_required_;
    captured_id content_id_;
    tree_node<Object>** predecessor_;
};

} // namespace alia

#endif
