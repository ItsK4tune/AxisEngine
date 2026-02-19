#ifndef ENTT_CONTAINER_DENSE_SET_HPP
#define ENTT_CONTAINER_DENSE_SET_HPP

#include <cmath>
#include <cstddef>
#include <functional>
#include <iterator>
#include <limits>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "../core/bit.hpp"
#include "../core/compressed_pair.hpp"
#include "../core/type_traits.hpp"
#include "fwd.hpp"

namespace entt {


namespace internal {

static constexpr std::size_t dense_set_placeholder_position = (std::numeric_limits<std::size_t>::max)();

template<typename It>
class dense_set_iterator final {
    template<typename>
    friend class dense_set_iterator;

public:
    using value_type = typename It::value_type::second_type;
    using pointer = const value_type *;
    using reference = const value_type &;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::random_access_iterator_tag;

    constexpr dense_set_iterator() noexcept
        : it{} {}

    constexpr dense_set_iterator(const It iter) noexcept
        : it{iter} {}

    template<typename Other, typename = std::enable_if_t<!std::is_same_v<It, Other> && std::is_constructible_v<It, Other>>>
    constexpr dense_set_iterator(const dense_set_iterator<Other> &other) noexcept
        : it{other.it} {}

    constexpr dense_set_iterator &operator++() noexcept {
        return ++it, *this;
    }

    constexpr dense_set_iterator operator++(int) noexcept {
        const dense_set_iterator orig = *this;
        return ++(*this), orig;
    }

    constexpr dense_set_iterator &operator--() noexcept {
        return --it, *this;
    }

    constexpr dense_set_iterator operator--(int) noexcept {
        const dense_set_iterator orig = *this;
        return operator--(), orig;
    }

    constexpr dense_set_iterator &operator+=(const difference_type value) noexcept {
        it += value;
        return *this;
    }

    constexpr dense_set_iterator operator+(const difference_type value) const noexcept {
        dense_set_iterator copy = *this;
        return (copy += value);
    }

    constexpr dense_set_iterator &operator-=(const difference_type value) noexcept {
        return (*this += -value);
    }

    constexpr dense_set_iterator operator-(const difference_type value) const noexcept {
        return (*this + -value);
    }

    [[nodiscard]] constexpr reference operator[](const difference_type value) const noexcept {
        return it[value].second;
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept {
        return std::addressof(operator[](0));
    }

    [[nodiscard]] constexpr reference operator*() const noexcept {
        return operator[](0);
    }

    template<typename Lhs, typename Rhs>
    friend constexpr std::ptrdiff_t operator-(const dense_set_iterator<Lhs> &, const dense_set_iterator<Rhs> &) noexcept;

    template<typename Lhs, typename Rhs>
    friend constexpr bool operator==(const dense_set_iterator<Lhs> &, const dense_set_iterator<Rhs> &) noexcept;

    template<typename Lhs, typename Rhs>
    friend constexpr bool operator<(const dense_set_iterator<Lhs> &, const dense_set_iterator<Rhs> &) noexcept;

private:
    It it;
};

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr std::ptrdiff_t operator-(const dense_set_iterator<Lhs> &lhs, const dense_set_iterator<Rhs> &rhs) noexcept {
    return lhs.it - rhs.it;
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator==(const dense_set_iterator<Lhs> &lhs, const dense_set_iterator<Rhs> &rhs) noexcept {
    return lhs.it == rhs.it;
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator!=(const dense_set_iterator<Lhs> &lhs, const dense_set_iterator<Rhs> &rhs) noexcept {
    return !(lhs == rhs);
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator<(const dense_set_iterator<Lhs> &lhs, const dense_set_iterator<Rhs> &rhs) noexcept {
    return lhs.it < rhs.it;
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator>(const dense_set_iterator<Lhs> &lhs, const dense_set_iterator<Rhs> &rhs) noexcept {
    return rhs < lhs;
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator<=(const dense_set_iterator<Lhs> &lhs, const dense_set_iterator<Rhs> &rhs) noexcept {
    return !(lhs > rhs);
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator>=(const dense_set_iterator<Lhs> &lhs, const dense_set_iterator<Rhs> &rhs) noexcept {
    return !(lhs < rhs);
}

template<typename It>
class dense_set_local_iterator final {
    template<typename>
    friend class dense_set_local_iterator;

public:
    using value_type = typename It::value_type::second_type;
    using pointer = const value_type *;
    using reference = const value_type &;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::forward_iterator_tag;

    constexpr dense_set_local_iterator() noexcept = default;

    constexpr dense_set_local_iterator(It iter, const std::size_t pos) noexcept
        : it{iter},
          offset{pos} {}

    template<typename Other, typename = std::enable_if_t<!std::is_same_v<It, Other> && std::is_constructible_v<It, Other>>>
    constexpr dense_set_local_iterator(const dense_set_local_iterator<Other> &other) noexcept
        : it{other.it},
          offset{other.offset} {}

    constexpr dense_set_local_iterator &operator++() noexcept {
        return offset = it[static_cast<typename It::difference_type>(offset)].first, *this;
    }

    constexpr dense_set_local_iterator operator++(int) noexcept {
        const dense_set_local_iterator orig = *this;
        return ++(*this), orig;
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept {
        return std::addressof(it[static_cast<typename It::difference_type>(offset)].second);
    }

    [[nodiscard]] constexpr reference operator*() const noexcept {
        return *operator->();
    }

    [[nodiscard]] constexpr std::size_t index() const noexcept {
        return offset;
    }

private:
    It it{};
    std::size_t offset{dense_set_placeholder_position};
};

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator==(const dense_set_local_iterator<Lhs> &lhs, const dense_set_local_iterator<Rhs> &rhs) noexcept {
    return lhs.index() == rhs.index();
}

template<typename Lhs, typename Rhs>
[[nodiscard]] constexpr bool operator!=(const dense_set_local_iterator<Lhs> &lhs, const dense_set_local_iterator<Rhs> &rhs) noexcept {
    return !(lhs == rhs);
}

} 



template<typename Type, typename Hash, typename KeyEqual, typename Allocator>
class dense_set {
    static constexpr float default_threshold = 0.875f;
    static constexpr std::size_t minimum_capacity = 8u;
    static constexpr std::size_t placeholder_position = internal::dense_set_placeholder_position;

    using node_type = std::pair<std::size_t, Type>;
    using alloc_traits = std::allocator_traits<Allocator>;
    static_assert(std::is_same_v<typename alloc_traits::value_type, Type>, "Invalid value type");
    using sparse_container_type = std::vector<std::size_t, typename alloc_traits::template rebind_alloc<std::size_t>>;
    using packed_container_type = std::vector<node_type, typename alloc_traits::template rebind_alloc<node_type>>;

    template<typename Other>
    [[nodiscard]] std::size_t value_to_bucket(const Other &value) const noexcept {
        return fast_mod(static_cast<size_type>(sparse.second()(value)), bucket_count());
    }

    template<typename Other>
    [[nodiscard]] auto constrained_find(const Other &value, const std::size_t bucket) {
        for(auto offset = sparse.first()[bucket]; offset != placeholder_position; offset = packed.first()[offset].first) {
            if(packed.second()(packed.first()[offset].second, value)) {
                return begin() + static_cast<typename iterator::difference_type>(offset);
            }
        }

        return end();
    }

    template<typename Other>
    [[nodiscard]] auto constrained_find(const Other &value, const std::size_t bucket) const {
        for(auto offset = sparse.first()[bucket]; offset != placeholder_position; offset = packed.first()[offset].first) {
            if(packed.second()(packed.first()[offset].second, value)) {
                return cbegin() + static_cast<typename const_iterator::difference_type>(offset);
            }
        }

        return cend();
    }

    template<typename Other>
    [[nodiscard]] auto insert_or_do_nothing(Other &&value) {
        const auto index = value_to_bucket(value);

        if(auto it = constrained_find(value, index); it != end()) {
            return std::make_pair(it, false);
        }

        packed.first().emplace_back(sparse.first()[index], std::forward<Other>(value));
        sparse.first()[index] = packed.first().size() - 1u;
        rehash_if_required();

        return std::make_pair(--end(), true);
    }

    void move_and_pop(const std::size_t pos) {
        if(const auto last = size() - 1u; pos != last) {
            size_type *curr = &sparse.first()[value_to_bucket(packed.first().back().second)];
            packed.first()[pos] = std::move(packed.first().back());
            for(; *curr != last; curr = &packed.first()[*curr].first) {}
            *curr = pos;
        }

        packed.first().pop_back();
    }

    void rehash_if_required() {
        if(const auto bc = bucket_count(); size() > static_cast<size_type>(static_cast<float>(bc) * max_load_factor())) {
            rehash(bc * 2u);
        }
    }

public:
    
    using allocator_type = Allocator;
    
    using key_type = Type;
    
    using value_type = Type;
    
    using size_type = std::size_t;
    
    using difference_type = std::ptrdiff_t;
    
    using hasher = Hash;
    
    using key_equal = KeyEqual;
    
    using iterator = internal::dense_set_iterator<typename packed_container_type::iterator>;
    
    using const_iterator = internal::dense_set_iterator<typename packed_container_type::const_iterator>;
    
    using reverse_iterator = std::reverse_iterator<iterator>;
    
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;
    
    using local_iterator = internal::dense_set_local_iterator<typename packed_container_type::iterator>;
    
    using const_local_iterator = internal::dense_set_local_iterator<typename packed_container_type::const_iterator>;

    
    dense_set()
        : dense_set{minimum_capacity} {}

    
    explicit dense_set(const allocator_type &allocator)
        : dense_set{minimum_capacity, hasher{}, key_equal{}, allocator} {}

    
    dense_set(const size_type cnt, const allocator_type &allocator)
        : dense_set{cnt, hasher{}, key_equal{}, allocator} {}

    
    dense_set(const size_type cnt, const hasher &hash, const allocator_type &allocator)
        : dense_set{cnt, hash, key_equal{}, allocator} {}

    
    explicit dense_set(const size_type cnt, const hasher &hash = hasher{}, const key_equal &equal = key_equal{}, const allocator_type &allocator = allocator_type{})
        : sparse{allocator, hash},
          packed{allocator, equal} {
        rehash(cnt);
    }

    
    dense_set(const dense_set &) = default;

    
    dense_set(const dense_set &other, const allocator_type &allocator)
        : sparse{std::piecewise_construct, std::forward_as_tuple(other.sparse.first(), allocator), std::forward_as_tuple(other.sparse.second())},
          packed{std::piecewise_construct, std::forward_as_tuple(other.packed.first(), allocator), std::forward_as_tuple(other.packed.second())},
          threshold{other.threshold} {}

    
    dense_set(dense_set &&) noexcept = default;

    
    dense_set(dense_set &&other, const allocator_type &allocator)
        : sparse{std::piecewise_construct, std::forward_as_tuple(std::move(other.sparse.first()), allocator), std::forward_as_tuple(std::move(other.sparse.second()))},
          packed{std::piecewise_construct, std::forward_as_tuple(std::move(other.packed.first()), allocator), std::forward_as_tuple(std::move(other.packed.second()))},
          threshold{other.threshold} {}

    
    ~dense_set() = default;

    
    dense_set &operator=(const dense_set &) = default;

    
    dense_set &operator=(dense_set &&) noexcept = default;

    
    void swap(dense_set &other) noexcept {
        using std::swap;
        swap(sparse, other.sparse);
        swap(packed, other.packed);
        swap(threshold, other.threshold);
    }

    
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return sparse.first().get_allocator();
    }

    
    [[nodiscard]] const_iterator cbegin() const noexcept {
        return packed.first().begin();
    }

    
    [[nodiscard]] const_iterator begin() const noexcept {
        return cbegin();
    }

    
    [[nodiscard]] iterator begin() noexcept {
        return packed.first().begin();
    }

    
    [[nodiscard]] const_iterator cend() const noexcept {
        return packed.first().end();
    }

    
    [[nodiscard]] const_iterator end() const noexcept {
        return cend();
    }

    
    [[nodiscard]] iterator end() noexcept {
        return packed.first().end();
    }

    
    [[nodiscard]] const_reverse_iterator crbegin() const noexcept {
        return std::make_reverse_iterator(cend());
    }

    
    [[nodiscard]] const_reverse_iterator rbegin() const noexcept {
        return crbegin();
    }

    
    [[nodiscard]] reverse_iterator rbegin() noexcept {
        return std::make_reverse_iterator(end());
    }

    
    [[nodiscard]] const_reverse_iterator crend() const noexcept {
        return std::make_reverse_iterator(cbegin());
    }

    
    [[nodiscard]] const_reverse_iterator rend() const noexcept {
        return crend();
    }

    
    [[nodiscard]] reverse_iterator rend() noexcept {
        return std::make_reverse_iterator(begin());
    }

    
    [[nodiscard]] bool empty() const noexcept {
        return packed.first().empty();
    }

    
    [[nodiscard]] size_type size() const noexcept {
        return packed.first().size();
    }

    
    [[nodiscard]] size_type max_size() const noexcept {
        return packed.first().max_size();
    }

    
    void clear() noexcept {
        sparse.first().clear();
        packed.first().clear();
        rehash(0u);
    }

    
    std::pair<iterator, bool> insert(const value_type &value) {
        return insert_or_do_nothing(value);
    }

    
    std::pair<iterator, bool> insert(value_type &&value) {
        return insert_or_do_nothing(std::move(value));
    }

    
    template<typename It>
    void insert(It first, It last) {
        for(; first != last; ++first) {
            insert(*first);
        }
    }

    
    template<typename... Args>
    std::pair<iterator, bool> emplace(Args &&...args) {
        if constexpr(((sizeof...(Args) == 1u) && ... && std::is_same_v<std::decay_t<Args>, value_type>)) {
            return insert_or_do_nothing(std::forward<Args>(args)...);
        } else {
            auto &node = packed.first().emplace_back(std::piecewise_construct, std::make_tuple(packed.first().size()), std::forward_as_tuple(std::forward<Args>(args)...));
            const auto index = value_to_bucket(node.second);

            if(auto it = constrained_find(node.second, index); it != end()) {
                packed.first().pop_back();
                return std::make_pair(it, false);
            }

            std::swap(node.first, sparse.first()[index]);
            rehash_if_required();

            return std::make_pair(--end(), true);
        }
    }

    
    iterator erase(const_iterator pos) {
        const auto diff = pos - cbegin();
        erase(*pos);
        return begin() + diff;
    }

    
    iterator erase(const_iterator first, const_iterator last) {
        const auto dist = first - cbegin();

        for(auto from = last - cbegin(); from != dist; --from) {
            erase(packed.first()[static_cast<size_type>(from) - 1u].second);
        }

        return (begin() + dist);
    }

    
    size_type erase(const value_type &value) {
        for(size_type *curr = &sparse.first()[value_to_bucket(value)]; *curr != placeholder_position; curr = &packed.first()[*curr].first) {
            if(packed.second()(packed.first()[*curr].second, value)) {
                const auto index = *curr;
                *curr = packed.first()[*curr].first;
                move_and_pop(index);
                return 1u;
            }
        }

        return 0u;
    }

    
    [[nodiscard]] size_type count(const value_type &key) const {
        return find(key) != end();
    }

    
    template<typename Other>
    [[nodiscard]] std::enable_if_t<is_transparent_v<hasher> && is_transparent_v<key_equal>, std::conditional_t<false, Other, size_type>>
    count(const Other &key) const {
        return find(key) != end();
    }

    
    [[nodiscard]] iterator find(const value_type &value) {
        return constrained_find(value, value_to_bucket(value));
    }

    
    [[nodiscard]] const_iterator find(const value_type &value) const {
        return constrained_find(value, value_to_bucket(value));
    }

    
    template<typename Other>
    [[nodiscard]] std::enable_if_t<is_transparent_v<hasher> && is_transparent_v<key_equal>, std::conditional_t<false, Other, iterator>>
    find(const Other &value) {
        return constrained_find(value, value_to_bucket(value));
    }

    
    template<typename Other>
    [[nodiscard]] std::enable_if_t<is_transparent_v<hasher> && is_transparent_v<key_equal>, std::conditional_t<false, Other, const_iterator>>
    find(const Other &value) const {
        return constrained_find(value, value_to_bucket(value));
    }

    
    [[nodiscard]] std::pair<iterator, iterator> equal_range(const value_type &value) {
        const auto it = find(value);
        return {it, it + !(it == end())};
    }

    
    [[nodiscard]] std::pair<const_iterator, const_iterator> equal_range(const value_type &value) const {
        const auto it = find(value);
        return {it, it + !(it == cend())};
    }

    
    template<typename Other>
    [[nodiscard]] std::enable_if_t<is_transparent_v<hasher> && is_transparent_v<key_equal>, std::conditional_t<false, Other, std::pair<iterator, iterator>>>
    equal_range(const Other &value) {
        const auto it = find(value);
        return {it, it + !(it == end())};
    }

    
    template<typename Other>
    [[nodiscard]] std::enable_if_t<is_transparent_v<hasher> && is_transparent_v<key_equal>, std::conditional_t<false, Other, std::pair<const_iterator, const_iterator>>>
    equal_range(const Other &value) const {
        const auto it = find(value);
        return {it, it + !(it == cend())};
    }

    
    [[nodiscard]] bool contains(const value_type &value) const {
        return (find(value) != cend());
    }

    
    template<typename Other>
    [[nodiscard]] std::enable_if_t<is_transparent_v<hasher> && is_transparent_v<key_equal>, std::conditional_t<false, Other, bool>>
    contains(const Other &value) const {
        return (find(value) != cend());
    }

    
    [[nodiscard]] const_local_iterator cbegin(const size_type index) const {
        return {packed.first().begin(), sparse.first()[index]};
    }

    
    [[nodiscard]] const_local_iterator begin(const size_type index) const {
        return cbegin(index);
    }

    
    [[nodiscard]] local_iterator begin(const size_type index) {
        return {packed.first().begin(), sparse.first()[index]};
    }

    
    [[nodiscard]] const_local_iterator cend([[maybe_unused]] const size_type index) const {
        return {};
    }

    
    [[nodiscard]] const_local_iterator end(const size_type index) const {
        return cend(index);
    }

    
    [[nodiscard]] local_iterator end([[maybe_unused]] const size_type index) {
        return {};
    }

    
    [[nodiscard]] size_type bucket_count() const {
        return sparse.first().size();
    }

    
    [[nodiscard]] size_type max_bucket_count() const {
        return sparse.first().max_size();
    }

    
    [[nodiscard]] size_type bucket_size(const size_type index) const {
        return static_cast<size_type>(std::distance(begin(index), end(index)));
    }

    
    [[nodiscard]] size_type bucket(const value_type &value) const {
        return value_to_bucket(value);
    }

    
    [[nodiscard]] float load_factor() const {
        return static_cast<float>(size()) / static_cast<float>(bucket_count());
    }

    
    [[nodiscard]] float max_load_factor() const {
        return threshold;
    }

    
    void max_load_factor(const float value) {
        ENTT_ASSERT(value > 0.f, "Invalid load factor");
        threshold = value;
        rehash(0u);
    }

    
    void rehash(const size_type cnt) {
        auto value = cnt > minimum_capacity ? cnt : minimum_capacity;
        const auto cap = static_cast<size_type>(static_cast<float>(size()) / max_load_factor());
        value = value > cap ? value : cap;

        if(const auto sz = next_power_of_two(value); sz != bucket_count()) {
            sparse.first().resize(sz);

            for(auto &&elem: sparse.first()) {
                elem = placeholder_position;
            }

            for(size_type pos{}, last = size(); pos < last; ++pos) {
                const auto index = value_to_bucket(packed.first()[pos].second);
                packed.first()[pos].first = std::exchange(sparse.first()[index], pos);
            }
        }
    }

    
    void reserve(const size_type cnt) {
        packed.first().reserve(cnt);
        rehash(static_cast<size_type>(std::ceil(static_cast<float>(cnt) / max_load_factor())));
    }

    
    [[nodiscard]] hasher hash_function() const {
        return sparse.second();
    }

    
    [[nodiscard]] key_equal key_eq() const {
        return packed.second();
    }

private:
    compressed_pair<sparse_container_type, hasher> sparse;
    compressed_pair<packed_container_type, key_equal> packed;
    float threshold{default_threshold};
};

} 

#endif
