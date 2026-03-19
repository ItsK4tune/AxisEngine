#ifndef ENTT_META_UTILITY_HPP
#define ENTT_META_UTILITY_HPP

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>
#include "../core/type_traits.hpp"
#include "../locator/locator.hpp"
#include "meta.hpp"
#include "node.hpp"
#include "policy.hpp"

namespace entt {


template<typename Ret, typename Args, bool Static, bool Const>
struct meta_function_descriptor_traits {
    
    using return_type = Ret;
    
    using args_type = Args;

    
    static constexpr bool is_static = Static;
    
    static constexpr bool is_const = Const;
};


template<typename, typename>
struct meta_function_descriptor;


template<typename Type, typename Ret, typename Class, typename... Args>
struct meta_function_descriptor<Type, Ret (Class::*)(Args...) const>
    : meta_function_descriptor_traits<
          Ret,
          std::conditional_t<std::is_base_of_v<Class, Type>, type_list<Args...>, type_list<const Class &, Args...>>,
          !std::is_base_of_v<Class, Type>,
          true> {};


template<typename Type, typename Ret, typename Class, typename... Args>
struct meta_function_descriptor<Type, Ret (Class::*)(Args...)>
    : meta_function_descriptor_traits<
          Ret,
          std::conditional_t<std::is_base_of_v<Class, Type>, type_list<Args...>, type_list<Class &, Args...>>,
          !std::is_base_of_v<Class, Type>,
          false> {};


template<typename Type, typename Ret, typename Class>
struct meta_function_descriptor<Type, Ret Class::*>
    : meta_function_descriptor_traits<
          Ret &,
          std::conditional_t<std::is_base_of_v<Class, Type>, type_list<>, type_list<Class &>>,
          !std::is_base_of_v<Class, Type>,
          false> {};


template<typename Type, typename Ret, typename MaybeType, typename... Args>
struct meta_function_descriptor<Type, Ret (*)(MaybeType, Args...)>
    : meta_function_descriptor_traits<
          Ret,
          std::conditional_t<
              std::is_same_v<std::remove_const_t<std::remove_reference_t<MaybeType>>, Type> || std::is_base_of_v<std::remove_const_t<std::remove_reference_t<MaybeType>>, Type>,
              type_list<Args...>,
              type_list<MaybeType, Args...>>,
          !(std::is_same_v<std::remove_const_t<std::remove_reference_t<MaybeType>>, Type> || std::is_base_of_v<std::remove_const_t<std::remove_reference_t<MaybeType>>, Type>),
          std::is_const_v<std::remove_reference_t<MaybeType>> && (std::is_same_v<std::remove_const_t<std::remove_reference_t<MaybeType>>, Type> || std::is_base_of_v<std::remove_const_t<std::remove_reference_t<MaybeType>>, Type>)> {};


template<typename Type, typename Ret>
struct meta_function_descriptor<Type, Ret (*)()>
    : meta_function_descriptor_traits<
          Ret,
          type_list<>,
          true,
          false> {};


template<typename Type, typename Candidate>
class meta_function_helper {
    template<typename Ret, typename... Args, typename Class>
    static constexpr meta_function_descriptor<Type, Ret (Class::*)(Args...) const> get_rid_of_noexcept(Ret (Class::*)(Args...) const);

    template<typename Ret, typename... Args, typename Class>
    static constexpr meta_function_descriptor<Type, Ret (Class::*)(Args...)> get_rid_of_noexcept(Ret (Class::*)(Args...));

    template<typename Ret, typename Class, typename = std::enable_if_t<std::is_member_object_pointer_v<Ret Class::*>>>
    static constexpr meta_function_descriptor<Type, Ret Class::*> get_rid_of_noexcept(Ret Class::*);

    template<typename Ret, typename... Args>
    static constexpr meta_function_descriptor<Type, Ret (*)(Args...)> get_rid_of_noexcept(Ret (*)(Args...));

    template<typename Class>
    static constexpr meta_function_descriptor<Class, decltype(&Class::operator())> get_rid_of_noexcept(Class);

public:
    
    using type = decltype(get_rid_of_noexcept(std::declval<Candidate>()));
};


template<typename Type, typename Candidate>
using meta_function_helper_t = typename meta_function_helper<Type, Candidate>::type;


template<typename Policy = as_value_t, typename Type>
[[nodiscard]] std::enable_if_t<is_meta_policy_v<Policy>, meta_any> meta_dispatch(const meta_ctx &ctx, [[maybe_unused]] Type &&value) {
    if constexpr(std::is_same_v<Policy, as_cref_t>) {
        static_assert(std::is_lvalue_reference_v<Type>, "Invalid type");
        return meta_any{ctx, std::in_place_type<const std::remove_reference_t<Type> &>, std::as_const(value)};
    } else if constexpr(std::is_same_v<Policy, as_ref_t> || (std::is_same_v<Policy, as_is_t> && std::is_lvalue_reference_v<Type>)) {
        return meta_any{ctx, std::in_place_type<Type>, value};
    } else if constexpr(std::is_same_v<Policy, as_void_t>) {
        return meta_any{ctx, std::in_place_type<void>};
    } else {
        return meta_any{ctx, std::forward<Type>(value)};
    }
}


template<typename Policy = as_value_t, typename Type>
[[nodiscard]] std::enable_if_t<is_meta_policy_v<Policy>, meta_any> meta_dispatch(Type &&value) {
    return meta_dispatch<Policy, Type>(locator<meta_ctx>::value_or(), std::forward<Type>(value));
}


namespace internal {

template<typename Policy, typename Candidate, typename... Args>
[[nodiscard]] meta_any meta_invoke_with_args(const meta_ctx &ctx, Candidate &&candidate, Args &&...args) {
    if constexpr(std::is_void_v<decltype(std::invoke(std::forward<Candidate>(candidate), args...))>) {
        std::invoke(std::forward<Candidate>(candidate), args...);
        return meta_any{ctx, std::in_place_type<void>};
    } else {
        return meta_dispatch<Policy>(ctx, std::invoke(std::forward<Candidate>(candidate), args...));
    }
}

template<typename Type, typename Policy, typename Candidate, std::size_t... Index>
[[nodiscard]] meta_any meta_invoke(meta_any &instance, Candidate &&candidate, [[maybe_unused]] meta_any *const args, std::index_sequence<Index...>) {
    using descriptor = meta_function_helper_t<Type, std::remove_reference_t<Candidate>>;

    
    if constexpr(std::is_invocable_v<std::remove_reference_t<Candidate>, const Type &, type_list_element_t<Index, typename descriptor::args_type>...>) {
        if(const auto *const clazz = instance.try_cast<const Type>(); clazz && ((args + Index)->allow_cast<type_list_element_t<Index, typename descriptor::args_type>>() && ...)) {
            return meta_invoke_with_args<Policy>(instance.context(), std::forward<Candidate>(candidate), *clazz, (args + Index)->cast<type_list_element_t<Index, typename descriptor::args_type>>()...);
        }
    } else if constexpr(std::is_invocable_v<std::remove_reference_t<Candidate>, Type &, type_list_element_t<Index, typename descriptor::args_type>...>) {
        if(auto *const clazz = instance.try_cast<Type>(); clazz && ((args + Index)->allow_cast<type_list_element_t<Index, typename descriptor::args_type>>() && ...)) {
            return meta_invoke_with_args<Policy>(instance.context(), std::forward<Candidate>(candidate), *clazz, (args + Index)->cast<type_list_element_t<Index, typename descriptor::args_type>>()...);
        }
    } else {
        if(((args + Index)->allow_cast<type_list_element_t<Index, typename descriptor::args_type>>() && ...)) {
            return meta_invoke_with_args<Policy>(instance.context(), std::forward<Candidate>(candidate), (args + Index)->cast<type_list_element_t<Index, typename descriptor::args_type>>()...);
        }
    }
    

    return meta_any{meta_ctx_arg, instance.context()};
}

template<typename Type, typename... Args, std::size_t... Index>
[[nodiscard]] meta_any meta_construct(const meta_ctx &ctx, meta_any *const args, std::index_sequence<Index...>) {
    
    if(((args + Index)->allow_cast<Args>() && ...)) {
        return meta_any{ctx, std::in_place_type<Type>, (args + Index)->cast<Args>()...};
    }
    

    return meta_any{meta_ctx_arg, ctx};
}

} 



template<typename Type>
[[nodiscard]] meta_type meta_arg(const meta_ctx &ctx, const std::size_t index) noexcept {
    const auto &context = internal::meta_context::from(ctx);
    return {ctx, internal::meta_arg_node(context, Type{}, index)};
}


template<typename Type>
[[nodiscard]] meta_type meta_arg(const std::size_t index) noexcept {
    return meta_arg<Type>(locator<meta_ctx>::value_or(), index);
}


template<typename Type, auto Data>
[[nodiscard]] bool meta_setter([[maybe_unused]] meta_handle instance, [[maybe_unused]] meta_any value) {
    if constexpr(std::is_member_function_pointer_v<decltype(Data)> || std::is_function_v<std::remove_reference_t<std::remove_pointer_t<decltype(Data)>>>) {
        using descriptor = meta_function_helper_t<Type, decltype(Data)>;
        using data_type = type_list_element_t<descriptor::is_static, typename descriptor::args_type>;

        if(auto *const clazz = instance->try_cast<Type>(); clazz && value.allow_cast<data_type>()) {
            std::invoke(Data, *clazz, value.cast<data_type>());
            return true;
        }
    } else if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
        using data_type = std::remove_reference_t<typename meta_function_helper_t<Type, decltype(Data)>::return_type>;

        if constexpr(!std::is_array_v<data_type> && !std::is_const_v<data_type>) {
            if(auto *const clazz = instance->try_cast<Type>(); clazz && value.allow_cast<data_type>()) {
                std::invoke(Data, *clazz) = value.cast<data_type>();
                return true;
            }
        }
    } else if constexpr(std::is_pointer_v<decltype(Data)>) {
        using data_type = std::remove_reference_t<decltype(*Data)>;

        if constexpr(!std::is_array_v<data_type> && !std::is_const_v<data_type>) {
            if(value.allow_cast<data_type>()) {
                *Data = value.cast<data_type>();
                return true;
            }
        }
    }

    return false;
}


template<typename Type, auto Data, typename Policy = as_value_t>
[[nodiscard]] std::enable_if_t<is_meta_policy_v<Policy>, meta_any> meta_getter(meta_handle instance) {
    if constexpr(std::is_member_pointer_v<decltype(Data)> || std::is_function_v<std::remove_reference_t<std::remove_pointer_t<decltype(Data)>>>) {
        if constexpr(!std::is_array_v<std::remove_const_t<std::remove_reference_t<std::invoke_result_t<decltype(Data), Type &>>>>) {
            if constexpr(std::is_invocable_v<decltype(Data), Type &>) {
                if(auto *clazz = instance->try_cast<Type>(); clazz) {
                    return meta_dispatch<Policy>(instance->context(), std::invoke(Data, *clazz));
                }
            }

            if constexpr(std::is_invocable_v<decltype(Data), const Type &>) {
                if(auto *fallback = instance->try_cast<const Type>(); fallback) {
                    return meta_dispatch<Policy>(instance->context(), std::invoke(Data, *fallback));
                }
            }
        }

        return meta_any{meta_ctx_arg, instance->context()};
    } else if constexpr(std::is_pointer_v<decltype(Data)>) {
        if constexpr(std::is_array_v<std::remove_pointer_t<decltype(Data)>>) {
            return meta_any{meta_ctx_arg, instance->context()};
        } else {
            return meta_dispatch<Policy>(instance->context(), *Data);
        }
    } else {
        return meta_dispatch<Policy>(instance->context(), Data);
    }
}


template<typename Type, typename Policy = as_value_t, typename Candidate>
[[nodiscard]] std::enable_if_t<is_meta_policy_v<Policy>, meta_any> meta_invoke(meta_handle instance, Candidate &&candidate, meta_any *const args) {
    return internal::meta_invoke<Type, Policy>(*instance.operator->(), std::forward<Candidate>(candidate), args, std::make_index_sequence<meta_function_helper_t<Type, std::remove_reference_t<Candidate>>::args_type::size>{});
}


template<typename Type, auto Candidate, typename Policy = as_value_t>
[[nodiscard]] std::enable_if_t<is_meta_policy_v<Policy>, meta_any> meta_invoke(meta_handle instance, meta_any *const args) {
    return internal::meta_invoke<Type, Policy>(*instance.operator->(), Candidate, args, std::make_index_sequence<meta_function_helper_t<Type, std::remove_reference_t<decltype(Candidate)>>::args_type::size>{});
}


template<typename Type, typename... Args>
[[nodiscard]] meta_any meta_construct(const meta_ctx &ctx, meta_any *const args) {
    return internal::meta_construct<Type, Args...>(ctx, args, std::index_sequence_for<Args...>{});
}


template<typename Type, typename... Args>
[[nodiscard]] meta_any meta_construct(meta_any *const args) {
    return meta_construct<Type, Args...>(locator<meta_ctx>::value_or(), args);
}


template<typename Type, typename Policy = as_value_t, typename Candidate>
[[nodiscard]] meta_any meta_construct(const meta_ctx &ctx, Candidate &&candidate, meta_any *const args) {
    if constexpr(meta_function_helper_t<Type, Candidate>::is_static || std::is_class_v<std::remove_const_t<std::remove_reference_t<Candidate>>>) {
        meta_any placeholder{meta_ctx_arg, ctx};
        return internal::meta_invoke<Type, Policy>(placeholder, std::forward<Candidate>(candidate), args, std::make_index_sequence<meta_function_helper_t<Type, std::remove_reference_t<Candidate>>::args_type::size>{});
    } else {
        
        return internal::meta_invoke<Type, Policy>(*args, std::forward<Candidate>(candidate), args + 1u, std::make_index_sequence<meta_function_helper_t<Type, std::remove_reference_t<Candidate>>::args_type::size>{});
    }
}


template<typename Type, typename Policy = as_value_t, typename Candidate>
[[nodiscard]] std::enable_if_t<is_meta_policy_v<Policy>, meta_any> meta_construct(Candidate &&candidate, meta_any *const args) {
    return meta_construct<Type, Policy>(locator<meta_ctx>::value_or(), std::forward<Candidate>(candidate), args);
}


template<typename Type, auto Candidate, typename Policy = as_value_t>
[[nodiscard]] std::enable_if_t<is_meta_policy_v<Policy>, meta_any> meta_construct(const meta_ctx &ctx, meta_any *const args) {
    return meta_construct<Type, Policy>(ctx, Candidate, args);
}


template<typename Type, auto Candidate, typename Policy = as_value_t>
[[nodiscard]] std::enable_if_t<is_meta_policy_v<Policy>, meta_any> meta_construct(meta_any *const args) {
    return meta_construct<Type, Candidate, Policy>(locator<meta_ctx>::value_or(), args);
}

} 

#endif
