#ifndef ENTT_ENTITY_GROUP_HPP
#define ENTT_ENTITY_GROUP_HPP

#include <array>
#include <cstddef>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/algorithm.hpp"
#include "../core/fwd.hpp"
#include "../core/iterator.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "entity.hpp"
#include "fwd.hpp"

namespace entt {


namespace internal {

template<typename, typename, typename>
class extended_group_iterator;

template<typename It, typename... Owned, typename... Get>
class extended_group_iterator<It, owned_t<Owned...>, get_t<Get...>> {
    template<typename Type>
    [[nodiscard]] auto index_to_element([[maybe_unused]] Type &cpool) const {
        if constexpr(std::is_void_v<typename Type::value_type>) {
            return std::make_tuple();
        } else {
            return std::forward_as_tuple(cpool.rbegin()[it.index()]);
        }
    }

public:
    using iterator_type = It;
    using value_type = decltype(std::tuple_cat(std::make_tuple(*std::declval<It>()), std::declval<Owned>().get_as_tuple({})..., std::declval<Get>().get_as_tuple({})...));
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

    constexpr extended_group_iterator()
        : it{},
          pools{} {}

    extended_group_iterator(iterator_type from, std::tuple<Owned *..., Get *...> cpools)
        : it{from},
          pools{std::move(cpools)} {}

    extended_group_iterator &operator++() noexcept {
        return ++it, *this;
    }

    extended_group_iterator operator++(int) noexcept {
        const extended_group_iterator orig = *this;
        return ++(*this), orig;
    }

    [[nodiscard]] reference operator*() const noexcept {
        return std::tuple_cat(std::make_tuple(*it), index_to_element(*std::get<Owned *>(pools))..., std::get<Get *>(pools)->get_as_tuple(*it)...);
    }

    [[nodiscard]] pointer operator->() const noexcept {
        return operator*();
    }

    [[nodiscard]] constexpr iterator_type base() const noexcept {
        return it;
    }

    template<typename... Lhs, typename... Rhs>
    friend constexpr bool operator==(const extended_group_iterator<Lhs...> &, const extended_group_iterator<Rhs...> &) noexcept;

private:
    It it;
    std::tuple<Owned *..., Get *...> pools;
};

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator==(const extended_group_iterator<Lhs...> &lhs, const extended_group_iterator<Rhs...> &rhs) noexcept {
    return lhs.it == rhs.it;
}

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator!=(const extended_group_iterator<Lhs...> &lhs, const extended_group_iterator<Rhs...> &rhs) noexcept {
    return !(lhs == rhs);
}

struct group_descriptor {
    using size_type = std::size_t;
    virtual ~group_descriptor() = default;
    [[nodiscard]] virtual bool owned(const id_type) const noexcept {
        return false;
    }
};

template<typename Type, std::size_t Owned, std::size_t Get, std::size_t Exclude>
class group_handler final: public group_descriptor {
    using entity_type = typename Type::entity_type;

    void swap_elements(const std::size_t pos, const entity_type entt) {
        for(size_type next{}; next < Owned; ++next) {
            pools[next]->swap_elements((*pools[next])[pos], entt);
        }
    }

    void push_on_construct(const entity_type entt) {
        if(std::apply([entt, pos = len](auto *cpool, auto *...other) { return cpool->contains(entt) && !(cpool->index(entt) < pos) && (other->contains(entt) && ...); }, pools)
           && std::apply([entt](auto *...cpool) { return (!cpool->contains(entt) && ...); }, filter)) {
            swap_elements(len++, entt);
        }
    }

    void push_on_destroy(const entity_type entt) {
        if(std::apply([entt, pos = len](auto *cpool, auto *...other) { return cpool->contains(entt) && !(cpool->index(entt) < pos) && (other->contains(entt) && ...); }, pools)
           && std::apply([entt](auto *...cpool) { return (0u + ... + cpool->contains(entt)) == 1u; }, filter)) {
            swap_elements(len++, entt);
        }
    }

    void remove_if(const entity_type entt) {
        if(pools[0u]->contains(entt) && (pools[0u]->index(entt) < len)) {
            swap_elements(--len, entt);
        }
    }

    void common_setup() {
        
        for(auto first = pools[0u]->rbegin(), last = first + static_cast<typename decltype(pools)::difference_type>(pools[0u]->size()); first != last; ++first) {
            push_on_construct(*first);
        }
    }

public:
    using common_type = Type;
    using size_type = typename Type::size_type;

    template<typename... OGType, typename... EType>
    group_handler(std::tuple<OGType &...> ogpool, std::tuple<EType &...> epool)
        : pools{std::apply([](auto &&...cpool) { return std::array<common_type *, (Owned + Get)>{&cpool...}; }, ogpool)},
          filter{std::apply([](auto &&...cpool) { return std::array<common_type *, Exclude>{&cpool...}; }, epool)} {
        std::apply([this](auto &...cpool) { ((cpool.on_construct().template connect<&group_handler::push_on_construct>(*this), cpool.on_destroy().template connect<&group_handler::remove_if>(*this)), ...); }, ogpool);
        std::apply([this](auto &...cpool) { ((cpool.on_construct().template connect<&group_handler::remove_if>(*this), cpool.on_destroy().template connect<&group_handler::push_on_destroy>(*this)), ...); }, epool);
        common_setup();
    }

    [[nodiscard]] bool owned(const id_type hash) const noexcept override {
        for(size_type pos{}; pos < Owned; ++pos) {
            if(pools[pos]->info().hash() == hash) {
                return true;
            }
        }

        return false;
    }

    [[nodiscard]] size_type length() const noexcept {
        return len;
    }

    template<std::size_t Index>
    [[nodiscard]] common_type *storage() const noexcept {
        if constexpr(Index < (Owned + Get)) {
            return pools[Index];
        } else {
            return filter[Index - (Owned + Get)];
        }
    }

private:
    std::array<common_type *, (Owned + Get)> pools;
    std::array<common_type *, Exclude> filter;
    std::size_t len{};
};

template<typename Type, std::size_t Get, std::size_t Exclude>
class group_handler<Type, 0u, Get, Exclude> final: public group_descriptor {
    using entity_type = typename Type::entity_type;

    void push_on_construct(const entity_type entt) {
        if(!elem.contains(entt)
           && std::apply([entt](auto *...cpool) { return (cpool->contains(entt) && ...); }, pools)
           && std::apply([entt](auto *...cpool) { return (!cpool->contains(entt) && ...); }, filter)) {
            elem.push(entt);
        }
    }

    void push_on_destroy(const entity_type entt) {
        if(!elem.contains(entt)
           && std::apply([entt](auto *...cpool) { return (cpool->contains(entt) && ...); }, pools)
           && std::apply([entt](auto *...cpool) { return (0u + ... + cpool->contains(entt)) == 1u; }, filter)) {
            elem.push(entt);
        }
    }

    void remove_if(const entity_type entt) {
        elem.remove(entt);
    }

    void common_setup() {
        for(const auto entity: *pools[0u]) {
            push_on_construct(entity);
        }
    }

public:
    using common_type = Type;

    template<typename Allocator, typename... GType, typename... EType>
    group_handler(const Allocator &allocator, std::tuple<GType &...> gpool, std::tuple<EType &...> epool)
        : pools{std::apply([](auto &&...cpool) { return std::array<common_type *, Get>{&cpool...}; }, gpool)},
          filter{std::apply([](auto &&...cpool) { return std::array<common_type *, Exclude>{&cpool...}; }, epool)},
          elem{allocator} {
        std::apply([this](auto &...cpool) { ((cpool.on_construct().template connect<&group_handler::push_on_construct>(*this), cpool.on_destroy().template connect<&group_handler::remove_if>(*this)), ...); }, gpool);
        std::apply([this](auto &...cpool) { ((cpool.on_construct().template connect<&group_handler::remove_if>(*this), cpool.on_destroy().template connect<&group_handler::push_on_destroy>(*this)), ...); }, epool);
        common_setup();
    }

    [[nodiscard]] common_type &handle() noexcept {
        return elem;
    }

    [[nodiscard]] const common_type &handle() const noexcept {
        return elem;
    }

    template<std::size_t Index>
    [[nodiscard]] common_type *storage() const noexcept {
        if constexpr(Index < Get) {
            return pools[Index];
        } else {
            return filter[Index - Get];
        }
    }

private:
    std::array<common_type *, Get> pools;
    std::array<common_type *, Exclude> filter;
    common_type elem;
};

} 



template<typename, typename, typename>
class basic_group;


template<typename... Get, typename... Exclude>
class basic_group<owned_t<>, get_t<Get...>, exclude_t<Exclude...>> {
    using base_type = std::common_type_t<typename Get::base_type..., typename Exclude::base_type...>;
    using underlying_type = typename base_type::entity_type;

    template<typename Type>
    static constexpr std::size_t index_of = type_list_index_v<std::remove_const_t<Type>, type_list<typename Get::element_type..., typename Exclude::element_type...>>;

    template<std::size_t... Index>
    [[nodiscard]] auto pools_for(std::index_sequence<Index...>) const noexcept {
        using return_type = std::tuple<Get *...>;
        return descriptor ? return_type{static_cast<Get *>(descriptor->template storage<Index>())...} : return_type{};
    }

public:
    
    using entity_type = underlying_type;
    
    using size_type = std::size_t;
    
    using difference_type = std::ptrdiff_t;
    
    using common_type = base_type;
    
    using iterator = typename common_type::iterator;
    
    using reverse_iterator = typename common_type::reverse_iterator;
    
    using iterable = iterable_adaptor<internal::extended_group_iterator<iterator, owned_t<>, get_t<Get...>>>;
    
    using handler = internal::group_handler<common_type, 0u, sizeof...(Get), sizeof...(Exclude)>;

    
    static id_type group_id() noexcept {
        return type_hash<basic_group<owned_t<>, get_t<std::remove_const_t<Get>...>, exclude_t<std::remove_const_t<Exclude>...>>>::value();
    }

    
    basic_group() noexcept
        : descriptor{} {}

    
    basic_group(handler &ref) noexcept
        : descriptor{&ref} {}

    
    [[nodiscard]] const common_type &handle() const noexcept {
        return descriptor->handle();
    }

    
    template<typename Type>
    [[nodiscard]] auto *storage() const noexcept {
        return storage<index_of<Type>>();
    }

    
    template<std::size_t Index>
    [[nodiscard]] auto *storage() const noexcept {
        using type = type_list_element_t<Index, type_list<Get..., Exclude...>>;
        return *this ? static_cast<type *>(descriptor->template storage<Index>()) : nullptr;
    }

    
    [[nodiscard]] size_type size() const noexcept {
        return *this ? handle().size() : size_type{};
    }

    
    [[nodiscard]] size_type capacity() const noexcept {
        return *this ? handle().capacity() : size_type{};
    }

    
    void shrink_to_fit() {
        if(*this) {
            descriptor->handle().shrink_to_fit();
        }
    }

    
    [[nodiscard]] bool empty() const noexcept {
        return !*this || handle().empty();
    }

    
    [[nodiscard]] iterator begin() const noexcept {
        return *this ? handle().begin() : iterator{};
    }

    
    [[nodiscard]] iterator end() const noexcept {
        return *this ? handle().end() : iterator{};
    }

    
    [[nodiscard]] reverse_iterator rbegin() const noexcept {
        return *this ? handle().rbegin() : reverse_iterator{};
    }

    
    [[nodiscard]] reverse_iterator rend() const noexcept {
        return *this ? handle().rend() : reverse_iterator{};
    }

    
    [[nodiscard]] entity_type front() const noexcept {
        const auto it = begin();
        return it != end() ? *it : null;
    }

    
    [[nodiscard]] entity_type back() const noexcept {
        const auto it = rbegin();
        return it != rend() ? *it : null;
    }

    
    [[nodiscard]] iterator find(const entity_type entt) const noexcept {
        return *this ? handle().find(entt) : iterator{};
    }

    
    [[nodiscard]] entity_type operator[](const size_type pos) const {
        return begin()[static_cast<difference_type>(pos)];
    }

    
    [[nodiscard]] explicit operator bool() const noexcept {
        return descriptor != nullptr;
    }

    
    [[nodiscard]] bool contains(const entity_type entt) const noexcept {
        return *this && handle().contains(entt);
    }

    
    template<typename Type, typename... Other>
    [[nodiscard]] decltype(auto) get(const entity_type entt) const {
        return get<index_of<Type>, index_of<Other>...>(entt);
    }

    
    template<std::size_t... Index>
    [[nodiscard]] decltype(auto) get(const entity_type entt) const {
        const auto cpools = pools_for(std::index_sequence_for<Get...>{});

        if constexpr(sizeof...(Index) == 0) {
            return std::apply([entt](auto *...curr) { return std::tuple_cat(curr->get_as_tuple(entt)...); }, cpools);
        } else if constexpr(sizeof...(Index) == 1) {
            return (std::get<Index>(cpools)->get(entt), ...);
        } else {
            return std::tuple_cat(std::get<Index>(cpools)->get_as_tuple(entt)...);
        }
    }

    
    template<typename Func>
    void each(Func func) const {
        for(const auto entt: *this) {
            if constexpr(is_applicable_v<Func, decltype(std::tuple_cat(std::tuple<entity_type>{}, std::declval<basic_group>().get({})))>) {
                std::apply(func, std::tuple_cat(std::make_tuple(entt), get(entt)));
            } else {
                std::apply(func, get(entt));
            }
        }
    }

    
    [[nodiscard]] iterable each() const noexcept {
        const auto cpools = pools_for(std::index_sequence_for<Get...>{});
        return iterable{{begin(), cpools}, {end(), cpools}};
    }

    
    template<typename Type, typename... Other, typename Compare, typename Sort = std_sort, typename... Args>
    void sort(Compare compare, Sort algo = Sort{}, Args &&...args) {
        sort<index_of<Type>, index_of<Other>...>(std::move(compare), std::move(algo), std::forward<Args>(args)...);
    }

    
    template<std::size_t... Index, typename Compare, typename Sort = std_sort, typename... Args>
    void sort(Compare compare, Sort algo = Sort{}, Args &&...args) {
        if(*this) {
            if constexpr(sizeof...(Index) == 0) {
                static_assert(std::is_invocable_v<Compare, const entity_type, const entity_type>, "Invalid comparison function");
                descriptor->handle().sort(std::move(compare), std::move(algo), std::forward<Args>(args)...);
            } else {
                auto comp = [&compare, cpools = pools_for(std::index_sequence_for<Get...>{})](const entity_type lhs, const entity_type rhs) {
                    if constexpr(sizeof...(Index) == 1) {
                        return compare((std::get<Index>(cpools)->get(lhs), ...), (std::get<Index>(cpools)->get(rhs), ...));
                    } else {
                        return compare(std::forward_as_tuple(std::get<Index>(cpools)->get(lhs)...), std::forward_as_tuple(std::get<Index>(cpools)->get(rhs)...));
                    }
                };

                descriptor->handle().sort(std::move(comp), std::move(algo), std::forward<Args>(args)...);
            }
        }
    }

    
    template<typename It>
    void sort_as(It first, It last) const {
        if(*this) {
            descriptor->handle().sort_as(first, last);
        }
    }

private:
    handler *descriptor;
};


template<typename... Owned, typename... Get, typename... Exclude>
class basic_group<owned_t<Owned...>, get_t<Get...>, exclude_t<Exclude...>> {
    static_assert(((Owned::storage_policy != deletion_policy::in_place) && ...), "Groups do not support in-place delete");

    using base_type = std::common_type_t<typename Owned::base_type..., typename Get::base_type..., typename Exclude::base_type...>;
    using underlying_type = typename base_type::entity_type;

    template<typename Type>
    static constexpr std::size_t index_of = type_list_index_v<std::remove_const_t<Type>, type_list<typename Owned::element_type..., typename Get::element_type..., typename Exclude::element_type...>>;

    template<std::size_t... Index, std::size_t... Other>
    [[nodiscard]] auto pools_for(std::index_sequence<Index...>, std::index_sequence<Other...>) const noexcept {
        using return_type = std::tuple<Owned *..., Get *...>;
        return descriptor ? return_type{static_cast<Owned *>(descriptor->template storage<Index>())..., static_cast<Get *>(descriptor->template storage<sizeof...(Owned) + Other>())...} : return_type{};
    }

public:
    
    using entity_type = underlying_type;
    
    using size_type = std::size_t;
    
    using difference_type = std::ptrdiff_t;
    
    using common_type = base_type;
    
    using iterator = typename common_type::iterator;
    
    using reverse_iterator = typename common_type::reverse_iterator;
    
    using iterable = iterable_adaptor<internal::extended_group_iterator<iterator, owned_t<Owned...>, get_t<Get...>>>;
    
    using handler = internal::group_handler<common_type, sizeof...(Owned), sizeof...(Get), sizeof...(Exclude)>;

    
    static id_type group_id() noexcept {
        return type_hash<basic_group<owned_t<std::remove_const_t<Owned>...>, get_t<std::remove_const_t<Get>...>, exclude_t<std::remove_const_t<Exclude>...>>>::value();
    }

    
    basic_group() noexcept
        : descriptor{} {}

    
    basic_group(handler &ref) noexcept
        : descriptor{&ref} {}

    
    [[nodiscard]] const common_type &handle() const noexcept {
        return *storage<0>();
    }

    
    template<typename Type>
    [[nodiscard]] auto *storage() const noexcept {
        return storage<index_of<Type>>();
    }

    
    template<std::size_t Index>
    [[nodiscard]] auto *storage() const noexcept {
        using type = type_list_element_t<Index, type_list<Owned..., Get..., Exclude...>>;
        return *this ? static_cast<type *>(descriptor->template storage<Index>()) : nullptr;
    }

    
    [[nodiscard]] size_type size() const noexcept {
        return *this ? descriptor->length() : size_type{};
    }

    
    [[nodiscard]] bool empty() const noexcept {
        return !*this || !descriptor->length();
    }

    
    [[nodiscard]] iterator begin() const noexcept {
        return *this ? (handle().end() - static_cast<difference_type>(descriptor->length())) : iterator{};
    }

    
    [[nodiscard]] iterator end() const noexcept {
        return *this ? handle().end() : iterator{};
    }

    
    [[nodiscard]] reverse_iterator rbegin() const noexcept {
        return *this ? handle().rbegin() : reverse_iterator{};
    }

    
    [[nodiscard]] reverse_iterator rend() const noexcept {
        return *this ? (handle().rbegin() + static_cast<difference_type>(descriptor->length())) : reverse_iterator{};
    }

    
    [[nodiscard]] entity_type front() const noexcept {
        const auto it = begin();
        return it != end() ? *it : null;
    }

    
    [[nodiscard]] entity_type back() const noexcept {
        const auto it = rbegin();
        return it != rend() ? *it : null;
    }

    
    [[nodiscard]] iterator find(const entity_type entt) const noexcept {
        const auto it = *this ? handle().find(entt) : iterator{};
        return it >= begin() ? it : iterator{};
    }

    
    [[nodiscard]] entity_type operator[](const size_type pos) const {
        return begin()[static_cast<difference_type>(pos)];
    }

    
    [[nodiscard]] explicit operator bool() const noexcept {
        return descriptor != nullptr;
    }

    
    [[nodiscard]] bool contains(const entity_type entt) const noexcept {
        return *this && handle().contains(entt) && (handle().index(entt) < (descriptor->length()));
    }

    
    template<typename Type, typename... Other>
    [[nodiscard]] decltype(auto) get(const entity_type entt) const {
        return get<index_of<Type>, index_of<Other>...>(entt);
    }

    
    template<std::size_t... Index>
    [[nodiscard]] decltype(auto) get(const entity_type entt) const {
        const auto cpools = pools_for(std::index_sequence_for<Owned...>{}, std::index_sequence_for<Get...>{});

        if constexpr(sizeof...(Index) == 0) {
            return std::apply([entt](auto *...curr) { return std::tuple_cat(curr->get_as_tuple(entt)...); }, cpools);
        } else if constexpr(sizeof...(Index) == 1) {
            return (std::get<Index>(cpools)->get(entt), ...);
        } else {
            return std::tuple_cat(std::get<Index>(cpools)->get_as_tuple(entt)...);
        }
    }

    
    template<typename Func>
    void each(Func func) const {
        for(auto args: each()) {
            if constexpr(is_applicable_v<Func, decltype(std::tuple_cat(std::tuple<entity_type>{}, std::declval<basic_group>().get({})))>) {
                std::apply(func, args);
            } else {
                std::apply([&func](auto, auto &&...less) { func(std::forward<decltype(less)>(less)...); }, args);
            }
        }
    }

    
    [[nodiscard]] iterable each() const noexcept {
        const auto cpools = pools_for(std::index_sequence_for<Owned...>{}, std::index_sequence_for<Get...>{});
        return iterable{{begin(), cpools}, {end(), cpools}};
    }

    
    template<typename Type, typename... Other, typename Compare, typename Sort = std_sort, typename... Args>
    void sort(Compare compare, Sort algo = Sort{}, Args &&...args) const {
        sort<index_of<Type>, index_of<Other>...>(std::move(compare), std::move(algo), std::forward<Args>(args)...);
    }

    
    template<std::size_t... Index, typename Compare, typename Sort = std_sort, typename... Args>
    void sort(Compare compare, Sort algo = Sort{}, Args &&...args) const {
        const auto cpools = pools_for(std::index_sequence_for<Owned...>{}, std::index_sequence_for<Get...>{});

        if constexpr(sizeof...(Index) == 0) {
            static_assert(std::is_invocable_v<Compare, const entity_type, const entity_type>, "Invalid comparison function");
            storage<0>()->sort_n(descriptor->length(), std::move(compare), std::move(algo), std::forward<Args>(args)...);
        } else {
            auto comp = [&compare, &cpools](const entity_type lhs, const entity_type rhs) {
                if constexpr(sizeof...(Index) == 1) {
                    return compare((std::get<Index>(cpools)->get(lhs), ...), (std::get<Index>(cpools)->get(rhs), ...));
                } else {
                    return compare(std::forward_as_tuple(std::get<Index>(cpools)->get(lhs)...), std::forward_as_tuple(std::get<Index>(cpools)->get(rhs)...));
                }
            };

            storage<0>()->sort_n(descriptor->length(), std::move(comp), std::move(algo), std::forward<Args>(args)...);
        }

        auto cb = [this](auto *head, auto *...other) {
            for(auto next = descriptor->length(); next; --next) {
                const auto pos = next - 1;
                [[maybe_unused]] const auto entt = head->data()[pos];
                (other->swap_elements(other->data()[pos], entt), ...);
            }
        };

        std::apply(cb, cpools);
    }

private:
    handler *descriptor;
};

} 

#endif
