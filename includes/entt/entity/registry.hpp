#ifndef ENTT_ENTITY_REGISTRY_HPP
#define ENTT_ENTITY_REGISTRY_HPP

#include <algorithm>
#include <array>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "../container/dense_map.hpp"
#include "../core/algorithm.hpp"
#include "../core/any.hpp"
#include "../core/fwd.hpp"
#include "../core/iterator.hpp"
#include "../core/memory.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "../core/utility.hpp"
#include "entity.hpp"
#include "fwd.hpp"
#include "group.hpp"
#include "mixin.hpp"
#include "sparse_set.hpp"
#include "storage.hpp"
#include "view.hpp"

namespace entt {


namespace internal {

template<typename It>
class registry_storage_iterator final {
    template<typename Other>
    friend class registry_storage_iterator;

    using mapped_type = std::remove_reference_t<decltype(std::declval<It>()->second)>;

public:
    using value_type = std::pair<id_type, constness_as_t<typename mapped_type::element_type, mapped_type> &>;
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::random_access_iterator_tag;

    constexpr registry_storage_iterator() noexcept
        : it{} {}

    constexpr registry_storage_iterator(It iter) noexcept
        : it{iter} {}

    template<typename Other, typename = std::enable_if_t<!std::is_same_v<It, Other> && std::is_constructible_v<It, Other>>>
    constexpr registry_storage_iterator(const registry_storage_iterator<Other> &other) noexcept
        : registry_storage_iterator{other.it} {}

    constexpr registry_storage_iterator &operator++() noexcept {
        return ++it, *this;
    }

    constexpr registry_storage_iterator operator++(int) noexcept {
        const registry_storage_iterator orig = *this;
        return ++(*this), orig;
    }

    constexpr registry_storage_iterator &operator--() noexcept {
        return --it, *this;
    }

    constexpr registry_storage_iterator operator--(int) noexcept {
        const registry_storage_iterator orig = *this;
        return operator--(), orig;
    }

    constexpr registry_storage_iterator &operator+=(const difference_type value) noexcept {
        it += value;
        return *this;
    }

    constexpr registry_storage_iterator operator+(const difference_type value) const noexcept {
        registry_storage_iterator copy = *this;
        return (copy += value);
    }

    constexpr registry_storage_iterator &operator-=(const difference_type value) noexcept {
        return (*this += -value);
    }

    constexpr registry_storage_iterator operator-(const difference_type value) const noexcept {
        return (*this + -value);
    }

    [[nodiscard]] constexpr reference operator[](const difference_type value) const noexcept {
        return {it[value].first, *it[value].second};
    }

    [[nodiscard]] constexpr reference operator*() const noexcept {
        return operator[](0);
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept {
        return operator*();
    }

    template<typename Lhs, typename Rhs>
    friend constexpr std::ptrdiff_t operator-(const registry_storage_iterator<Lhs> &, const registry_storage_iterator<Rhs> &) noexcept;

    template<typename Lhs, typename Rhs>
    friend constexpr bool operator==(const registry_storage_iterator<Lhs> &, const registry_storage_iterator<Rhs> &) noexcept;

    template<typename Lhs, typename Rhs>
    friend constexpr bool operator<(const registry_storage_iterator<Lhs> &, const registry_storage_iterator<Rhs> &) noexcept;

private:
    It it;
};

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr std::ptrdiff_t operator-(const registry_storage_iterator<Lhs> &lhs, const registry_storage_iterator<Rhs> &rhs) noexcept {
    return lhs.it - rhs.it;
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator==(const registry_storage_iterator<Lhs> &lhs, const registry_storage_iterator<Rhs> &rhs) noexcept {
    return lhs.it == rhs.it;
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator!=(const registry_storage_iterator<Lhs> &lhs, const registry_storage_iterator<Rhs> &rhs) noexcept {
    return !(lhs == rhs);
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator<(const registry_storage_iterator<Lhs> &lhs, const registry_storage_iterator<Rhs> &rhs) noexcept {
    return lhs.it < rhs.it;
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator>(const registry_storage_iterator<Lhs> &lhs, const registry_storage_iterator<Rhs> &rhs) noexcept {
    return rhs < lhs;
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator<=(const registry_storage_iterator<Lhs> &lhs, const registry_storage_iterator<Rhs> &rhs) noexcept {
    return !(lhs > rhs);
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator>=(const registry_storage_iterator<Lhs> &lhs, const registry_storage_iterator<Rhs> &rhs) noexcept {
    return !(lhs < rhs);
}

template<typename Allocator>
class registry_context {
    using alloc_traits = std::allocator_traits<Allocator>;
    using allocator_type = typename alloc_traits::template rebind_alloc<std::pair<const id_type, basic_any<0u>>>;

public:
    explicit registry_context(const allocator_type &allocator)
        : ctx{allocator} {}

    void clear() noexcept {
        ctx.clear();
    }

    template<typename Type, typename... Args>
    Type &emplace_as(const id_type id, Args &&...args) {
        return any_cast<Type &>(ctx.try_emplace(id, std::in_place_type<Type>, std::forward<Args>(args)...).first->second);
    }

    template<typename Type, typename... Args>
    Type &emplace(Args &&...args) {
        return emplace_as<Type>(type_id<Type>().hash(), std::forward<Args>(args)...);
    }

    template<typename Type>
    Type &insert_or_assign(const id_type id, Type &&value) {
        return any_cast<std::remove_const_t<std::remove_reference_t<Type>> &>(ctx.insert_or_assign(id, std::forward<Type>(value)).first->second);
    }

    template<typename Type>
    Type &insert_or_assign(Type &&value) {
        return insert_or_assign(type_id<Type>().hash(), std::forward<Type>(value));
    }

    template<typename Type>
    bool erase(const id_type id = type_id<Type>().hash()) {
        const auto it = ctx.find(id);
        return it != ctx.end() && it->second.info() == type_id<Type>() ? (ctx.erase(it), true) : false;
    }

    template<typename Type>
    [[nodiscard]] const Type &get(const id_type id = type_id<Type>().hash()) const {
        return any_cast<const Type &>(ctx.at(id));
    }

    template<typename Type>
    [[nodiscard]] Type &get(const id_type id = type_id<Type>().hash()) {
        return any_cast<Type &>(ctx.at(id));
    }

    template<typename Type>
    [[nodiscard]] const Type *find(const id_type id = type_id<Type>().hash()) const {
        const auto it = ctx.find(id);
        return it != ctx.cend() ? any_cast<const Type>(&it->second) : nullptr;
    }

    template<typename Type>
    [[nodiscard]] Type *find(const id_type id = type_id<Type>().hash()) {
        const auto it = ctx.find(id);
        return it != ctx.end() ? any_cast<Type>(&it->second) : nullptr;
    }

    template<typename Type>
    [[nodiscard]] bool contains(const id_type id = type_id<Type>().hash()) const {
        const auto it = ctx.find(id);
        return it != ctx.cend() && it->second.info() == type_id<Type>();
    }

private:
    dense_map<id_type, basic_any<0u>, identity, std::equal_to<>, allocator_type> ctx;
};

} 



template<typename Entity, typename Allocator>
class basic_registry {
    using base_type = basic_sparse_set<Entity, Allocator>;
    using alloc_traits = std::allocator_traits<Allocator>;
    static_assert(std::is_same_v<typename alloc_traits::value_type, Entity>, "Invalid value type");
    
    using pool_container_type = dense_map<id_type, std::shared_ptr<base_type>, identity, std::equal_to<>, typename alloc_traits::template rebind_alloc<std::pair<const id_type, std::shared_ptr<base_type>>>>;
    using group_container_type = dense_map<id_type, std::shared_ptr<internal::group_descriptor>, identity, std::equal_to<>, typename alloc_traits::template rebind_alloc<std::pair<const id_type, std::shared_ptr<internal::group_descriptor>>>>;
    using traits_type = entt_traits<Entity>;

    template<typename Type>
    [[nodiscard]] auto &assure([[maybe_unused]] const id_type id = type_hash<Type>::value()) {
        static_assert(std::is_same_v<Type, std::decay_t<Type>>, "Non-decayed types not allowed");

        if constexpr(std::is_same_v<Type, entity_type>) {
            ENTT_ASSERT(id == type_hash<Type>::value(), "User entity storage not allowed");
            return entities;
        } else {
            using storage_type = storage_for_type<Type>;

            if(auto it = pools.find(id); it != pools.cend()) {
                ENTT_ASSERT(it->second->info() == type_id<Type>(), "Unexpected type");
                return static_cast<storage_type &>(*it->second);
            }

            using alloc_type = typename storage_type::allocator_type;
            typename pool_container_type::mapped_type cpool{};

            if constexpr(std::is_void_v<Type> && !std::is_constructible_v<alloc_type, allocator_type>) {
                
                cpool = std::allocate_shared<storage_type>(get_allocator(), alloc_type{});
            } else {
                cpool = std::allocate_shared<storage_type>(get_allocator(), get_allocator());
            }

            pools.emplace(id, cpool);
            cpool->bind(*this);

            return static_cast<storage_type &>(*cpool);
        }
    }

    template<typename Type>
    [[nodiscard]] const auto *assure([[maybe_unused]] const id_type id = type_hash<Type>::value()) const {
        static_assert(std::is_same_v<Type, std::decay_t<Type>>, "Non-decayed types not allowed");

        if constexpr(std::is_same_v<Type, entity_type>) {
            ENTT_ASSERT(id == type_hash<Type>::value(), "User entity storage not allowed");
            return &entities;
        } else {
            if(const auto it = pools.find(id); it != pools.cend()) {
                ENTT_ASSERT(it->second->info() == type_id<Type>(), "Unexpected type");
                return static_cast<const storage_for_type<Type> *>(it->second.get());
            }

            return static_cast<const storage_for_type<Type> *>(nullptr);
        }
    }

    void rebind() {
        entities.bind(*this);

        for(auto &&curr: pools) {
            curr.second->bind(*this);
        }
    }

public:
    
    using allocator_type = Allocator;
    
    using entity_type = typename traits_type::value_type;
    
    using version_type = typename traits_type::version_type;
    
    using size_type = std::size_t;
    
    using common_type = base_type;
    
    using context = internal::registry_context<allocator_type>;
    
    using iterable = iterable_adaptor<internal::registry_storage_iterator<typename pool_container_type::iterator>>;
    
    using const_iterable = iterable_adaptor<internal::registry_storage_iterator<typename pool_container_type::const_iterator>>;

    
    template<typename Type>
    using storage_for_type = typename storage_for<Type, Entity, typename alloc_traits::template rebind_alloc<std::remove_const_t<Type>>>::type;

    
    basic_registry()
        : basic_registry{allocator_type{}} {}

    
    explicit basic_registry(const allocator_type &allocator)
        : basic_registry{0u, allocator} {}

    
    basic_registry(const size_type count, const allocator_type &allocator = allocator_type{})
        : vars{allocator},
          pools{allocator},
          groups{allocator},
          entities{allocator} {
        pools.reserve(count);
        rebind();
    }

    
    basic_registry(const basic_registry &) = delete;

    
    basic_registry(basic_registry &&other) noexcept
        : vars{std::move(other.vars)},
          pools{std::move(other.pools)},
          groups{std::move(other.groups)},
          entities{std::move(other.entities)} {
        rebind();
    }

    
    ~basic_registry() = default;

    
    basic_registry &operator=(const basic_registry &) = delete;

    
    basic_registry &operator=(basic_registry &&other) noexcept {
        swap(other);
        return *this;
    }

    
    void swap(basic_registry &other) noexcept {
        using std::swap;

        swap(vars, other.vars);
        swap(pools, other.pools);
        swap(groups, other.groups);
        swap(entities, other.entities);

        rebind();
        other.rebind();
    }

    
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return entities.get_allocator();
    }

    
    [[nodiscard]] iterable storage() noexcept {
        return iterable{pools.begin(), pools.end()};
    }

    
    [[nodiscard]] const_iterable storage() const noexcept {
        return const_iterable{pools.cbegin(), pools.cend()};
    }

    
    [[nodiscard]] common_type *storage(const id_type id) {
        return const_cast<common_type *>(std::as_const(*this).storage(id));
    }

    
    [[nodiscard]] const common_type *storage(const id_type id) const {
        const auto it = pools.find(id);
        return it == pools.cend() ? nullptr : it->second.get();
    }

    
    template<typename Type>
    storage_for_type<Type> &storage(const id_type id = type_hash<Type>::value()) {
        return assure<std::remove_const_t<Type>>(id);
    }

    
    template<typename Type>
    [[nodiscard]] const storage_for_type<Type> *storage(const id_type id = type_hash<Type>::value()) const {
        return assure<std::remove_const_t<Type>>(id);
    }

    
    bool reset(const id_type id) {
        ENTT_ASSERT(id != type_hash<entity_type>::value(), "Cannot reset entity storage");
        return !(pools.erase(id) == 0u);
    }

    
    [[nodiscard]] bool valid(const entity_type entt) const {
        return static_cast<size_type>(entities.find(entt).index()) < entities.free_list();
    }

    
    [[nodiscard]] version_type current(const entity_type entt) const {
        return entities.current(entt);
    }

    
    [[nodiscard]] entity_type create() {
        return entities.generate();
    }

    
    [[nodiscard]] entity_type create(const entity_type hint) {
        return entities.generate(hint);
    }

    
    template<typename It>
    void create(It first, It last) {
        entities.generate(std::move(first), std::move(last));
    }

    
    version_type destroy(const entity_type entt) {
        for(size_type pos = pools.size(); pos != 0u; --pos) {
            pools.begin()[static_cast<typename pool_container_type::difference_type>(pos - 1u)].second->remove(entt);
        }

        entities.erase(entt);
        return entities.current(entt);
    }

    
    version_type destroy(const entity_type entt, const version_type version) {
        destroy(entt);
        const auto elem = traits_type::construct(traits_type::to_entity(entt), version);
        return entities.bump((elem == tombstone) ? traits_type::next(elem) : elem);
    }

    
    template<typename It>
    void destroy(It first, It last) {
        const auto to = entities.sort_as(first, last);
        const auto from = entities.cend() - static_cast<typename common_type::difference_type>(entities.free_list());

        for(auto &&curr: pools) {
            curr.second->remove(from, to);
        }

        entities.erase(from, to);
    }

    
    template<typename Type, typename... Args>
    decltype(auto) emplace(const entity_type entt, Args &&...args) {
        ENTT_ASSERT(valid(entt), "Invalid entity");
        return assure<Type>().emplace(entt, std::forward<Args>(args)...);
    }

    
    template<typename Type, typename It>
    void insert(It first, It last) {
        ENTT_ASSERT(std::all_of(first, last, [this](const auto entt) { return valid(entt); }), "Invalid entity");
        assure<Type>().insert(std::move(first), std::move(last));
    }

    
    template<typename Type, typename It>
    void insert(It first, It last, const Type &value) {
        ENTT_ASSERT(std::all_of(first, last, [this](const auto entt) { return valid(entt); }), "Invalid entity");
        assure<Type>().insert(std::move(first), std::move(last), value);
    }

    
    template<typename Type, typename EIt, typename CIt, typename = std::enable_if_t<std::is_same_v<typename std::iterator_traits<CIt>::value_type, Type>>>
    void insert(EIt first, EIt last, CIt from) {
        ENTT_ASSERT(std::all_of(first, last, [this](const auto entt) { return valid(entt); }), "Invalid entity");
        assure<Type>().insert(first, last, from);
    }

    
    template<typename Type, typename... Args>
    decltype(auto) emplace_or_replace(const entity_type entt, Args &&...args) {
        auto &cpool = assure<Type>();
        ENTT_ASSERT(valid(entt), "Invalid entity");
        return cpool.contains(entt) ? cpool.patch(entt, [&args...](auto &...curr) { ((curr = Type{std::forward<Args>(args)...}), ...); }) : cpool.emplace(entt, std::forward<Args>(args)...);
    }

    
    template<typename Type, typename... Func>
    decltype(auto) patch(const entity_type entt, Func &&...func) {
        return assure<Type>().patch(entt, std::forward<Func>(func)...);
    }

    
    template<typename Type, typename... Args>
    decltype(auto) replace(const entity_type entt, Args &&...args) {
        return patch<Type>(entt, [&args...](auto &...curr) { ((curr = Type{std::forward<Args>(args)...}), ...); });
    }

    
    template<typename Type, typename... Other>
    size_type remove(const entity_type entt) {
        return (assure<Type>().remove(entt) + ... + assure<Other>().remove(entt));
    }

    
    template<typename Type, typename... Other, typename It>
    size_type remove(It first, It last) {
        size_type count{};

        if constexpr(std::is_same_v<It, typename common_type::iterator>) {
            std::array cpools{static_cast<common_type *>(&assure<Type>()), static_cast<common_type *>(&assure<Other>())...};

            for(auto from = cpools.begin(), to = cpools.end(); from != to; ++from) {
                if constexpr(sizeof...(Other) != 0u) {
                    if((*from)->data() == first.data()) {
                        std::swap((*from), cpools.back());
                    }
                }

                count += (*from)->remove(first, last);
            }

        } else {
            for(auto cpools = std::forward_as_tuple(assure<Type>(), assure<Other>()...); first != last; ++first) {
                count += std::apply([entt = *first](auto &...curr) { return (curr.remove(entt) + ... + 0u); }, cpools);
            }
        }

        return count;
    }

    
    template<typename Type, typename... Other>
    void erase(const entity_type entt) {
        (assure<Type>().erase(entt), (assure<Other>().erase(entt), ...));
    }

    
    template<typename Type, typename... Other, typename It>
    void erase(It first, It last) {
        if constexpr(std::is_same_v<It, typename common_type::iterator>) {
            std::array cpools{static_cast<common_type *>(&assure<Type>()), static_cast<common_type *>(&assure<Other>())...};

            for(auto from = cpools.begin(), to = cpools.end(); from != to; ++from) {
                if constexpr(sizeof...(Other) != 0u) {
                    if((*from)->data() == first.data()) {
                        std::swap(*from, cpools.back());
                    }
                }

                (*from)->erase(first, last);
            }
        } else {
            for(auto cpools = std::forward_as_tuple(assure<Type>(), assure<Other>()...); first != last; ++first) {
                std::apply([entt = *first](auto &...curr) { (curr.erase(entt), ...); }, cpools);
            }
        }
    }

    
    template<typename Func>
    void erase_if(const entity_type entt, Func func) {
        for(auto [id, cpool]: storage()) {
            if(cpool.contains(entt) && func(id, std::as_const(cpool))) {
                cpool.erase(entt);
            }
        }
    }

    
    template<typename... Type>
    void compact() {
        if constexpr(sizeof...(Type) == 0u) {
            for(auto &&curr: pools) {
                curr.second->compact();
            }
        } else {
            (assure<Type>().compact(), ...);
        }
    }

    
    template<typename... Type>
    [[nodiscard]] bool all_of([[maybe_unused]] const entity_type entt) const {
        if constexpr(sizeof...(Type) == 1u) {
            auto *cpool = assure<std::remove_const_t<Type>...>();
            return cpool && cpool->contains(entt);
        } else {
            return (all_of<Type>(entt) && ...);
        }
    }

    
    template<typename... Type>
    [[nodiscard]] bool any_of([[maybe_unused]] const entity_type entt) const {
        return (all_of<Type>(entt) || ...);
    }

    
    template<typename... Type>
    [[nodiscard]] decltype(auto) get([[maybe_unused]] const entity_type entt) const {
        if constexpr(sizeof...(Type) == 1u) {
            return (assure<std::remove_const_t<Type>>()->get(entt), ...);
        } else {
            return std::forward_as_tuple(get<Type>(entt)...);
        }
    }

    
    template<typename... Type>
    [[nodiscard]] decltype(auto) get([[maybe_unused]] const entity_type entt) {
        if constexpr(sizeof...(Type) == 1u) {
            return (static_cast<storage_for_type<Type> &>(assure<std::remove_const_t<Type>>()).get(entt), ...);
        } else {
            return std::forward_as_tuple(get<Type>(entt)...);
        }
    }

    
    template<typename Type, typename... Args>
    [[nodiscard]] decltype(auto) get_or_emplace(const entity_type entt, Args &&...args) {
        auto &cpool = assure<Type>();
        ENTT_ASSERT(valid(entt), "Invalid entity");
        return cpool.contains(entt) ? cpool.get(entt) : cpool.emplace(entt, std::forward<Args>(args)...);
    }

    
    template<typename... Type>
    [[nodiscard]] auto try_get([[maybe_unused]] const entity_type entt) const {
        if constexpr(sizeof...(Type) == 1u) {
            const auto *cpool = assure<std::remove_const_t<Type>...>();
            return (cpool && cpool->contains(entt)) ? std::addressof(cpool->get(entt)) : nullptr;
        } else {
            return std::make_tuple(try_get<Type>(entt)...);
        }
    }

    
    template<typename... Type>
    [[nodiscard]] auto try_get([[maybe_unused]] const entity_type entt) {
        if constexpr(sizeof...(Type) == 1u) {
            return (const_cast<Type *>(std::as_const(*this).template try_get<Type>(entt)), ...);
        } else {
            return std::make_tuple(try_get<Type>(entt)...);
        }
    }

    
    template<typename... Type>
    void clear() {
        if constexpr(sizeof...(Type) == 0u) {
            for(size_type pos = pools.size(); pos; --pos) {
                pools.begin()[static_cast<typename pool_container_type::difference_type>(pos - 1u)].second->clear();
            }

            const auto elem = entities.each();
            entities.erase(elem.begin().base(), elem.end().base());
        } else {
            (assure<Type>().clear(), ...);
        }
    }

    
    [[nodiscard]] bool orphan(const entity_type entt) const {
        return std::none_of(pools.cbegin(), pools.cend(), [entt](auto &&curr) { return curr.second->contains(entt); });
    }

    
    template<typename Type>
    [[nodiscard]] auto on_construct(const id_type id = type_hash<Type>::value()) {
        return assure<Type>(id).on_construct();
    }

    
    template<typename Type>
    [[nodiscard]] auto on_update(const id_type id = type_hash<Type>::value()) {
        return assure<Type>(id).on_update();
    }

    
    template<typename Type>
    [[nodiscard]] auto on_destroy(const id_type id = type_hash<Type>::value()) {
        return assure<Type>(id).on_destroy();
    }

    
    template<typename Type, typename... Other, typename... Exclude>
    [[nodiscard]] basic_view<get_t<storage_for_type<const Type>, storage_for_type<const Other>...>, exclude_t<storage_for_type<const Exclude>...>>
    view(exclude_t<Exclude...> = exclude_t{}) const {
        basic_view<get_t<storage_for_type<const Type>, storage_for_type<const Other>...>, exclude_t<storage_for_type<const Exclude>...>> elem{};
        [&elem](const auto *...curr) { ((curr ? elem.storage(*curr) : void()), ...); }(assure<std::remove_const_t<Exclude>>()..., assure<std::remove_const_t<Other>>()..., assure<std::remove_const_t<Type>>());
        return elem;
    }

    
    template<typename Type, typename... Other, typename... Exclude>
    [[nodiscard]] basic_view<get_t<storage_for_type<Type>, storage_for_type<Other>...>, exclude_t<storage_for_type<Exclude>...>>
    view(exclude_t<Exclude...> = exclude_t{}) {
        return {assure<std::remove_const_t<Type>>(), assure<std::remove_const_t<Other>>()..., assure<std::remove_const_t<Exclude>>()...};
    }

    
    template<typename... Owned, typename... Get, typename... Exclude>
    basic_group<owned_t<storage_for_type<Owned>...>, get_t<storage_for_type<Get>...>, exclude_t<storage_for_type<Exclude>...>>
    group(get_t<Get...> = get_t{}, exclude_t<Exclude...> = exclude_t{}) {
        using group_type = basic_group<owned_t<storage_for_type<Owned>...>, get_t<storage_for_type<Get>...>, exclude_t<storage_for_type<Exclude>...>>;
        using handler_type = typename group_type::handler;

        if(auto it = groups.find(group_type::group_id()); it != groups.cend()) {
            return {*std::static_pointer_cast<handler_type>(it->second)};
        }

        std::shared_ptr<handler_type> handler{};

        if constexpr(sizeof...(Owned) == 0u) {
            handler = std::allocate_shared<handler_type>(get_allocator(), get_allocator(), std::forward_as_tuple(assure<std::remove_const_t<Get>>()...), std::forward_as_tuple(assure<std::remove_const_t<Exclude>>()...));
        } else {
            handler = std::allocate_shared<handler_type>(get_allocator(), std::forward_as_tuple(assure<std::remove_const_t<Owned>>()..., assure<std::remove_const_t<Get>>()...), std::forward_as_tuple(assure<std::remove_const_t<Exclude>>()...));
            ENTT_ASSERT(std::all_of(groups.cbegin(), groups.cend(), [](const auto &data) { return !(data.second->owned(type_id<Owned>().hash()) || ...); }), "Conflicting groups");
        }

        groups.emplace(group_type::group_id(), handler);
        return {*handler};
    }

    
    template<typename... Owned, typename... Get, typename... Exclude>
    [[nodiscard]] basic_group<owned_t<storage_for_type<const Owned>...>, get_t<storage_for_type<const Get>...>, exclude_t<storage_for_type<const Exclude>...>>
    group_if_exists(get_t<Get...> = get_t{}, exclude_t<Exclude...> = exclude_t{}) const {
        using group_type = basic_group<owned_t<storage_for_type<const Owned>...>, get_t<storage_for_type<const Get>...>, exclude_t<storage_for_type<const Exclude>...>>;
        using handler_type = typename group_type::handler;

        if(auto it = groups.find(group_type::group_id()); it != groups.cend()) {
            return {*std::static_pointer_cast<handler_type>(it->second)};
        }

        return {};
    }

    
    template<typename... Type>
    [[nodiscard]] bool owned() const {
        return std::any_of(groups.cbegin(), groups.cend(), [](auto &&data) { return (data.second->owned(type_id<Type>().hash()) || ...); });
    }

    
    template<typename Type, typename Compare, typename Sort = std_sort, typename... Args>
    void sort(Compare compare, Sort algo = Sort{}, Args &&...args) {
        ENTT_ASSERT(!owned<Type>(), "Cannot sort owned storage");
        auto &cpool = assure<Type>();

        if constexpr(std::is_invocable_v<Compare, decltype(cpool.get({})), decltype(cpool.get({}))>) {
            auto comp = [&cpool, compare = std::move(compare)](const auto lhs, const auto rhs) { return compare(std::as_const(cpool.get(lhs)), std::as_const(cpool.get(rhs))); };
            cpool.sort(std::move(comp), std::move(algo), std::forward<Args>(args)...);
        } else {
            cpool.sort(std::move(compare), std::move(algo), std::forward<Args>(args)...);
        }
    }

    
    template<typename To, typename From>
    void sort() {
        ENTT_ASSERT(!owned<To>(), "Cannot sort owned storage");
        const base_type &cpool = assure<From>();
        assure<To>().sort_as(cpool.begin(), cpool.end());
    }

    
    [[nodiscard]] context &ctx() noexcept {
        return vars;
    }

    
    [[nodiscard]] const context &ctx() const noexcept {
        return vars;
    }

private:
    context vars;
    pool_container_type pools;
    group_container_type groups;
    storage_for_type<entity_type> entities;
};

} 

#endif
