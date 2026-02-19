#ifndef ENTT_CORE_TUPLE_HPP
#define ENTT_CORE_TUPLE_HPP

#include <tuple>
#include <type_traits>
#include <utility>

namespace entt {


template<typename Type>
struct is_tuple: std::false_type {};


template<typename... Args>
struct is_tuple<std::tuple<Args...>>: std::true_type {};


template<typename Type>
inline constexpr bool is_tuple_v = is_tuple<Type>::value;


template<typename Type>
constexpr decltype(auto) unwrap_tuple(Type &&value) noexcept {
    if constexpr(std::tuple_size_v<std::remove_reference_t<Type>> == 1u) {
        return std::get<0>(std::forward<Type>(value));
    } else {
        return std::forward<Type>(value);
    }
}


template<typename Func>
struct forward_apply: private Func {
    
    template<typename... Args>
    constexpr forward_apply(Args &&...args) noexcept(std::is_nothrow_constructible_v<Func, Args...>)
        : Func{std::forward<Args>(args)...} {}

    
    template<typename Type>
    constexpr decltype(auto) operator()(Type &&args) noexcept(noexcept(std::apply(std::declval<Func &>(), args))) {
        return std::apply(static_cast<Func &>(*this), std::forward<Type>(args));
    }

    
    template<typename Type>
    constexpr decltype(auto) operator()(Type &&args) const noexcept(noexcept(std::apply(std::declval<const Func &>(), args))) {
        return std::apply(static_cast<const Func &>(*this), std::forward<Type>(args));
    }
};


template<typename Func>
forward_apply(Func) -> forward_apply<std::remove_reference_t<std::remove_const_t<Func>>>;

} 

#endif
