#ifndef ALIA_FLOW_OBJECT_TREES_HPP
#define ALIA_FLOW_OBJECT_TREES_HPP

namespace alia {

template<class Object>
struct tree_object
{
    virtual void
    relocate(Object& parent, Object* after)
        = 0;

    virtual void
    truncate(Object* after)
        = 0;

    Object** prev_ = nullptr;
    Object* next_ = nullptr;
    Object* children_ = nullptr;
};

template<class Object>
struct tree_traversal
{
    Object* last_sibling = nullptr;
    Object** next_ptr = nullptr;
    Object* active_parent = nullptr;
};

template<class Object>
void
check_for_movement(tree_traversal<Object>& traversal, Object& object)
{
    Object* expected_object = *traversal.next_ptr;
    if (expected_object != &object)
    {
        object.relocate(*traversal.active_parent, traversal.last_sibling);
        *traversal.next_ptr = &object;
        object.next_ = expected_object;
    }
}

template<class Object>
void
prepare_for_next_object(tree_traversal<Object>& traversal, Object& object)
{
    traversal.last_sibling = &object;
    traversal.next_ptr = &object.next_;
}

template<class Object>
void
refresh_tree_object(tree_traversal<Object>& traversal, Object& object)
{
    check_for_movement(traversal, object);
    prepare_for_next_object(traversal, object);
}

template<class Object>
void
activate_parent_object(tree_traversal<Object>& traversal, Object& object)
{
    traversal.active_parent = &object;
    traversal.last_sibling = nullptr;
    traversal.next_ptr = &object.children_;
}

template<class Object>
void
finish_sibling_list(tree_traversal<Object>& traversal)
{
    if (*traversal.next_ptr)
    {
        traversal.active_parent->truncate(traversal.last_sibling);
        *traversal.next_ptr = nullptr;
    }
}

template<class Object>
struct scoped_tree_object
{
    scoped_tree_object() : traversal_(nullptr)
    {
    }
    scoped_tree_object(tree_traversal<Object>& traversal, Object& object)
    {
        begin(traversal, object);
    }
    ~scoped_tree_object()
    {
        end();
    }

    void
    begin(tree_traversal<Object>& traversal, Object& object)
    {
        traversal_ = &traversal;
        object_ = &object;
        parent_ = traversal.active_parent;
        check_for_insertion(traversal, object);
        activate_parent_object(traversal, object);
    }

    void
    end()
    {
        if (traversal_)
        {
            finish_sibling_list(*traversal_);
            traversal_->active_parent = parent_;
            prepare_for_next_object(*traversal_, *object_);
            traversal_ = nullptr;
        }
    }

 private:
    tree_traversal<Object>* traversal_;
    Object* parent_;
    Object* object_;
};

template<class Object, class Content>
void
traverse_object_tree(
    tree_traversal<Object>& traversal, Object& root, Content&& content)
{
    activate_parent_object(traversal, root);
    content();
    finish_sibling_list(traversal);
}

} // namespace alia

#endif
