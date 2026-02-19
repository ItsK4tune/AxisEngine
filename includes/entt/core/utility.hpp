#ifndef ENTT_CORE_UTILITY_HPP
#define ENTT_CORE_UTILITY_HPP

#include <type_traits>
#include <utility>

namespace entt {


struct identity {
    
    using is_transparent = void;

    
    template<typename Type>
    [[nodiscard]] constexpr Type &&operator()(Type &&value) const noexcept {
        return std::forward<Type>(value);
    }
};


template<typename Type, typename Class>
[[nodiscard]] constexpr auto overload(Type Class::*member) noexcept {
    return member;
}


template<typename Func>
[[nodiscard]] constexpr auto overload(Func *func) noexcept {
    return func;
}


template<typename... Func>
struct overloaded: Func... {
    using Func::operator()...;
};


template<typename... Func>
overloaded(Func...) -> overloaded<Func...>;


template<typename Func>
struct y_combinator {
    
    constexpr y_combinator(Func recursive) noexcept(std::is_nothrow_move_constructible_v<Func>)
        : func{std::move(recursive)} {}

    
    template<typename... Args>
    constexpr decltype(auto) operator()(Args &&...args) const noexcept(std::is_nothrow_invocable_v<Func, const y_combinator &, Args...>) {
        return func(*this, std::forward<Args>(args)...);
    }

    
    template<typename... Args>
    constexpr decltype(auto) operator()(Args &&...args) noexcept(std::is_nothrow_invocable_v<Func, y_combinator &, Args...>) {
        return func(*this, std::forward<Args>(args)...);
    }

private:
    Func func;
};

} 

#endif
