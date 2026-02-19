#ifndef ENTT_META_META_HPP
#define ENTT_META_META_HPP

#include <array>
#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/any.hpp"
#include "../core/fwd.hpp"
#include "../core/iterator.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "../core/utility.hpp"
#include "../locator/locator.hpp"
#include "adl_pointer.hpp"
#include "context.hpp"
#include "fwd.hpp"
#include "node.hpp"
#include "range.hpp"
#include "type_traits.hpp"

namespace entt {

class meta_any;
class meta_type;


class meta_sequence_container {
    class meta_iterator;

public:
    
    using size_type = std::size_t;
    
    using iterator = meta_iterator;

    
    meta_sequence_container() = default;

    
    template<typename Type>
    meta_sequence_container(const meta_ctx &area, Type &instance) noexcept
        : ctx{&area},
          data{&instance},
          value_type_node{&internal::resolve<typename Type::value_type>},
          const_reference_node{&internal::resolve<std::remove_const_t<std::remove_reference_t<typename Type::const_reference>>>},
          size_fn{meta_sequence_container_traits<std::remove_const_t<Type>>::size},
          clear_fn{meta_sequence_container_traits<std::remove_const_t<Type>>::clear},
          reserve_fn{meta_sequence_container_traits<std::remove_const_t<Type>>::reserve},
          resize_fn{meta_sequence_container_traits<std::remove_const_t<Type>>::resize},
          begin_end_fn{meta_sequence_container_traits<std::remove_const_t<Type>>::iter},
          insert_fn{meta_sequence_container_traits<std::remove_const_t<Type>>::insert},
          erase_fn{meta_sequence_container_traits<std::remove_const_t<Type>>::erase},
          const_only{std::is_const_v<Type>} {}

    [[nodiscard]] inline meta_type value_type() const noexcept;
    [[nodiscard]] inline size_type size() const noexcept;
    inline bool resize(size_type);
    inline bool clear();
    inline bool reserve(size_type);
    [[nodiscard]] inline iterator begin();
    [[nodiscard]] inline iterator end();
    inline iterator insert(const iterator &, meta_any);
    inline iterator erase(const iterator &);
    [[nodiscard]] inline meta_any operator[](size_type);
    [[nodiscard]] inline explicit operator bool() const noexcept;

private:
    const meta_ctx *ctx{};
    const void *data{};
    const internal::meta_type_node &(*value_type_node)(const internal::meta_context &){};
    const internal::meta_type_node &(*const_reference_node)(const internal::meta_context &){};
    size_type (*size_fn)(const void *){};
    bool (*clear_fn)(void *){};
    bool (*reserve_fn)(void *, const size_type){};
    bool (*resize_fn)(void *, const size_type){};
    iterator (*begin_end_fn)(const meta_ctx &, void *, const void *, const bool){};
    iterator (*insert_fn)(const meta_ctx &, void *, const void *, const void *, const iterator &){};
    iterator (*erase_fn)(const meta_ctx &, void *, const iterator &){};
    bool const_only{};
};


class meta_associative_container {
    class meta_iterator;

public:
    
    using size_type = std::size_t;
    
    using iterator = meta_iterator;

    
    meta_associative_container() = default;

    
    template<typename Type>
    meta_associative_container(const meta_ctx &area, Type &instance) noexcept
        : ctx{&area},
          data{&instance},
          key_type_node{&internal::resolve<typename Type::key_type>},
          value_type_node{&internal::resolve<typename Type::value_type>},
          size_fn{&meta_associative_container_traits<std::remove_const_t<Type>>::size},
          clear_fn{&meta_associative_container_traits<std::remove_const_t<Type>>::clear},
          reserve_fn{&meta_associative_container_traits<std::remove_const_t<Type>>::reserve},
          begin_end_fn{&meta_associative_container_traits<std::remove_const_t<Type>>::iter},
          insert_fn{&meta_associative_container_traits<std::remove_const_t<Type>>::insert},
          erase_fn{&meta_associative_container_traits<std::remove_const_t<Type>>::erase},
          find_fn{&meta_associative_container_traits<std::remove_const_t<Type>>::find},
          const_only{std::is_const_v<Type>} {
        if constexpr(!meta_associative_container_traits<std::remove_const_t<Type>>::key_only) {
            mapped_type_node = &internal::resolve<typename Type::mapped_type>;
        }
    }

    [[nodiscard]] inline meta_type key_type() const noexcept;
    [[nodiscard]] inline meta_type mapped_type() const noexcept;
    [[nodiscard]] inline meta_type value_type() const noexcept;
    [[nodiscard]] inline size_type size() const noexcept;
    inline bool clear();
    inline bool reserve(size_type);
    [[nodiscard]] inline iterator begin();
    [[nodiscard]] inline iterator end();
    inline bool insert(meta_any, meta_any);
    inline size_type erase(meta_any);
    [[nodiscard]] inline iterator find(meta_any);
    [[nodiscard]] inline explicit operator bool() const noexcept;

private:
    const meta_ctx *ctx{};
    const void *data{};
    const internal::meta_type_node &(*key_type_node)(const internal::meta_context &){};
    const internal::meta_type_node &(*mapped_type_node)(const internal::meta_context &){};
    const internal::meta_type_node &(*value_type_node)(const internal::meta_context &){};
    size_type (*size_fn)(const void *){};
    bool (*clear_fn)(void *){};
    bool (*reserve_fn)(void *, const size_type){};
    iterator (*begin_end_fn)(const meta_ctx &, void *, const void *, const bool){};
    bool (*insert_fn)(void *, const void *, const void *){};
    size_type (*erase_fn)(void *, const void *){};
    iterator (*find_fn)(const meta_ctx &, void *, const void *, const void *){};
    bool const_only{};
};


class meta_any {
    using vtable_type = void(const internal::meta_traits, const meta_any &, const void *);

    template<typename Type>
    static void basic_vtable(const internal::meta_traits req, const meta_any &value, [[maybe_unused]] const void *other) {
        static_assert(std::is_same_v<std::remove_const_t<std::remove_reference_t<Type>>, Type>, "Invalid type");

        if(req == internal::meta_traits::is_none) {
            value.node = &internal::resolve<Type>(internal::meta_context::from(*value.ctx));
        }

        if constexpr(is_meta_pointer_like_v<Type>) {
            if(req == internal::meta_traits::is_pointer_like) {
                if constexpr(std::is_function_v<typename std::pointer_traits<Type>::element_type>) {
                    const_cast<meta_any &>(value).emplace<Type>(*static_cast<const Type *>(other));
                } else if constexpr(!std::is_void_v<std::remove_const_t<typename std::pointer_traits<Type>::element_type>>) {
                    using in_place_type = decltype(adl_meta_pointer_like<Type>::dereference(std::declval<const Type &>()));

                    if constexpr(std::is_constructible_v<bool, Type>) {
                        if(const auto &pointer_like = *static_cast<const Type *>(other); pointer_like) {
                            const_cast<meta_any &>(value).emplace<in_place_type>(adl_meta_pointer_like<Type>::dereference(pointer_like));
                        }
                    } else {
                        const_cast<meta_any &>(value).emplace<in_place_type>(adl_meta_pointer_like<Type>::dereference(*static_cast<const Type *>(other)));
                    }
                }
            }
        }

        if constexpr(is_complete_v<meta_sequence_container_traits<Type>> || is_complete_v<meta_associative_container_traits<Type>>) {
            if(constexpr auto flag = (is_complete_v<meta_sequence_container_traits<Type>> ? internal::meta_traits::is_sequence_container : internal::meta_traits::is_associative_container); !!(req & flag)) {
                using container_type = std::conditional_t<is_complete_v<meta_sequence_container_traits<Type>>, meta_sequence_container, meta_associative_container>;

                if(!!(req & internal::meta_traits::is_const) || (value.storage.policy() == any_policy::cref)) {
                    
                    *static_cast<container_type *>(const_cast<void *>(other)) = container_type{*value.ctx, any_cast<const Type &>(value.storage)};
                } else {
                    
                    *static_cast<container_type *>(const_cast<void *>(other)) = container_type{*value.ctx, any_cast<Type &>(const_cast<meta_any &>(value).storage)};
                }
            }
        }
    }

    [[nodiscard]] const auto &fetch_node() const {
        if(node == nullptr) {
            ENTT_ASSERT(*this, "Invalid vtable function");
            vtable(internal::meta_traits::is_none, *this, nullptr);
        }

        ENTT_ASSERT(node != nullptr, "Invalid pointer to node");
        return *node;
    }

    meta_any(const meta_any &other, any elem)
        : storage{std::move(elem)},
          ctx{other.ctx},
          node{other.node},
          vtable{other.vtable} {}

public:
    
    meta_any() = default;

    
    meta_any(meta_ctx_arg_t, const meta_ctx &area)
        : ctx{&area} {}

    
    template<typename Type, typename... Args>
    explicit meta_any(std::in_place_type_t<Type>, Args &&...args)
        : meta_any{locator<meta_ctx>::value_or(), std::in_place_type<Type>, std::forward<Args>(args)...} {}

    
    template<typename Type, typename... Args>
    explicit meta_any(const meta_ctx &area, std::in_place_type_t<Type>, Args &&...args)
        : storage{std::in_place_type<Type>, std::forward<Args>(args)...},
          ctx{&area},
          vtable{&basic_vtable<std::remove_const_t<std::remove_reference_t<Type>>>} {}

    
    template<typename Type>
    explicit meta_any(std::in_place_t, Type *value)
        : meta_any{locator<meta_ctx>::value_or(), std::in_place, value} {}

    
    template<typename Type>
    explicit meta_any(const meta_ctx &area, std::in_place_t, Type *value)
        : storage{std::in_place, value},
          ctx{&area},
          vtable{storage ? &basic_vtable<Type> : nullptr} {
    }

    
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Type>, meta_any>>>
    meta_any(Type &&value)
        : meta_any{locator<meta_ctx>::value_or(), std::forward<Type>(value)} {}

    
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Type>, meta_any>>>
    meta_any(const meta_ctx &area, Type &&value)
        : meta_any{area, std::in_place_type<std::decay_t<Type>>, std::forward<Type>(value)} {}

    
    meta_any(const meta_ctx &area, const meta_any &other)
        : storage{other.storage},
          ctx{&area},
          node{(ctx == other.ctx) ? other.node : nullptr},
          vtable{other.vtable} {}

    
    meta_any(const meta_ctx &area, meta_any &&other)
        : storage{std::move(other.storage)},
          ctx{&area},
          node{(ctx == other.ctx) ? std::exchange(other.node, nullptr) : nullptr},
          vtable{std::exchange(other.vtable, nullptr)} {}

    
    meta_any(const meta_any &other) = default;

    
    meta_any(meta_any &&other) noexcept
        : storage{std::move(other.storage)},
          ctx{other.ctx},
          node{std::exchange(other.node, nullptr)},
          vtable{std::exchange(other.vtable, nullptr)} {}

    
    ~meta_any() = default;

    
    meta_any &operator=(const meta_any &other) {
        if(this != &other) {
            storage = other.storage;
            ctx = other.ctx;
            node = other.node;
            vtable = other.vtable;
        }

        return *this;
    }

    
    meta_any &operator=(meta_any &&other) noexcept {
        storage = std::move(other.storage);
        ctx = other.ctx;
        node = std::exchange(other.node, nullptr);
        vtable = std::exchange(other.vtable, nullptr);
        return *this;
    }

    
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Type>, meta_any>>>
    meta_any &operator=(Type &&value) {
        emplace<std::decay_t<Type>>(std::forward<Type>(value));
        return *this;
    }

    
    [[nodiscard]] inline meta_type type() const noexcept;

    
    template<typename... Args>
    meta_any invoke(id_type id, Args &&...args) const;

    
    template<typename... Args>
    meta_any invoke(id_type id, Args &&...args);

    
    template<typename Type>
    bool set(id_type id, Type &&value);

    
    [[nodiscard]] meta_any get(id_type id) const;

    
    [[nodiscard]] meta_any get(id_type id);

    
    template<typename Type>
    [[nodiscard]] const Type *try_cast() const {
        const auto *elem = any_cast<const Type>(&storage);
        return ((elem != nullptr) || !*this) ? elem : static_cast<const Type *>(internal::try_cast(internal::meta_context::from(*ctx), fetch_node(), type_hash<std::remove_const_t<Type>>::value(), storage.data()));
    }

    
    template<typename Type>
    [[nodiscard]] Type *try_cast() {
        return ((storage.policy() == any_policy::cref) && !std::is_const_v<Type>) ? nullptr : const_cast<Type *>(std::as_const(*this).try_cast<std::remove_const_t<Type>>());
    }

    
    template<typename Type>
    [[nodiscard]] std::remove_const_t<Type> cast() const {
        auto *const instance = try_cast<std::remove_reference_t<Type>>();
        ENTT_ASSERT(instance, "Invalid instance");
        return static_cast<Type>(*instance);
    }

    
    template<typename Type>
    [[nodiscard]] std::remove_const_t<Type> cast() {
        
        auto *const instance = try_cast<std::remove_reference_t<const Type>>();
        ENTT_ASSERT(instance, "Invalid instance");
        return static_cast<Type>(*instance);
    }

    
    [[nodiscard]] meta_any allow_cast(const meta_type &type) const;

    
    [[nodiscard]] bool allow_cast(const meta_type &type);

    
    template<typename Type>
    [[nodiscard]] meta_any allow_cast() const {
        if constexpr(!std::is_reference_v<Type> || std::is_const_v<std::remove_reference_t<Type>>) {
            if(storage.has_value<std::remove_const_t<std::remove_reference_t<Type>>>()) {
                return as_ref();
            } else if(*this) {
                if constexpr(std::is_arithmetic_v<std::remove_const_t<std::remove_reference_t<Type>>> || std::is_enum_v<std::remove_const_t<std::remove_reference_t<Type>>>) {
                    if(const auto &from = fetch_node(); from.conversion_helper) {
                        return meta_any{*ctx, static_cast<Type>(from.conversion_helper(nullptr, storage.data()))};
                    }
                }

                if(const auto &from = fetch_node(); from.details != nullptr) {
                    if(const auto *elem = internal::find_member<&internal::meta_conv_node::type>(from.details->conv, entt::type_hash<std::remove_const_t<std::remove_reference_t<Type>>>::value()); elem != nullptr) {
                        return elem->conv(*ctx, storage.data());
                    }

                    for(auto &&curr: from.details->base) {
                        if(auto other = curr.resolve(internal::meta_context::from(*ctx)).from_void(*ctx, nullptr, curr.cast(storage.data())); curr.type == entt::type_hash<std::remove_const_t<std::remove_reference_t<Type>>>::value()) {
                            return other;
                        } else if(auto from_base = std::as_const(other).template allow_cast<Type>(); from_base) {
                            return from_base;
                        }
                    }
                }
            }
        }

        return meta_any{meta_ctx_arg, *ctx};
    }

    
    template<typename Type>
    [[nodiscard]] bool allow_cast() {
        if constexpr(std::is_reference_v<Type> && !std::is_const_v<std::remove_reference_t<Type>>) {
            return allow_cast<const std::remove_reference_t<Type> &>() && (storage.policy() != any_policy::cref);
        } else {
            if(storage.has_value<std::remove_const_t<std::remove_reference_t<Type>>>()) {
                return true;
            } else if(auto other = std::as_const(*this).allow_cast<std::remove_const_t<std::remove_reference_t<Type>>>(); other) {
                if(other.storage.owner()) {
                    std::swap(*this, other);
                }

                return true;
            }

            return false;
        }
    }

    
    template<typename Type, typename... Args>
    void emplace(Args &&...args) {
        storage.emplace<Type>(std::forward<Args>(args)...);
        auto *prev = std::exchange(vtable, &basic_vtable<std::remove_const_t<std::remove_reference_t<Type>>>);
        node = (prev == vtable) ? node : nullptr;
    }

    
    bool assign(const meta_any &other);

    
    bool assign(meta_any &&other);

    
    void reset() {
        storage.reset();
        node = nullptr;
        vtable = nullptr;
    }

    
    [[nodiscard]] meta_sequence_container as_sequence_container() noexcept {
        meta_sequence_container proxy{};
        if(*this) { vtable(internal::meta_traits::is_sequence_container, *this, &proxy); }
        return proxy;
    }

    
    [[nodiscard]] meta_sequence_container as_sequence_container() const noexcept {
        meta_sequence_container proxy{};
        if(*this) { vtable(internal::meta_traits::is_sequence_container | internal::meta_traits::is_const, *this, &proxy); }
        return proxy;
    }

    
    [[nodiscard]] meta_associative_container as_associative_container() noexcept {
        meta_associative_container proxy{};
        if(*this) { vtable(internal::meta_traits::is_associative_container, *this, &proxy); }
        return proxy;
    }

    
    [[nodiscard]] meta_associative_container as_associative_container() const noexcept {
        meta_associative_container proxy{};
        if(*this) { vtable(internal::meta_traits::is_associative_container | internal::meta_traits::is_const, *this, &proxy); }
        return proxy;
    }

    
    [[nodiscard]] meta_any operator*() const noexcept {
        meta_any ret{meta_ctx_arg, *ctx};
        if(*this) { vtable(internal::meta_traits::is_pointer_like, ret, storage.data()); }
        return ret;
    }

    
    [[nodiscard]] explicit operator bool() const noexcept {
        return !(vtable == nullptr);
    }

    
    [[nodiscard]] bool operator==(const meta_any &other) const noexcept {
        return (ctx == other.ctx) && (!*this == !other) && (storage == other.storage);
    }

    
    [[nodiscard]] bool operator!=(const meta_any &other) const noexcept {
        return !(*this == other);
    }

    
    [[nodiscard]] meta_any as_ref() noexcept {
        return meta_any{*this, storage.as_ref()};
    }

    
    [[nodiscard]] meta_any as_ref() const noexcept {
        return meta_any{*this, storage.as_ref()};
    }

    
    [[nodiscard]] const any &base() const noexcept {
        return storage;
    }

    
    [[nodiscard]] const meta_ctx &context() const noexcept {
        return *ctx;
    }

private:
    any storage{};
    const meta_ctx *ctx{&locator<meta_ctx>::value_or()};
    mutable const internal::meta_type_node *node{};
    vtable_type *vtable{};
};


template<typename Type>
[[nodiscard]] meta_any forward_as_meta(const meta_ctx &ctx, Type &&value) {
    return meta_any{ctx, std::in_place_type<Type &&>, std::forward<Type>(value)};
}


template<typename Type>
[[nodiscard]] meta_any forward_as_meta(Type &&value) {
    return forward_as_meta(locator<meta_ctx>::value_or(), std::forward<Type>(value));
}


class meta_handle {
    template<typename Type, typename... Args, typename = std::enable_if_t<std::is_same_v<std::decay_t<Type>, meta_any>>>
    meta_handle(int, Type &value, Args &&...args)
        : any{std::forward<Args>(args)..., value.as_ref()} {}

    template<typename Type, typename... Args>
    meta_handle(char, Type &value, Args &&...args)
        : any{std::forward<Args>(args)..., std::in_place_type<Type &>, value} {}

public:
    
    meta_handle() = default;

    
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Type>, meta_handle>>>
    meta_handle(const meta_ctx &ctx, Type &value)
        : meta_handle{0, value, ctx} {}

    
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Type>, meta_handle>>>
    meta_handle(Type &value)
        : meta_handle{0, value} {}

    
    meta_handle(const meta_ctx &area, meta_handle &&other)
        : any{area, std::move(other.any)} {}

    
    meta_handle(const meta_handle &) = delete;

    
    meta_handle(meta_handle &&) = default;

    
    ~meta_handle() = default;

    
    meta_handle &operator=(const meta_handle &) = delete;

    
    meta_handle &operator=(meta_handle &&) = default;

    
    [[nodiscard]] explicit operator bool() const noexcept {
        return static_cast<bool>(any);
    }

    
    [[nodiscard]] meta_any *operator->() {
        return &any;
    }

    
    [[deprecated("do not use const handles")]] [[nodiscard]] const meta_any *operator->() const {
        return &any;
    }

private:
    meta_any any{};
};


struct meta_custom {
    
    meta_custom() noexcept = default;

    
    meta_custom(const internal::meta_custom_node &curr) noexcept
        : node{&curr} {}

    
    template<typename Type>
    [[nodiscard]] operator Type *() const noexcept {
        return ((node != nullptr) && (type_hash<std::remove_const_t<Type>>::value() == node->type)) ? static_cast<Type *>(node->value.get()) : nullptr;
    }

    
    template<typename Type>
    [[nodiscard]] operator Type &() const noexcept {
        ENTT_ASSERT(static_cast<Type *>(*this) != nullptr, "Invalid type");
        return *static_cast<Type *>(node->value.get());
    }

private:
    const internal::meta_custom_node *node{};
};


class meta_data {
    [[nodiscard]] auto &node_or_assert() const noexcept {
        ENTT_ASSERT(node != nullptr, "Invalid pointer to node");
        return *node;
    }

public:
    
    using size_type = typename internal::meta_data_node::size_type;

    
    meta_data() noexcept = default;

    
    meta_data(const meta_ctx &area, const internal::meta_data_node &curr) noexcept
        : node{&curr},
          ctx{&area} {}

    
    [[nodiscard]] const char *name() const noexcept {
        return node_or_assert().name;
    }

    
    [[nodiscard]] size_type arity() const noexcept {
        return node_or_assert().arity;
    }

    
    [[nodiscard]] bool is_const() const noexcept {
        return !!(node_or_assert().traits & internal::meta_traits::is_const);
    }

    
    [[nodiscard]] bool is_static() const noexcept {
        return !!(node_or_assert().traits & internal::meta_traits::is_static);
    }

    
    [[nodiscard]] inline meta_type type() const noexcept;

    
    template<typename Instance = meta_handle, typename Type>
    
    bool set(Instance &&instance, Type &&value) const {
        return node_or_assert().set(meta_handle{*ctx, std::forward<Instance>(instance)}, meta_any{*ctx, std::forward<Type>(value)});
    }

    
    template<typename Instance = meta_handle>
    [[nodiscard]] meta_any get(Instance &&instance) const {
        return node_or_assert().get(meta_handle{*ctx, std::forward<Instance>(instance)});
    }

    
    [[nodiscard]] inline meta_type arg(size_type index) const noexcept;

    
    template<typename Type>
    [[nodiscard]] Type traits() const noexcept {
        return internal::meta_to_user_traits<Type>(node_or_assert().traits);
    }

    
    [[nodiscard]] meta_custom custom() const noexcept {
        return {node_or_assert().custom};
    }

    
    [[nodiscard]] explicit operator bool() const noexcept {
        return (node != nullptr);
    }

    
    [[nodiscard]] bool operator==(const meta_data &other) const noexcept {
        return (ctx == other.ctx) && (node == other.node);
    }

private:
    const internal::meta_data_node *node{};
    const meta_ctx *ctx{&locator<meta_ctx>::value_or()};
};


[[nodiscard]] inline bool operator!=(const meta_data &lhs, const meta_data &rhs) noexcept {
    return !(lhs == rhs);
}


class meta_func {
    [[nodiscard]] auto &node_or_assert() const noexcept {
        ENTT_ASSERT(node != nullptr, "Invalid pointer to node");
        return *node;
    }

public:
    
    using size_type = typename internal::meta_func_node::size_type;

    
    meta_func() noexcept = default;

    
    meta_func(const meta_ctx &area, const internal::meta_func_node &curr) noexcept
        : node{&curr},
          ctx{&area} {}

    
    [[nodiscard]] const char *name() const noexcept {
        return node_or_assert().name;
    }

    
    [[nodiscard]] size_type arity() const noexcept {
        return node_or_assert().arity;
    }

    
    [[nodiscard]] bool is_const() const noexcept {
        return !!(node_or_assert().traits & internal::meta_traits::is_const);
    }

    
    [[nodiscard]] bool is_static() const noexcept {
        return !!(node_or_assert().traits & internal::meta_traits::is_static);
    }

    
    [[nodiscard]] inline meta_type ret() const noexcept;

    
    [[nodiscard]] inline meta_type arg(size_type index) const noexcept;

    
    template<typename Instance = meta_handle>
    meta_any invoke(Instance &&instance, meta_any *const args, const size_type sz) const {
        return (sz == arity()) ? node_or_assert().invoke(meta_handle{*ctx, std::forward<Instance>(instance)}, args) : meta_any{meta_ctx_arg, *ctx};
    }

    
    template<typename Instance = meta_handle, typename... Args>
    
    meta_any invoke(Instance &&instance, Args &&...args) const {
        return invoke(std::forward<Instance>(instance), std::array<meta_any, sizeof...(Args)>{meta_any{*ctx, std::forward<Args>(args)}...}.data(), sizeof...(Args));
    }

    
    template<typename Type>
    [[nodiscard]] Type traits() const noexcept {
        return internal::meta_to_user_traits<Type>(node_or_assert().traits);
    }

    
    [[nodiscard]] meta_custom custom() const noexcept {
        return {node_or_assert().custom};
    }

    
    [[nodiscard]] meta_func next() const {
        return (node_or_assert().next != nullptr) ? meta_func{*ctx, *node_or_assert().next} : meta_func{};
    }

    
    [[nodiscard]] explicit operator bool() const noexcept {
        return (node != nullptr);
    }

    
    [[nodiscard]] bool operator==(const meta_func &other) const noexcept {
        return (ctx == other.ctx) && (node == other.node);
    }

private:
    const internal::meta_func_node *node{};
    const meta_ctx *ctx{&locator<meta_ctx>::value_or()};
};


[[nodiscard]] inline bool operator!=(const meta_func &lhs, const meta_func &rhs) noexcept {
    return !(lhs == rhs);
}


class meta_type {
    [[nodiscard]] const auto &fetch_node() const {
        return (node == nullptr) ? internal::resolve<void>(internal::meta_context::from(*ctx)) : *node;
    }

    template<typename Func>
    [[nodiscard]] auto lookup(meta_any *const args, const typename internal::meta_type_node::size_type sz, [[maybe_unused]] bool constness, Func next) const {
        decltype(next()) candidate = nullptr;
        size_type same{};
        bool ambiguous{};

        for(auto curr = next(); curr; curr = next()) {
            if constexpr(std::is_same_v<std::decay_t<decltype(*curr)>, internal::meta_func_node>) {
                if(constness && !(curr->traits & internal::meta_traits::is_const)) {
                    continue;
                }
            }

            if(curr->arity == sz) {
                size_type match{};
                size_type pos{};

                
                for(; pos < sz && args[pos]; ++pos) {
                    const auto other = curr->arg(*ctx, pos);
                    const auto type = args[pos].type();

                    if(const auto &info = other.info(); info == type.info()) {
                        ++match;
                    } else if(!(type.fetch_node().conversion_helper && other.fetch_node().conversion_helper) && !(type.fetch_node().details && (internal::find_member<&internal::meta_base_node::type>(type.fetch_node().details->base, info.hash()) || internal::find_member<&internal::meta_conv_node::type>(type.fetch_node().details->conv, info.hash())))) {
                        break;
                    }
                }
                

                if(pos == sz) {
                    if(!candidate || match > same) {
                        candidate = curr;
                        same = match;
                        ambiguous = false;
                    } else if(match == same) {
                        if constexpr(std::is_same_v<std::decay_t<decltype(*curr)>, internal::meta_func_node>) {
                            if(!!(curr->traits & internal::meta_traits::is_const) != !!(candidate->traits & internal::meta_traits::is_const)) {
                                candidate = !!(candidate->traits & internal::meta_traits::is_const) ? curr : candidate;
                                ambiguous = false;
                                continue;
                            }
                        }

                        ambiguous = true;
                    }
                }
            }
        }

        return ambiguous ? nullptr : candidate;
    }

public:
    
    using size_type = typename internal::meta_type_node::size_type;

    
    meta_type() noexcept = default;

    
    meta_type(const meta_ctx &area, const internal::meta_type_node &curr) noexcept
        : node{&curr},
          ctx{&area} {}

    
    meta_type(const meta_ctx &area, const internal::meta_base_node &curr) noexcept
        : meta_type{area, curr.resolve(internal::meta_context::from(area))} {}

    
    [[nodiscard]] const type_info &info() const noexcept {
        return *fetch_node().info;
    }

    
    [[nodiscard]] id_type id() const noexcept {
        return fetch_node().id;
    }

    
    [[nodiscard]] const char *name() const noexcept {
        return fetch_node().name;
    }

    
    [[nodiscard]] size_type size_of() const noexcept {
        return fetch_node().size_of;
    }

    
    [[nodiscard]] bool is_arithmetic() const noexcept {
        return !!(fetch_node().traits & internal::meta_traits::is_arithmetic);
    }

    
    [[nodiscard]] bool is_integral() const noexcept {
        return !!(fetch_node().traits & internal::meta_traits::is_integral);
    }

    
    [[nodiscard]] bool is_signed() const noexcept {
        return !!(fetch_node().traits & internal::meta_traits::is_signed);
    }

    
    [[nodiscard]] bool is_array() const noexcept {
        return !!(fetch_node().traits & internal::meta_traits::is_array);
    }

    
    [[nodiscard]] bool is_enum() const noexcept {
        return !!(fetch_node().traits & internal::meta_traits::is_enum);
    }

    
    [[nodiscard]] bool is_class() const noexcept {
        return !!(fetch_node().traits & internal::meta_traits::is_class);
    }

    
    [[nodiscard]] bool is_pointer() const noexcept {
        return !!(fetch_node().traits & internal::meta_traits::is_pointer);
    }

    
    [[nodiscard]] meta_type remove_pointer() const noexcept {
        return meta_type{*ctx, fetch_node().remove_pointer(internal::meta_context::from(*ctx))};
    }

    
    [[nodiscard]] bool is_pointer_like() const noexcept {
        return !!(fetch_node().traits & internal::meta_traits::is_pointer_like);
    }

    
    [[nodiscard]] bool is_sequence_container() const noexcept {
        return !!(fetch_node().traits & internal::meta_traits::is_sequence_container);
    }

    
    [[nodiscard]] bool is_associative_container() const noexcept {
        return !!(fetch_node().traits & internal::meta_traits::is_associative_container);
    }

    
    [[nodiscard]] bool is_template_specialization() const noexcept {
        return (fetch_node().templ.arity != 0u);
    }

    
    [[nodiscard]] size_type template_arity() const noexcept {
        return fetch_node().templ.arity;
    }

    
    [[nodiscard]] meta_type template_type() const noexcept {
        return (fetch_node().templ.resolve != nullptr) ? meta_type{*ctx, fetch_node().templ.resolve(internal::meta_context::from(*ctx))} : meta_type{};
    }

    
    [[nodiscard]] meta_type template_arg(const size_type index) const noexcept {
        return index < template_arity() ? meta_type{*ctx, fetch_node().templ.arg(internal::meta_context::from(*ctx), index)} : meta_type{};
    }

    
    [[nodiscard]] bool can_cast(const meta_type &other) const noexcept {
        
        return other && ((*this == other) || (internal::try_cast(internal::meta_context::from(*ctx), fetch_node(), other.fetch_node().info->hash(), this) != nullptr));
    }

    
    [[nodiscard]] bool can_convert(const meta_type &other) const noexcept {
        if(const auto &to = other.info().hash(); (info().hash() == to) || ((fetch_node().conversion_helper != nullptr) && (other.is_arithmetic() || other.is_enum()))) {
            return true;
        } else if(const auto &from = fetch_node(); from.details) {
            if(const auto *elem = internal::find_member<&internal::meta_conv_node::type>(from.details->conv, to); elem != nullptr) {
                return true;
            }

            for(auto &&curr: from.details->base) {
                if(curr.type == to || meta_type{*ctx, curr.resolve(internal::meta_context::from(*ctx))}.can_convert(other)) {
                    return true;
                }
            }
        }

        return false;
    }

    
    [[nodiscard]] meta_range<meta_type, typename decltype(internal::meta_type_descriptor::base)::const_iterator> base() const noexcept {
        using range_type = meta_range<meta_type, typename decltype(internal::meta_type_descriptor::base)::const_iterator>;
        return fetch_node().details ? range_type{{*ctx, fetch_node().details->base.cbegin()}, {*ctx, fetch_node().details->base.cend()}} : range_type{};
    }

    
    [[nodiscard]] meta_range<meta_data, typename decltype(internal::meta_type_descriptor::data)::const_iterator> data() const noexcept {
        using range_type = meta_range<meta_data, typename decltype(internal::meta_type_descriptor::data)::const_iterator>;
        return fetch_node().details ? range_type{{*ctx, fetch_node().details->data.cbegin()}, {*ctx, fetch_node().details->data.cend()}} : range_type{};
    }

    
    [[nodiscard]] meta_data data(const id_type id, const bool recursive = true) const {
        const auto *elem = internal::look_for<&internal::meta_type_descriptor::data>(internal::meta_context::from(*ctx), fetch_node(), id, recursive);
        return (elem != nullptr) ? meta_data{*ctx, *elem} : meta_data{};
    }

    
    [[nodiscard]] meta_range<meta_func, typename decltype(internal::meta_type_descriptor::func)::const_iterator> func() const noexcept {
        using return_type = meta_range<meta_func, typename decltype(internal::meta_type_descriptor::func)::const_iterator>;
        return fetch_node().details ? return_type{{*ctx, fetch_node().details->func.cbegin()}, {*ctx, fetch_node().details->func.cend()}} : return_type{};
    }

    
    [[nodiscard]] meta_func func(const id_type id, const bool recursive = true) const {
        const auto *elem = internal::look_for<&internal::meta_type_descriptor::func>(internal::meta_context::from(*ctx), fetch_node(), id, recursive);
        return (elem != nullptr) ? meta_func{*ctx, *elem} : meta_func{};
    }

    
    [[nodiscard]] meta_any construct(meta_any *const args, const size_type sz) const {
        if(const auto &ref = fetch_node(); ref.details) {
            if(const auto *candidate = lookup(args, sz, false, [first = ref.details->ctor.cbegin(), last = ref.details->ctor.cend()]() mutable { return first == last ? nullptr : &*(first++); }); candidate) {
                return candidate->invoke(*ctx, args);
            }
        }

        if(const auto &ref = fetch_node(); (sz == 0u) && (ref.default_constructor != nullptr)) {
            return ref.default_constructor(*ctx);
        }

        return meta_any{meta_ctx_arg, *ctx};
    }

    
    template<typename... Args>
    [[nodiscard]] meta_any construct(Args &&...args) const {
        return construct(std::array<meta_any, sizeof...(Args)>{meta_any{*ctx, std::forward<Args>(args)}...}.data(), sizeof...(Args));
        
    }

    
    [[nodiscard]] meta_any from_void(void *elem, bool transfer_ownership = false) const {
        return ((elem != nullptr) && (fetch_node().from_void != nullptr)) ? fetch_node().from_void(*ctx, elem, transfer_ownership ? elem : nullptr) : meta_any{meta_ctx_arg, *ctx};
    }

    
    [[nodiscard]] meta_any from_void(const void *elem) const {
        return ((elem != nullptr) && (fetch_node().from_void != nullptr)) ? fetch_node().from_void(*ctx, nullptr, elem) : meta_any{meta_ctx_arg, *ctx};
    }

    
    template<typename Instance = meta_handle>
    
    meta_any invoke(const id_type id, Instance &&instance, meta_any *const args, const size_type sz) const {
        meta_handle wrapped{*ctx, std::forward<Instance>(instance)};

        if(const auto &ref = fetch_node(); ref.details) {
            if(auto *elem = internal::find_member<&internal::meta_func_node::id>(ref.details->func, id); elem != nullptr) {
                if(const auto *candidate = lookup(args, sz, (wrapped->base().policy() == any_policy::cref), [curr = elem]() mutable { return (curr != nullptr) ? std::exchange(curr, curr->next.get()) : nullptr; }); candidate) {
                    return candidate->invoke(std::move(wrapped), args);
                }
            }
        }

        for(auto &&curr: base()) {
            if(auto elem = curr.second.invoke(id, *wrapped.operator->(), args, sz); elem) {
                return elem;
            }
        }

        return meta_any{meta_ctx_arg, *ctx};
    }

    
    template<typename Instance = meta_handle, typename... Args>
    
    meta_any invoke(const id_type id, Instance &&instance, Args &&...args) const {
        return invoke(id, std::forward<Instance>(instance), std::array<meta_any, sizeof...(Args)>{meta_any{*ctx, std::forward<Args>(args)}...}.data(), sizeof...(Args));
    }

    
    template<typename Instance = meta_handle, typename Type>
    
    bool set(const id_type id, Instance &&instance, Type &&value) const {
        const auto candidate = data(id);
        return candidate && candidate.set(std::forward<Instance>(instance), std::forward<Type>(value));
    }

    
    template<typename Instance = meta_handle>
    [[nodiscard]] meta_any get(const id_type id, Instance &&instance) const {
        const auto candidate = data(id);
        return candidate ? candidate.get(std::forward<Instance>(instance)) : meta_any{meta_ctx_arg, *ctx};
    }

    
    template<typename Type>
    [[nodiscard]] Type traits() const noexcept {
        return internal::meta_to_user_traits<Type>(fetch_node().traits);
    }

    
    [[nodiscard]] meta_custom custom() const noexcept {
        return fetch_node().custom;
    }

    
    [[nodiscard]] explicit operator bool() const noexcept {
        return (node != nullptr);
    }

    
    [[nodiscard]] bool operator==(const meta_type &other) const noexcept {
        return (ctx == other.ctx) && (fetch_node().id == other.fetch_node().id);
    }

private:
    mutable const internal::meta_type_node *node{};
    const meta_ctx *ctx{&locator<meta_ctx>::value_or()};
};


[[nodiscard]] inline bool operator!=(const meta_type &lhs, const meta_type &rhs) noexcept {
    return !(lhs == rhs);
}

[[nodiscard]] inline meta_type meta_any::type() const noexcept {
    return *this ? meta_type{*ctx, fetch_node()} : meta_type{};
}

template<typename... Args>

meta_any meta_any::invoke(const id_type id, Args &&...args) const {
    return type().invoke(id, *this, std::forward<Args>(args)...);
}

template<typename... Args>
meta_any meta_any::invoke(const id_type id, Args &&...args) {
    return type().invoke(id, *this, std::forward<Args>(args)...);
}

template<typename Type>
bool meta_any::set(const id_type id, Type &&value) {
    return type().set(id, *this, std::forward<Type>(value));
}

[[nodiscard]] inline meta_any meta_any::get(const id_type id) const {
    return type().get(id, *this);
}

[[nodiscard]] inline meta_any meta_any::get(const id_type id) {
    return type().get(id, *this);
}

[[nodiscard]] inline meta_any meta_any::allow_cast(const meta_type &type) const {
    if(storage.has_value(type.info())) {
        return as_ref();
    } else if(*this) {
        if(const auto &from = fetch_node(); (from.conversion_helper != nullptr) && (type.is_arithmetic() || type.is_enum())) {
            auto other = type.construct();
            const auto value = from.conversion_helper(nullptr, storage.data());
            other.fetch_node().conversion_helper(other.storage.data(), &value);
            return other;
        }

        if(const auto &from = fetch_node(); from.details) {
            if(const auto *elem = internal::find_member<&internal::meta_conv_node::type>(from.details->conv, type.info().hash()); elem != nullptr) {
                return elem->conv(*ctx, storage.data());
            }

            for(auto &&curr: from.details->base) {
                if(auto other = curr.resolve(internal::meta_context::from(*ctx)).from_void(*ctx, nullptr, curr.cast(storage.data())); curr.type == type.info().hash()) {
                    return other;
                } else if(auto from_base = std::as_const(other).allow_cast(type); from_base) {
                    return from_base;
                }
            }
        }
    }

    return meta_any{meta_ctx_arg, *ctx};
}

[[nodiscard]] inline bool meta_any::allow_cast(const meta_type &type) {
    if(storage.has_value(type.info())) {
        return true;
    } else if(auto other = std::as_const(*this).allow_cast(type); other) {
        if(other.storage.owner()) {
            std::swap(*this, other);
        }

        return true;
    }

    return false;
}

inline bool meta_any::assign(const meta_any &other) {
    if(!storage.assign(other.storage)) {
        auto value = other.allow_cast(type());
        return storage.assign(value.storage);
    }

    return true;
}

inline bool meta_any::assign(meta_any &&other) {
    return storage.assign(std::move(other.storage)) || storage.assign(std::as_const(other).allow_cast(type()).storage);
}

[[nodiscard]] inline meta_type meta_data::type() const noexcept {
    return meta_type{*ctx, node_or_assert().type(internal::meta_context::from(*ctx))};
}

[[nodiscard]] inline meta_type meta_data::arg(const size_type index) const noexcept {
    return index < arity() ? node_or_assert().arg(*ctx, index) : meta_type{};
}

[[nodiscard]] inline meta_type meta_func::ret() const noexcept {
    return meta_type{*ctx, node_or_assert().ret(internal::meta_context::from(*ctx))};
}

[[nodiscard]] inline meta_type meta_func::arg(const size_type index) const noexcept {
    return index < arity() ? node_or_assert().arg(*ctx, index) : meta_type{};
}


class meta_sequence_container::meta_iterator final {
    using vtable_type = void(const void *, const std::ptrdiff_t, meta_any *);

    template<typename It>
    static void basic_vtable(const void *value, const std::ptrdiff_t offset, meta_any *other) {
        const auto &it = *static_cast<const It *>(value);
        other ? other->emplace<decltype(*it)>(*it) : std::advance(const_cast<It &>(it), offset);
    }

public:
    using value_type = meta_any;
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::bidirectional_iterator_tag;

    meta_iterator() = default;

    template<typename It>
    meta_iterator(const meta_ctx &area, It iter) noexcept
        : ctx{&area},
          vtable{&basic_vtable<It>},
          handle{iter} {}

    meta_iterator &operator++() noexcept {
        return vtable(handle.data(), 1, nullptr), *this;
    }

    meta_iterator operator++(int value) noexcept {
        meta_iterator orig = *this;
        vtable(handle.data(), ++value, nullptr);
        return orig;
    }

    meta_iterator &operator--() noexcept {
        return vtable(handle.data(), -1, nullptr), *this;
    }

    meta_iterator operator--(int value) noexcept {
        meta_iterator orig = *this;
        vtable(handle.data(), --value, nullptr);
        return orig;
    }

    [[nodiscard]] reference operator*() const {
        reference other{meta_ctx_arg, *ctx};
        vtable(handle.data(), 0, &other);
        return other;
    }

    [[nodiscard]] pointer operator->() const {
        return operator*();
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return (vtable != nullptr);
    }

    [[nodiscard]] bool operator==(const meta_iterator &other) const noexcept {
        return handle == other.handle;
    }

    [[nodiscard]] const any &base() const noexcept {
        return handle;
    }

private:
    const meta_ctx *ctx{};
    vtable_type *vtable{};
    any handle{};
};

[[nodiscard]] inline bool operator!=(const meta_sequence_container::iterator &lhs, const meta_sequence_container::iterator &rhs) noexcept {
    return !(lhs == rhs);
}

class meta_associative_container::meta_iterator final {
    using vtable_type = void(const void *, std::pair<meta_any, meta_any> *);

    template<bool KeyOnly, typename It>
    static void basic_vtable(const void *value, std::pair<meta_any, meta_any> *other) {
        if(const auto &it = *static_cast<const It *>(value); other) {
            if constexpr(KeyOnly) {
                other->first.emplace<decltype(*it)>(*it);
            } else {
                other->first.emplace<decltype((it->first))>(it->first);
                other->second.emplace<decltype((it->second))>(it->second);
            }
        } else {
            ++const_cast<It &>(it);
        }
    }

public:
    using value_type = std::pair<meta_any, meta_any>;
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

    meta_iterator() = default;

    template<bool KeyOnly, typename It>
    meta_iterator(const meta_ctx &area, std::bool_constant<KeyOnly>, It iter) noexcept
        : ctx{&area},
          vtable{&basic_vtable<KeyOnly, It>},
          handle{iter} {}

    meta_iterator &operator++() noexcept {
        return vtable(handle.data(), nullptr), *this;
    }

    meta_iterator operator++(int) noexcept {
        meta_iterator orig = *this;
        vtable(handle.data(), nullptr);
        return orig;
    }

    [[nodiscard]] reference operator*() const {
        reference other{{meta_ctx_arg, *ctx}, {meta_ctx_arg, *ctx}};
        vtable(handle.data(), &other);
        return other;
    }

    [[nodiscard]] pointer operator->() const {
        return operator*();
    }

    [[nodiscard]] explicit operator bool() const noexcept {
        return (vtable != nullptr);
    }

    [[nodiscard]] bool operator==(const meta_iterator &other) const noexcept {
        return handle == other.handle;
    }

private:
    const meta_ctx *ctx{};
    vtable_type *vtable{};
    any handle{};
};

[[nodiscard]] inline bool operator!=(const meta_associative_container::iterator &lhs, const meta_associative_container::iterator &rhs) noexcept {
    return !(lhs == rhs);
}



[[nodiscard]] inline meta_type meta_sequence_container::value_type() const noexcept {
    return (value_type_node != nullptr) ? meta_type{*ctx, value_type_node(internal::meta_context::from(*ctx))} : meta_type{};
}


[[nodiscard]] inline meta_sequence_container::size_type meta_sequence_container::size() const noexcept {
    return size_fn(data);
}


inline bool meta_sequence_container::resize(const size_type sz) {
    return !const_only && resize_fn(const_cast<void *>(data), sz);
}


inline bool meta_sequence_container::clear() {
    return !const_only && clear_fn(const_cast<void *>(data));
}


inline bool meta_sequence_container::reserve(const size_type sz) {
    return !const_only && reserve_fn(const_cast<void *>(data), sz);
}


[[nodiscard]] inline meta_sequence_container::iterator meta_sequence_container::begin() {
    return begin_end_fn(*ctx, const_only ? nullptr : const_cast<void *>(data), data, false);
}


[[nodiscard]] inline meta_sequence_container::iterator meta_sequence_container::end() {
    return begin_end_fn(*ctx, const_only ? nullptr : const_cast<void *>(data), data, true);
}


inline meta_sequence_container::iterator meta_sequence_container::insert(const iterator &it, meta_any value) {
    
    if(const auto &vtype = value_type_node(internal::meta_context::from(*ctx)); !const_only && (value.allow_cast({*ctx, vtype}) || value.allow_cast({*ctx, const_reference_node(internal::meta_context::from(*ctx))}))) {
        const bool is_value_type = (value.type().info() == *vtype.info);
        return insert_fn(*ctx, const_cast<void *>(data), is_value_type ? value.base().data() : nullptr, is_value_type ? nullptr : value.base().data(), it);
    }

    return iterator{};
}


inline meta_sequence_container::iterator meta_sequence_container::erase(const iterator &it) {
    return const_only ? iterator{} : erase_fn(*ctx, const_cast<void *>(data), it);
}


[[nodiscard]] inline meta_any meta_sequence_container::operator[](const size_type pos) {
    auto it = begin();
    it.operator++(static_cast<int>(pos) - 1);
    return *it;
}


[[nodiscard]] inline meta_sequence_container::operator bool() const noexcept {
    return (data != nullptr);
}


[[nodiscard]] inline meta_type meta_associative_container::key_type() const noexcept {
    return (key_type_node != nullptr) ? meta_type{*ctx, key_type_node(internal::meta_context::from(*ctx))} : meta_type{};
}


[[nodiscard]] inline meta_type meta_associative_container::mapped_type() const noexcept {
    return (mapped_type_node != nullptr) ? meta_type{*ctx, mapped_type_node(internal::meta_context::from(*ctx))} : meta_type{};
}


[[nodiscard]] inline meta_type meta_associative_container::value_type() const noexcept {
    return (value_type_node != nullptr) ? meta_type{*ctx, value_type_node(internal::meta_context::from(*ctx))} : meta_type{};
}


[[nodiscard]] inline meta_associative_container::size_type meta_associative_container::size() const noexcept {
    return size_fn(data);
}


inline bool meta_associative_container::clear() {
    return !const_only && clear_fn(const_cast<void *>(data));
}


inline bool meta_associative_container::reserve(const size_type sz) {
    return !const_only && reserve_fn(const_cast<void *>(data), sz);
}


[[nodiscard]] inline meta_associative_container::iterator meta_associative_container::begin() {
    return begin_end_fn(*ctx, const_only ? nullptr : const_cast<void *>(data), data, false);
}


[[nodiscard]] inline meta_associative_container::iterator meta_associative_container::end() {
    return begin_end_fn(*ctx, const_only ? nullptr : const_cast<void *>(data), data, true);
}


inline bool meta_associative_container::insert(meta_any key, meta_any value = {}) {
    return !const_only && key.allow_cast(meta_type{*ctx, key_type_node(internal::meta_context::from(*ctx))})
           && ((mapped_type_node == nullptr) || value.allow_cast(meta_type{*ctx, mapped_type_node(internal::meta_context::from(*ctx))}))
           && insert_fn(const_cast<void *>(data), key.base().data(), value.base().data());
}


inline meta_associative_container::size_type meta_associative_container::erase(meta_any key) {
    return (!const_only && key.allow_cast(meta_type{*ctx, key_type_node(internal::meta_context::from(*ctx))})) ? erase_fn(const_cast<void *>(data), key.base().data()) : 0u;
}


[[nodiscard]] inline meta_associative_container::iterator meta_associative_container::find(meta_any key) {
    return key.allow_cast(meta_type{*ctx, key_type_node(internal::meta_context::from(*ctx))}) ? find_fn(*ctx, const_only ? nullptr : const_cast<void *>(data), data, key.base().data()) : iterator{};
}


[[nodiscard]] inline meta_associative_container::operator bool() const noexcept {
    return (data != nullptr);
}

} 

#endif
