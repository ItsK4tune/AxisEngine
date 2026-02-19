#ifndef ENTT_CORE_TYPE_TRAITS_HPP
#define ENTT_CORE_TYPE_TRAITS_HPP

#include <cstddef>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "fwd.hpp"

namespace entt {


template<std::size_t N>
struct choice_t
    
    :  choice_t<N - 1> 
{};


template<>
struct choice_t<0> {};


template<std::size_t N>
inline constexpr choice_t<N> choice{};


template<typename Type>
struct type_identity {
    
    using type = Type;
};


template<typename Type>
using type_identity_t = typename type_identity<Type>::type;


template<typename Type, typename = void>
struct size_of: std::integral_constant<std::size_t, 0u> {};


template<typename Type>
struct size_of<Type, std::void_t<decltype(sizeof(Type))>>
    
    : std::integral_constant<std::size_t, sizeof(Type)> {};


template<typename Type>
inline constexpr std::size_t size_of_v = size_of<Type>::value;


template<typename Type, typename>
using unpack_as_type = Type;


template<auto Value, typename>
inline constexpr auto unpack_as_value = Value;


template<auto Value>
using integral_constant = std::integral_constant<decltype(Value), Value>;


template<id_type Value>
using tag = integral_constant<Value>;


template<typename... Type>
struct type_list {
    
    using type = type_list;
    
    static constexpr auto size = sizeof...(Type);
};


template<std::size_t, typename>
struct type_list_element;


template<std::size_t Index, typename First, typename... Other>
struct type_list_element<Index, type_list<First, Other...>>
    : type_list_element<Index - 1u, type_list<Other...>> {};


template<typename First, typename... Other>
struct type_list_element<0u, type_list<First, Other...>> {
    
    using type = First;
};


template<std::size_t Index, typename List>
using type_list_element_t = typename type_list_element<Index, List>::type;


template<typename, typename>
struct type_list_index;


template<typename Type, typename First, typename... Other>
struct type_list_index<Type, type_list<First, Other...>> {
    
    using value_type = std::size_t;
    
    static constexpr value_type value = 1u + type_list_index<Type, type_list<Other...>>::value;
};


template<typename Type, typename... Other>
struct type_list_index<Type, type_list<Type, Other...>> {
    static_assert(type_list_index<Type, type_list<Other...>>::value == sizeof...(Other), "Non-unique type");
    
    using value_type = std::size_t;
    
    static constexpr value_type value = 0u;
};


template<typename Type>
struct type_list_index<Type, type_list<>> {
    
    using value_type = std::size_t;
    
    static constexpr value_type value = 0u;
};


template<typename Type, typename List>
inline constexpr std::size_t type_list_index_v = type_list_index<Type, List>::value;


template<typename... Type, typename... Other>
constexpr type_list<Type..., Other...> operator+(type_list<Type...>, type_list<Other...>) {
    return {};
}


template<typename...>
struct type_list_cat;


template<>
struct type_list_cat<> {
    
    using type = type_list<>;
};


template<typename... Type, typename... Other, typename... List>
struct type_list_cat<type_list<Type...>, type_list<Other...>, List...> {
    
    using type = typename type_list_cat<type_list<Type..., Other...>, List...>::type;
};


template<typename... Type>
struct type_list_cat<type_list<Type...>> {
    
    using type = type_list<Type...>;
};


template<typename... List>
using type_list_cat_t = typename type_list_cat<List...>::type;


namespace internal {

template<typename...>
struct type_list_unique;

template<typename First, typename... Other, typename... Type>
struct type_list_unique<type_list<First, Other...>, Type...>
    : std::conditional_t<(std::is_same_v<First, Type> || ...), type_list_unique<type_list<Other...>, Type...>, type_list_unique<type_list<Other...>, Type..., First>> {};

template<typename... Type>
struct type_list_unique<type_list<>, Type...> {
    using type = type_list<Type...>;
};

} 



template<typename List>
struct type_list_unique {
    
    using type = typename internal::type_list_unique<List>::type;
};


template<typename List>
using type_list_unique_t = typename type_list_unique<List>::type;


template<typename List, typename Type>
struct type_list_contains;


template<typename... Type, typename Other>
struct type_list_contains<type_list<Type...>, Other>
    : std::bool_constant<(std::is_same_v<Type, Other> || ...)> {};


template<typename List, typename Type>
inline constexpr bool type_list_contains_v = type_list_contains<List, Type>::value;


template<typename...>
struct type_list_diff;


template<typename... Type, typename... Other>
struct type_list_diff<type_list<Type...>, type_list<Other...>> {
    
    using type = type_list_cat_t<std::conditional_t<type_list_contains_v<type_list<Other...>, Type>, type_list<>, type_list<Type>>...>;
};


template<typename... List>
using type_list_diff_t = typename type_list_diff<List...>::type;


template<typename, template<typename...> class>
struct type_list_transform;


template<typename... Type, template<typename...> class Op>
struct type_list_transform<type_list<Type...>, Op> {
    
    
    using type = type_list<typename Op<Type>::type...>;
};


template<typename List, template<typename...> class Op>
using type_list_transform_t = typename type_list_transform<List, Op>::type;


template<auto... Value>
struct value_list {
    
    using type = value_list;
    
    static constexpr auto size = sizeof...(Value);
};


template<std::size_t, typename>
struct value_list_element;


template<std::size_t Index, auto Value, auto... Other>
struct value_list_element<Index, value_list<Value, Other...>>
    : value_list_element<Index - 1u, value_list<Other...>> {};


template<auto Value, auto... Other>
struct value_list_element<0u, value_list<Value, Other...>> {
    
    using type = decltype(Value);
    
    static constexpr auto value = Value;
};


template<std::size_t Index, typename List>
using value_list_element_t = typename value_list_element<Index, List>::type;


template<std::size_t Index, typename List>
inline constexpr auto value_list_element_v = value_list_element<Index, List>::value;


template<auto, typename>
struct value_list_index;


template<auto Value, auto First, auto... Other>
struct value_list_index<Value, value_list<First, Other...>> {
    
    using value_type = std::size_t;
    
    static constexpr value_type value = 1u + value_list_index<Value, value_list<Other...>>::value;
};


template<auto Value, auto... Other>
struct value_list_index<Value, value_list<Value, Other...>> {
    static_assert(value_list_index<Value, value_list<Other...>>::value == sizeof...(Other), "Non-unique type");
    
    using value_type = std::size_t;
    
    static constexpr value_type value = 0u;
};


template<auto Value>
struct value_list_index<Value, value_list<>> {
    
    using value_type = std::size_t;
    
    static constexpr value_type value = 0u;
};


template<auto Value, typename List>
inline constexpr std::size_t value_list_index_v = value_list_index<Value, List>::value;


template<auto... Value, auto... Other>
constexpr value_list<Value..., Other...> operator+(value_list<Value...>, value_list<Other...>) {
    return {};
}


template<typename...>
struct value_list_cat;


template<>
struct value_list_cat<> {
    
    using type = value_list<>;
};


template<auto... Value, auto... Other, typename... List>
struct value_list_cat<value_list<Value...>, value_list<Other...>, List...> {
    
    using type = typename value_list_cat<value_list<Value..., Other...>, List...>::type;
};


template<auto... Value>
struct value_list_cat<value_list<Value...>> {
    
    using type = value_list<Value...>;
};


template<typename... List>
using value_list_cat_t = typename value_list_cat<List...>::type;


template<typename>
struct value_list_unique;


template<auto Value, auto... Other>
struct value_list_unique<value_list<Value, Other...>> {
    
    using type = std::conditional_t<
        ((Value == Other) || ...),
        typename value_list_unique<value_list<Other...>>::type,
        value_list_cat_t<value_list<Value>, typename value_list_unique<value_list<Other...>>::type>>;
};


template<>
struct value_list_unique<value_list<>> {
    
    using type = value_list<>;
};


template<typename Type>
using value_list_unique_t = typename value_list_unique<Type>::type;


template<typename List, auto Value>
struct value_list_contains;


template<auto... Value, auto Other>
struct value_list_contains<value_list<Value...>, Other>
    : std::bool_constant<((Value == Other) || ...)> {};


template<typename List, auto Value>
inline constexpr bool value_list_contains_v = value_list_contains<List, Value>::value;


template<typename...>
struct value_list_diff;


template<auto... Value, auto... Other>
struct value_list_diff<value_list<Value...>, value_list<Other...>> {
    
    using type = value_list_cat_t<std::conditional_t<value_list_contains_v<value_list<Other...>, Value>, value_list<>, value_list<Value>>...>;
};


template<typename... List>
using value_list_diff_t = typename value_list_diff<List...>::type;


template<typename, typename>
struct is_applicable: std::false_type {};


template<typename Func, template<typename...> class Tuple, typename... Args>
struct is_applicable<Func, Tuple<Args...>>: std::is_invocable<Func, Args...> {};


template<typename Func, template<typename...> class Tuple, typename... Args>
struct is_applicable<Func, const Tuple<Args...>>: std::is_invocable<Func, Args...> {};


template<typename Func, typename Args>
inline constexpr bool is_applicable_v = is_applicable<Func, Args>::value;


template<typename, typename, typename>
struct is_applicable_r: std::false_type {};


template<typename Ret, typename Func, typename... Args>
struct is_applicable_r<Ret, Func, std::tuple<Args...>>: std::is_invocable_r<Ret, Func, Args...> {};


template<typename Ret, typename Func, typename Args>
inline constexpr bool is_applicable_r_v = is_applicable_r<Ret, Func, Args>::value;


template<typename Type, typename = void>
struct is_complete: std::false_type {};


template<typename Type>
struct is_complete<Type, std::void_t<decltype(sizeof(Type))>>: std::true_type {};


template<typename Type>
inline constexpr bool is_complete_v = is_complete<Type>::value;


template<typename Type, typename = void>
struct is_iterator: std::false_type {};


namespace internal {

template<typename, typename = void>
struct has_iterator_category: std::false_type {};

template<typename Type>
struct has_iterator_category<Type, std::void_t<typename std::iterator_traits<Type>::iterator_category>>: std::true_type {};

} 



template<typename Type>
struct is_iterator<Type, std::enable_if_t<!std::is_void_v<std::remove_const_t<std::remove_pointer_t<Type>>>>>
    : internal::has_iterator_category<Type> {};


template<typename Type>
inline constexpr bool is_iterator_v = is_iterator<Type>::value;


template<typename Type>
struct is_ebco_eligible
    : std::bool_constant<std::is_empty_v<Type> && !std::is_final_v<Type>> {};


template<typename Type>
inline constexpr bool is_ebco_eligible_v = is_ebco_eligible<Type>::value;


template<typename Type, typename = void>
struct is_transparent: std::false_type {};


template<typename Type>
struct is_transparent<Type, std::void_t<typename Type::is_transparent>>: std::true_type {};


template<typename Type>
inline constexpr bool is_transparent_v = is_transparent<Type>::value;


namespace internal {

template<typename, typename = void>
struct has_tuple_size_value: std::false_type {};

template<typename Type>
struct has_tuple_size_value<Type, std::void_t<decltype(std::tuple_size<const Type>::value)>>: std::true_type {};

template<typename, typename = void>
struct has_value_type: std::false_type {};

template<typename Type>
struct has_value_type<Type, std::void_t<typename Type::value_type>>: std::true_type {};

template<typename>
[[nodiscard]] constexpr bool dispatch_is_equality_comparable();

template<typename Type, std::size_t... Index>
[[nodiscard]] constexpr bool unpack_maybe_equality_comparable(std::index_sequence<Index...>) {
    return (dispatch_is_equality_comparable<std::tuple_element_t<Index, Type>>() && ...);
}

template<typename>
[[nodiscard]] constexpr bool maybe_equality_comparable(char) {
    return false;
}

template<typename Type>
[[nodiscard]] constexpr auto maybe_equality_comparable(int) -> decltype(std::declval<Type>() == std::declval<Type>()) {
    return true;
}

template<typename Type>
[[nodiscard]] constexpr bool dispatch_is_equality_comparable() {
    
    if constexpr(std::is_array_v<Type>) {
        return false;
    } else if constexpr(is_complete_v<std::tuple_size<std::remove_const_t<Type>>>) {
        if constexpr(has_tuple_size_value<Type>::value) {
            return maybe_equality_comparable<Type>(0) && unpack_maybe_equality_comparable<Type>(std::make_index_sequence<std::tuple_size<Type>::value>{});
        } else {
            return maybe_equality_comparable<Type>(0);
        }
    } else if constexpr(has_value_type<Type>::value) {
        if constexpr(is_iterator_v<Type> || std::is_same_v<typename Type::value_type, Type> || dispatch_is_equality_comparable<typename Type::value_type>()) {
            return maybe_equality_comparable<Type>(0);
        } else {
            return false;
        }
    } else {
        return maybe_equality_comparable<Type>(0);
    }
    
}

} 



template<typename Type>
struct is_equality_comparable: std::bool_constant<internal::dispatch_is_equality_comparable<Type>()> {};


template<typename Type>
struct is_equality_comparable<const Type>: is_equality_comparable<Type> {};


template<typename Type>
inline constexpr bool is_equality_comparable_v = is_equality_comparable<Type>::value;


template<typename To, typename From>
struct constness_as {
    
    using type = std::remove_const_t<To>;
};


template<typename To, typename From>
struct constness_as<To, const From> {
    
    using type = const To;
};


template<typename To, typename From>
using constness_as_t = typename constness_as<To, From>::type;


template<typename Member>
class member_class {
    static_assert(std::is_member_pointer_v<Member>, "Invalid pointer type to non-static member object or function");

    template<typename Class, typename Ret, typename... Args>
    static Class *clazz(Ret (Class::*)(Args...));

    template<typename Class, typename Ret, typename... Args>
    static Class *clazz(Ret (Class::*)(Args...) const);

    template<typename Class, typename Type>
    static Class *clazz(Type Class::*);

public:
    
    using type = std::remove_pointer_t<decltype(clazz(std::declval<Member>()))>;
};


template<typename Member>
using member_class_t = typename member_class<Member>::type;


template<std::size_t Index, typename Candidate>
class nth_argument {
    template<typename Ret, typename... Args>
    static constexpr type_list<Args...> pick_up(Ret (*)(Args...));

    template<typename Ret, typename Class, typename... Args>
    static constexpr type_list<Args...> pick_up(Ret (Class ::*)(Args...));

    template<typename Ret, typename Class, typename... Args>
    static constexpr type_list<Args...> pick_up(Ret (Class ::*)(Args...) const);

    template<typename Type, typename Class>
    static constexpr type_list<Type> pick_up(Type Class ::*);

    template<typename Type>
    static constexpr decltype(pick_up(&Type::operator())) pick_up(Type &&);

public:
    
    using type = type_list_element_t<Index, decltype(pick_up(std::declval<Candidate>()))>;
};


template<std::size_t Index, typename Candidate>
using nth_argument_t = typename nth_argument<Index, Candidate>::type;

} 

template<typename... Type>
struct std::tuple_size<entt::type_list<Type...>>: std::integral_constant<std::size_t, entt::type_list<Type...>::size> {};

template<std::size_t Index, typename... Type>
struct std::tuple_element<Index, entt::type_list<Type...>>: entt::type_list_element<Index, entt::type_list<Type...>> {};

template<auto... Value>
struct std::tuple_size<entt::value_list<Value...>>: std::integral_constant<std::size_t, entt::value_list<Value...>::size> {};

template<std::size_t Index, auto... Value>
struct std::tuple_element<Index, entt::value_list<Value...>>: entt::value_list_element<Index, entt::value_list<Value...>> {};

#endif
