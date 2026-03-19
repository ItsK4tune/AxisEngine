#ifndef ENTT_ENTITY_FWD_HPP
#define ENTT_ENTITY_FWD_HPP

#include <cstdint>
#include <memory>
#include <type_traits>
#include "../config/config.h"
#include "../core/fwd.hpp"
#include "../core/type_traits.hpp"

namespace entt {


enum class entity : id_type {};


enum class deletion_policy : std::uint8_t {
    
    swap_and_pop = 0u,
    
    in_place = 1u,
    
    swap_only = 2u,
    
    unspecified = swap_and_pop
};

template<typename Type, typename Entity = entity, typename = void>
struct component_traits;

template<typename Entity = entity, typename = std::allocator<Entity>>
class basic_sparse_set;

template<typename Type, typename = entity, typename = std::allocator<Type>, typename = void>
class basic_storage;

template<typename, typename>
class basic_sigh_mixin;

template<typename, typename>
class basic_reactive_mixin;

template<typename Entity = entity, typename = std::allocator<Entity>>
class basic_registry;

template<typename, typename, typename = void>
class basic_view;

template<typename Type, typename = std::allocator<Type *>>
class basic_runtime_view;

template<typename, typename, typename>
class basic_group;

template<typename>
class basic_organizer;

template<typename, typename...>
class basic_handle;

template<typename>
class basic_snapshot;

template<typename>
class basic_snapshot_loader;

template<typename>
class basic_continuous_loader;


using sparse_set = basic_sparse_set<>;


template<typename Type>
using storage = basic_storage<Type>;


template<typename Type>
using sigh_mixin = basic_sigh_mixin<Type, basic_registry<typename Type::entity_type, typename Type::base_type::allocator_type>>;


template<typename Type>
using reactive_mixin = basic_reactive_mixin<Type, basic_registry<typename Type::entity_type, typename Type::base_type::allocator_type>>;


using registry = basic_registry<>;


using organizer = basic_organizer<registry>;


using handle = basic_handle<registry>;


using const_handle = basic_handle<const registry>;


template<typename... Args>
using handle_view = basic_handle<registry, Args...>;


template<typename... Args>
using const_handle_view = basic_handle<const registry, Args...>;


using snapshot = basic_snapshot<registry>;


using snapshot_loader = basic_snapshot_loader<registry>;


using continuous_loader = basic_continuous_loader<registry>;


using runtime_view = basic_runtime_view<sparse_set>;


using const_runtime_view = basic_runtime_view<const sparse_set>;


template<typename... Type>
struct exclude_t final: type_list<Type...> {
    
    explicit constexpr exclude_t() = default;
};


template<typename... Type>
inline constexpr exclude_t<Type...> exclude{};


template<typename... Type>
struct get_t final: type_list<Type...> {
    
    explicit constexpr get_t() = default;
};


template<typename... Type>
inline constexpr get_t<Type...> get{};


template<typename... Type>
struct owned_t final: type_list<Type...> {
    
    explicit constexpr owned_t() = default;
};


template<typename... Type>
inline constexpr owned_t<Type...> owned{};


template<typename... Type, template<typename...> class Op>
struct type_list_transform<get_t<Type...>, Op> {
    
    using type = get_t<typename Op<Type>::type...>;
};


template<typename... Type, template<typename...> class Op>
struct type_list_transform<exclude_t<Type...>, Op> {
    
    using type = exclude_t<typename Op<Type>::type...>;
};


template<typename... Type, template<typename...> class Op>
struct type_list_transform<owned_t<Type...>, Op> {
    
    using type = owned_t<typename Op<Type>::type...>;
};


template<typename Type, typename Entity = entity, typename Allocator = std::allocator<Type>, typename = void>
struct storage_type {
    
    using type = ENTT_STORAGE(sigh_mixin, basic_storage<Type, Entity, Allocator>);
};


struct reactive final {};


template<typename Entity, typename Allocator>
struct storage_type<reactive, Entity, Allocator> {
    
    using type = ENTT_STORAGE(reactive_mixin, basic_storage<reactive, Entity, Allocator>);
};


template<typename... Args>
using storage_type_t = typename storage_type<Args...>::type;


template<typename Type, typename Entity = entity, typename Allocator = std::allocator<std::remove_const_t<Type>>>
struct storage_for {
    
    using type = constness_as_t<storage_type_t<std::remove_const_t<Type>, Entity, Allocator>, Type>;
};


template<typename... Args>
using storage_for_t = typename storage_for<Args...>::type;


template<typename Get, typename Exclude = exclude_t<>>
using view = basic_view<type_list_transform_t<Get, storage_for>, type_list_transform_t<Exclude, storage_for>>;


template<typename Owned, typename Get = get_t<>, typename Exclude = exclude_t<>>
using group = basic_group<type_list_transform_t<Owned, storage_for>, type_list_transform_t<Get, storage_for>, type_list_transform_t<Exclude, storage_for>>;

} 

#endif
