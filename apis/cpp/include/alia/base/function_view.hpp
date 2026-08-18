#pragma once

#include <type_traits>
#include <utility>

namespace alia {

// `function_view` is the non-owning equivalent of `std::function`.
template<class Signature>
struct function_view;
template<class Return, class... Args>
struct function_view<Return(Args...)>
{
 private:
    void* ptr_;
    Return (*erased_fn_)(void*, Args...);

 public:
    template<class T>
    function_view(T&& x) noexcept : ptr_{(void*) std::addressof(x)}
    {
        erased_fn_ = [](void* ptr, Args... xs) -> Return {
            return (*reinterpret_cast<std::add_pointer_t<T>>(ptr))(
                std::forward<Args>(xs)...);
        };
    }

    decltype(auto)
    operator()(Args... xs) const
        noexcept(noexcept(erased_fn_(ptr_, std::forward<Args>(xs)...)))
    {
        return erased_fn_(ptr_, std::forward<Args>(xs)...);
    }
};

} // namespace alia
