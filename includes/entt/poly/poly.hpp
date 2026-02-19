#ifndef ENTT_POLY_POLY_HPP
#define ENTT_POLY_POLY_HPP

#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>
#include "../core/any.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "fwd.hpp"

namespace entt {


struct poly_inspector {
    
    template<typename Type>
    operator Type &&() const;

    
    template<std::size_t Member, typename... Args>
    [[nodiscard]] poly_inspector invoke(Args &&...args) const;

    
    template<std::size_t Member, typename... Args>
    [[nodiscard]] poly_inspector invoke(Args &&...args);
};


template<typename Concept, std::size_t Len, std::size_t Align>
class poly_vtable {
    using inspector = typename Concept::template type<poly_inspector>;

    template<typename Ret, typename Clazz, typename... Args>
    static auto vtable_entry(Ret (*)(Clazz &, Args...))
        -> std::enable_if_t<std::is_base_of_v<std::remove_const_t<Clazz>, inspector>, Ret (*)(constness_as_t<basic_any<Len, Align>, Clazz> &, Args...)>;

    template<typename Ret, typename... Args>
    static auto vtable_entry(Ret (*)(Args...))
        -> Ret (*)(const basic_any<Len, Align> &, Args...);

    template<typename Ret, typename Clazz, typename... Args>
    static auto vtable_entry(Ret (Clazz::*)(Args...))
        -> std::enable_if_t<std::is_base_of_v<Clazz, inspector>, Ret (*)(basic_any<Len, Align> &, Args...)>;

    template<typename Ret, typename Clazz, typename... Args>
    static auto vtable_entry(Ret (Clazz::*)(Args...) const)
        -> std::enable_if_t<std::is_base_of_v<Clazz, inspector>, Ret (*)(const basic_any<Len, Align> &, Args...)>;

    template<auto... Candidate>
    static auto make_vtable(value_list<Candidate...>) noexcept
        -> decltype(std::make_tuple(vtable_entry(Candidate)...));

    template<typename... Func>
    [[nodiscard]] static constexpr auto make_vtable(type_list<Func...>) noexcept {
        if constexpr(sizeof...(Func) == 0u) {
            return decltype(make_vtable(typename Concept::template impl<inspector>{})){};
        } else if constexpr((std::is_function_v<Func> && ...)) {
            return decltype(std::make_tuple(vtable_entry(std::declval<Func inspector::*>())...)){};
        }
    }

    template<typename Type, auto Candidate, typename Ret, typename Any, typename... Args>
    static void fill_vtable_entry(Ret (*&entry)(Any &, Args...)) noexcept {
        if constexpr(std::is_invocable_r_v<Ret, decltype(Candidate), Args...>) {
            entry = +[](Any &, Args... args) -> Ret {
                return std::invoke(Candidate, std::forward<Args>(args)...);
            };
        } else {
            entry = +[](Any &instance, Args... args) -> Ret {
                return static_cast<Ret>(std::invoke(Candidate, any_cast<constness_as_t<Type, Any> &>(instance), std::forward<Args>(args)...));
            };
        }
    }

    template<typename Type, auto... Index>
    [[nodiscard]] static auto fill_vtable(std::index_sequence<Index...>) noexcept {
        vtable_type impl{};
        (fill_vtable_entry<Type, value_list_element_v<Index, typename Concept::template impl<Type>>>(std::get<Index>(impl)), ...);
        return impl;
    }

    using vtable_type = decltype(make_vtable(Concept{}));
    static constexpr bool is_mono = std::tuple_size_v<vtable_type> == 1u;

public:
    
    using type = std::conditional_t<is_mono, std::tuple_element_t<0u, vtable_type>, const vtable_type *>;

    
    template<typename Type>
    [[nodiscard]] static type instance() noexcept {
        static_assert(std::is_same_v<Type, std::decay_t<Type>>, "Type differs from its decayed form");
        static const vtable_type vtable = fill_vtable<Type>(std::make_index_sequence<Concept::template impl<Type>::size>{});

        if constexpr(is_mono) {
            return std::get<0>(vtable);
        } else {
            return &vtable;
        }
    }
};


template<typename Poly>
struct poly_base {
    
    template<std::size_t Member, typename... Args>
    [[nodiscard]] decltype(auto) invoke(const poly_base &self, Args &&...args) const {
        const auto &poly = static_cast<const Poly &>(self);

        if constexpr(std::is_function_v<std::remove_pointer_t<decltype(poly.vtable)>>) {
            return poly.vtable(poly.storage, std::forward<Args>(args)...);
        } else {
            return std::get<Member>(*poly.vtable)(poly.storage, std::forward<Args>(args)...);
        }
    }

    
    template<std::size_t Member, typename... Args>
    [[nodiscard]] decltype(auto) invoke(poly_base &self, Args &&...args) {
        auto &poly = static_cast<Poly &>(self);

        if constexpr(std::is_function_v<std::remove_pointer_t<decltype(poly.vtable)>>) {
            static_assert(Member == 0u, "Unknown member");
            return poly.vtable(poly.storage, std::forward<Args>(args)...);
        } else {
            return std::get<Member>(*poly.vtable)(poly.storage, std::forward<Args>(args)...);
        }
    }
};


template<std::size_t Member, typename Poly, typename... Args>
decltype(auto) poly_call(Poly &&self, Args &&...args) {
    return std::forward<Poly>(self).template invoke<Member>(self, std::forward<Args>(args)...);
}


template<typename Concept, std::size_t Len, std::size_t Align>
class basic_poly: private Concept::template type<poly_base<basic_poly<Concept, Len, Align>>> {
    friend struct poly_base<basic_poly>;

public:
    
    using concept_type = typename Concept::template type<poly_base<basic_poly>>;
    
    using vtable_type = typename poly_vtable<Concept, Len, Align>::type;

    
    basic_poly() noexcept = default;

    
    template<typename Type, typename... Args>
    explicit basic_poly(std::in_place_type_t<Type>, Args &&...args)
        : storage{std::in_place_type<Type>, std::forward<Args>(args)...},
          vtable{poly_vtable<Concept, Len, Align>::template instance<std::remove_const_t<std::remove_reference_t<Type>>>()} {}

    
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::remove_const_t<std::remove_reference_t<Type>>, basic_poly>>>
    basic_poly(Type &&value) noexcept
        : basic_poly{std::in_place_type<std::remove_const_t<std::remove_reference_t<Type>>>, std::forward<Type>(value)} {}

    
    [[nodiscard]] const type_info &info() const noexcept {
        return storage.info();
    }

    
    [[deprecated("use ::info instead")]] [[nodiscard]] const type_info &type() const noexcept {
        return info();
    }

    
    [[nodiscard]] const void *data() const noexcept {
        return storage.data();
    }

    
    [[nodiscard]] void *data() noexcept {
        return storage.data();
    }

    
    template<typename Type, typename... Args>
    void emplace(Args &&...args) {
        storage.template emplace<Type>(std::forward<Args>(args)...);
        vtable = poly_vtable<Concept, Len, Align>::template instance<std::remove_const_t<std::remove_reference_t<Type>>>();
    }

    
    void reset() {
        storage.reset();
        vtable = {};
    }

    
    [[nodiscard]] explicit operator bool() const noexcept {
        return static_cast<bool>(storage);
    }

    
    [[nodiscard]] concept_type *operator->() noexcept {
        return this;
    }

    
    [[nodiscard]] const concept_type *operator->() const noexcept {
        return this;
    }

    
    [[nodiscard]] basic_poly as_ref() noexcept {
        basic_poly ref{};
        ref.storage = storage.as_ref();
        ref.vtable = vtable;
        return ref;
    }

    
    [[nodiscard]] basic_poly as_ref() const noexcept {
        basic_poly ref{};
        ref.storage = storage.as_ref();
        ref.vtable = vtable;
        return ref;
    }

private:
    basic_any<Len, Align> storage{};
    vtable_type vtable{};
};

} 

#endif
