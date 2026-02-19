#ifndef ENTT_CONTAINER_TABLE_HPP
#define ENTT_CONTAINER_TABLE_HPP

#include <cstddef>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/iterator.hpp"
#include "fwd.hpp"

namespace entt {


namespace internal {

template<typename... It>
class table_iterator {
    template<typename...>
    friend class table_iterator;

public:
    using value_type = decltype(std::forward_as_tuple(*std::declval<It>()...));
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::random_access_iterator_tag;

    constexpr table_iterator() noexcept
        : it{} {}

    constexpr table_iterator(It... from) noexcept
        : it{from...} {}

    template<typename... Other, typename = std::enable_if_t<(std::is_constructible_v<It, Other> && ...)>>
    constexpr table_iterator(const table_iterator<Other...> &other) noexcept
        : table_iterator{std::get<Other>(other.it)...} {}

    constexpr table_iterator &operator++() noexcept {
        return (++std::get<It>(it), ...), *this;
    }

    constexpr table_iterator operator++(int) noexcept {
        const table_iterator orig = *this;
        return ++(*this), orig;
    }

    constexpr table_iterator &operator--() noexcept {
        return (--std::get<It>(it), ...), *this;
    }

    constexpr table_iterator operator--(int) noexcept {
        const table_iterator orig = *this;
        return operator--(), orig;
    }

    constexpr table_iterator &operator+=(const difference_type value) noexcept {
        return ((std::get<It>(it) += value), ...), *this;
    }

    constexpr table_iterator operator+(const difference_type value) const noexcept {
        table_iterator copy = *this;
        return (copy += value);
    }

    constexpr table_iterator &operator-=(const difference_type value) noexcept {
        return (*this += -value);
    }

    constexpr table_iterator operator-(const difference_type value) const noexcept {
        return (*this + -value);
    }

    [[nodiscard]] constexpr reference operator[](const difference_type value) const noexcept {
        return std::forward_as_tuple(std::get<It>(it)[value]...);
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept {
        return {operator[](0)};
    }

    [[nodiscard]] constexpr reference operator*() const noexcept {
        return operator[](0);
    }

    template<typename... Lhs, typename... Rhs>
    friend constexpr std::ptrdiff_t operator-(const table_iterator<Lhs...> &, const table_iterator<Rhs...> &) noexcept;

    template<typename... Lhs, typename... Rhs>
    friend constexpr bool operator==(const table_iterator<Lhs...> &, const table_iterator<Rhs...> &) noexcept;

    template<typename... Lhs, typename... Rhs>
    friend constexpr bool operator<(const table_iterator<Lhs...> &, const table_iterator<Rhs...> &) noexcept;

private:
    std::tuple<It...> it;
};

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr std::ptrdiff_t operator-(const table_iterator<Lhs...> &lhs, const table_iterator<Rhs...> &rhs) noexcept {
    return std::get<0>(lhs.it) - std::get<0>(rhs.it);
}

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator==(const table_iterator<Lhs...> &lhs, const table_iterator<Rhs...> &rhs) noexcept {
    return std::get<0>(lhs.it) == std::get<0>(rhs.it);
}

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator!=(const table_iterator<Lhs...> &lhs, const table_iterator<Rhs...> &rhs) noexcept {
    return !(lhs == rhs);
}

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator<(const table_iterator<Lhs...> &lhs, const table_iterator<Rhs...> &rhs) noexcept {
    return std::get<0>(lhs.it) < std::get<0>(rhs.it);
}

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator>(const table_iterator<Lhs...> &lhs, const table_iterator<Rhs...> &rhs) noexcept {
    return rhs < lhs;
}

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator<=(const table_iterator<Lhs...> &lhs, const table_iterator<Rhs...> &rhs) noexcept {
    return !(lhs > rhs);
}

template<typename... Lhs, typename... Rhs>
[[nodiscard]] constexpr bool operator>=(const table_iterator<Lhs...> &lhs, const table_iterator<Rhs...> &rhs) noexcept {
    return !(lhs < rhs);
}

} 



template<typename... Container>
class basic_table {
    using container_type = std::tuple<Container...>;

public:
    
    using size_type = std::size_t;
    
    using difference_type = std::ptrdiff_t;
    
    using iterator = internal::table_iterator<typename Container::iterator...>;
    
    using const_iterator = internal::table_iterator<typename Container::const_iterator...>;
    
    using reverse_iterator = internal::table_iterator<typename Container::reverse_iterator...>;
    
    using const_reverse_iterator = internal::table_iterator<typename Container::const_reverse_iterator...>;

    
    basic_table()
        : payload{} {
    }

    
    explicit basic_table(const Container &...container) noexcept
        : payload{container...} {
        ENTT_ASSERT((((std::get<Container>(payload).size() * sizeof...(Container)) == (std::get<Container>(payload).size() + ...)) && ...), "Unexpected container size");
    }

    
    explicit basic_table(Container &&...container) noexcept
        : payload{std::move(container)...} {
        ENTT_ASSERT((((std::get<Container>(payload).size() * sizeof...(Container)) == (std::get<Container>(payload).size() + ...)) && ...), "Unexpected container size");
    }

    
    basic_table(const basic_table &) = delete;

    
    basic_table(basic_table &&other) noexcept
        : payload{std::move(other.payload)} {}

    
    template<typename Allocator>
    explicit basic_table(const Allocator &allocator)
        : payload{Container{allocator}...} {}

    
    template<class Allocator>
    basic_table(const Container &...container, const Allocator &allocator) noexcept
        : payload{Container{container, allocator}...} {
        ENTT_ASSERT((((std::get<Container>(payload).size() * sizeof...(Container)) == (std::get<Container>(payload).size() + ...)) && ...), "Unexpected container size");
    }

    
    template<class Allocator>
    basic_table(Container &&...container, const Allocator &allocator) noexcept
        : payload{Container{std::move(container), allocator}...} {
        ENTT_ASSERT((((std::get<Container>(payload).size() * sizeof...(Container)) == (std::get<Container>(payload).size() + ...)) && ...), "Unexpected container size");
    }

    
    template<class Allocator>
    basic_table(basic_table &&other, const Allocator &allocator)
        : payload{Container{std::move(std::get<Container>(other.payload)), allocator}...} {}

    
    ~basic_table() = default;

    
    basic_table &operator=(const basic_table &) = delete;

    
    basic_table &operator=(basic_table &&other) noexcept {
        swap(other);
        return *this;
    }

    
    void swap(basic_table &other) noexcept {
        using std::swap;
        swap(payload, other.payload);
    }

    
    void reserve(const size_type cap) {
        (std::get<Container>(payload).reserve(cap), ...);
    }

    
    [[nodiscard]] size_type capacity() const noexcept {
        return std::get<0>(payload).capacity();
    }

    
    void shrink_to_fit() {
        (std::get<Container>(payload).shrink_to_fit(), ...);
    }

    
    [[nodiscard]] size_type size() const noexcept {
        return std::get<0>(payload).size();
    }

    
    [[nodiscard]] bool empty() const noexcept {
        return std::get<0>(payload).empty();
    }

    
    [[nodiscard]] const_iterator cbegin() const noexcept {
        return {std::get<Container>(payload).cbegin()...};
    }

    
    [[nodiscard]] const_iterator begin() const noexcept {
        return cbegin();
    }

    
    [[nodiscard]] iterator begin() noexcept {
        return {std::get<Container>(payload).begin()...};
    }

    
    [[nodiscard]] const_iterator cend() const noexcept {
        return {std::get<Container>(payload).cend()...};
    }

    
    [[nodiscard]] const_iterator end() const noexcept {
        return cend();
    }

    
    [[nodiscard]] iterator end() noexcept {
        return {std::get<Container>(payload).end()...};
    }

    
    [[nodiscard]] const_reverse_iterator crbegin() const noexcept {
        return {std::get<Container>(payload).crbegin()...};
    }

    
    [[nodiscard]] const_reverse_iterator rbegin() const noexcept {
        return crbegin();
    }

    
    [[nodiscard]] reverse_iterator rbegin() noexcept {
        return {std::get<Container>(payload).rbegin()...};
    }

    
    [[nodiscard]] const_reverse_iterator crend() const noexcept {
        return {std::get<Container>(payload).crend()...};
    }

    
    [[nodiscard]] const_reverse_iterator rend() const noexcept {
        return crend();
    }

    
    [[nodiscard]] reverse_iterator rend() noexcept {
        return {std::get<Container>(payload).rend()...};
    }

    
    template<typename... Args>
    std::tuple<typename Container::value_type &...> emplace(Args &&...args) {
        if constexpr(sizeof...(Args) == 0u) {
            return std::forward_as_tuple(std::get<Container>(payload).emplace_back()...);
        } else {
            return std::forward_as_tuple(std::get<Container>(payload).emplace_back(std::forward<Args>(args))...);
        }
    }

    
    iterator erase(const_iterator pos) {
        const auto diff = pos - begin();
        return {std::get<Container>(payload).erase(std::get<Container>(payload).begin() + diff)...};
    }

    
    void erase(const size_type pos) {
        ENTT_ASSERT(pos < size(), "Index out of bounds");
        erase(begin() + static_cast<difference_type>(pos));
    }

    
    [[nodiscard]] std::tuple<const typename Container::value_type &...> operator[](const size_type pos) const {
        ENTT_ASSERT(pos < size(), "Index out of bounds");
        return std::forward_as_tuple(std::get<Container>(payload)[pos]...);
    }

    
    [[nodiscard]] std::tuple<typename Container::value_type &...> operator[](const size_type pos) {
        ENTT_ASSERT(pos < size(), "Index out of bounds");
        return std::forward_as_tuple(std::get<Container>(payload)[pos]...);
    }

    
    void clear() {
        (std::get<Container>(payload).clear(), ...);
    }

private:
    container_type payload;
};

} 


namespace std {

template<typename... Container, typename Allocator>
struct uses_allocator<entt::basic_table<Container...>, Allocator>
    : std::bool_constant<(std::uses_allocator_v<Container, Allocator> && ...)> {};

} 


#endif
