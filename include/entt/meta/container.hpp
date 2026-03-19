

#ifndef ENTT_META_CONTAINER_HPP
#define ENTT_META_CONTAINER_HPP

#include <array>
#include <cstddef>
#include <deque>
#include <iterator>
#include <list>
#include <map>
#include <set>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "../container/dense_map.hpp"
#include "../container/dense_set.hpp"
#include "../core/type_traits.hpp"
#include "context.hpp"
#include "fwd.hpp"
#include "meta.hpp"
#include "type_traits.hpp"

namespace entt {


namespace internal {

template<typename Type, typename = void>
struct sequence_container_extent: integral_constant<meta_dynamic_extent> {};

template<typename Type>
struct sequence_container_extent<Type, std::enable_if_t<is_complete_v<std::tuple_size<Type>>>>: integral_constant<std::tuple_size_v<Type>> {};

template<typename Type>
inline constexpr std::size_t sequence_container_extent_v = sequence_container_extent<Type>::value;

template<typename, typename = void>
struct key_only_associative_container: std::true_type {};

template<typename Type>
struct key_only_associative_container<Type, std::void_t<typename Type::mapped_type>>: std::false_type {};

template<typename Type>
inline constexpr bool key_only_associative_container_v = key_only_associative_container<Type>::value;

template<typename, typename = void>
struct reserve_aware_container: std::false_type {};

template<typename Type>
struct reserve_aware_container<Type, std::void_t<decltype(&Type::reserve)>>: std::true_type {};

template<typename Type>
inline constexpr bool reserve_aware_container_v = reserve_aware_container<Type>::value;

} 



template<typename Type>
struct basic_meta_sequence_container_traits {
    static_assert(std::is_same_v<Type, std::remove_const_t<std::remove_reference_t<Type>>>, "Unexpected type");

    
    using size_type = typename meta_sequence_container::size_type;
    
    using iterator = typename meta_sequence_container::iterator;

    
    static constexpr std::size_t extent = internal::sequence_container_extent_v<Type>;
    
    [[deprecated("use ::extent instead")]] static constexpr bool fixed_size = (extent != meta_dynamic_extent);

    
    [[nodiscard]] static size_type size(const void *container) {
        return static_cast<const Type *>(container)->size();
    }

    
    [[nodiscard]] static bool clear([[maybe_unused]] void *container) {
        if constexpr(extent == meta_dynamic_extent) {
            static_cast<Type *>(container)->clear();
            return true;
        } else {
            return false;
        }
    }

    
    [[nodiscard]] static bool reserve([[maybe_unused]] void *container, [[maybe_unused]] const size_type sz) {
        if constexpr(internal::reserve_aware_container_v<Type>) {
            static_cast<Type *>(container)->reserve(sz);
            return true;
        } else {
            return false;
        }
    }

    
    [[nodiscard]] static bool resize([[maybe_unused]] void *container, [[maybe_unused]] const size_type sz) {
        if constexpr((extent == meta_dynamic_extent) && std::is_default_constructible_v<typename Type::value_type>) {
            static_cast<Type *>(container)->resize(sz);
            return true;
        } else {
            return false;
        }
    }

    
    static iterator iter(const meta_ctx &area, void *container, const void *as_const, const bool end) {
        return (container == nullptr)
                   ? iterator{area, end ? static_cast<const Type *>(as_const)->cend() : static_cast<const Type *>(as_const)->cbegin()}
                   : iterator{area, end ? static_cast<Type *>(container)->end() : static_cast<Type *>(container)->begin()};
    }

    
    [[nodiscard]] static iterator insert([[maybe_unused]] const meta_ctx &area, [[maybe_unused]] void *container, [[maybe_unused]] const void *value, [[maybe_unused]] const void *cref, [[maybe_unused]] const iterator &it) {
        if constexpr(extent == meta_dynamic_extent) {
            auto *const non_const = any_cast<typename Type::iterator>(&it.base());
            return {area, static_cast<Type *>(container)->insert(
                              non_const ? *non_const : any_cast<const typename Type::const_iterator &>(it.base()),
                              (value != nullptr) ? *static_cast<const typename Type::value_type *>(value) : *static_cast<const std::remove_reference_t<typename Type::const_reference> *>(cref))};
        } else {
            return iterator{};
        }
    }

    
    [[nodiscard]] static iterator erase([[maybe_unused]] const meta_ctx &area, [[maybe_unused]] void *container, [[maybe_unused]] const iterator &it) {
        if constexpr(extent == meta_dynamic_extent) {
            auto *const non_const = any_cast<typename Type::iterator>(&it.base());
            return {area, static_cast<Type *>(container)->erase(non_const ? *non_const : any_cast<const typename Type::const_iterator &>(it.base()))};
        } else {
            return iterator{};
        }
    }
};


template<typename Type>
struct basic_meta_associative_container_traits {
    static_assert(std::is_same_v<Type, std::remove_const_t<std::remove_reference_t<Type>>>, "Unexpected type");

    
    using size_type = typename meta_associative_container::size_type;
    
    using iterator = typename meta_associative_container::iterator;

    
    static constexpr bool key_only = internal::key_only_associative_container_v<Type>;

    
    [[nodiscard]] static size_type size(const void *container) {
        return static_cast<const Type *>(container)->size();
    }

    
    [[nodiscard]] static bool clear(void *container) {
        static_cast<Type *>(container)->clear();
        return true;
    }

    
    [[nodiscard]] static bool reserve([[maybe_unused]] void *container, [[maybe_unused]] const size_type sz) {
        if constexpr(internal::reserve_aware_container_v<Type>) {
            static_cast<Type *>(container)->reserve(sz);
            return true;
        } else {
            return false;
        }
    }

    
    static iterator iter(const meta_ctx &area, void *container, const void *as_const, const bool end) {
        return (container == nullptr)
                   ? iterator{area, std::bool_constant<key_only>{}, end ? static_cast<const Type *>(as_const)->cend() : static_cast<const Type *>(as_const)->cbegin()}
                   : iterator{area, std::bool_constant<key_only>{}, end ? static_cast<Type *>(container)->end() : static_cast<Type *>(container)->begin()};
    }

    
    [[nodiscard]] static bool insert(void *container, const void *key, [[maybe_unused]] const void *value) {
        if constexpr(key_only) {
            return static_cast<Type *>(container)->insert(*static_cast<const typename Type::key_type *>(key)).second;
        } else {
            return static_cast<Type *>(container)->emplace(*static_cast<const typename Type::key_type *>(key), *static_cast<const typename Type::mapped_type *>(value)).second;
        }
    }

    
    [[nodiscard]] static size_type erase(void *container, const void *key) {
        return static_cast<Type *>(container)->erase(*static_cast<const typename Type::key_type *>(key));
    }

    
    static iterator find(const meta_ctx &area, void *container, const void *as_const, const void *key) {
        return (container != nullptr) ? iterator{area, std::bool_constant<key_only>{}, static_cast<Type *>(container)->find(*static_cast<const typename Type::key_type *>(key))}
                                      : iterator{area, std::bool_constant<key_only>{}, static_cast<const Type *>(as_const)->find(*static_cast<const typename Type::key_type *>(key))};
    }
};


template<typename... Args>
struct meta_sequence_container_traits<std::vector<Args...>>
    : basic_meta_sequence_container_traits<std::vector<Args...>> {};


template<typename Type, auto N>
struct meta_sequence_container_traits<std::array<Type, N>>
    : basic_meta_sequence_container_traits<std::array<Type, N>> {};


template<typename... Args>
struct meta_sequence_container_traits<std::list<Args...>>
    : basic_meta_sequence_container_traits<std::list<Args...>> {};


template<typename... Args>
struct meta_sequence_container_traits<std::deque<Args...>>
    : basic_meta_sequence_container_traits<std::deque<Args...>> {};


template<typename... Args>
struct meta_associative_container_traits<std::map<Args...>>
    : basic_meta_associative_container_traits<std::map<Args...>> {};


template<typename... Args>
struct meta_associative_container_traits<std::unordered_map<Args...>>
    : basic_meta_associative_container_traits<std::unordered_map<Args...>> {};


template<typename... Args>
struct meta_associative_container_traits<std::set<Args...>>
    : basic_meta_associative_container_traits<std::set<Args...>> {};


template<typename... Args>
struct meta_associative_container_traits<std::unordered_set<Args...>>
    : basic_meta_associative_container_traits<std::unordered_set<Args...>> {};


template<typename... Args>
struct meta_associative_container_traits<dense_map<Args...>>
    : basic_meta_associative_container_traits<dense_map<Args...>> {};


template<typename... Args>
struct meta_associative_container_traits<dense_set<Args...>>
    : basic_meta_associative_container_traits<dense_set<Args...>> {};

} 

#endif
