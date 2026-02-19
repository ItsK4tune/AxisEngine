#ifndef ENTT_CORE_ITERATOR_HPP
#define ENTT_CORE_ITERATOR_HPP

#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>

namespace entt {


template<typename Type>
struct input_iterator_pointer final {
    
    using value_type = Type;
    
    using pointer = Type *;
    
    using reference = Type &;

    
    constexpr input_iterator_pointer(value_type &&val) noexcept(std::is_nothrow_move_constructible_v<value_type>)
        : value{std::move(val)} {}

    
    [[nodiscard]] constexpr pointer operator->() noexcept {
        return std::addressof(value);
    }

    
    [[nodiscard]] constexpr reference operator*() noexcept {
        return value;
    }

private:
    Type value;
};


template<typename Type>
class iota_iterator final {
    static_assert(std::is_integral_v<Type>, "Not an integral type");

public:
    
    using value_type = Type;
    
    using pointer = void;
    
    using reference = value_type;
    
    using difference_type = std::ptrdiff_t;
    
    using iterator_category = std::input_iterator_tag;

    
    constexpr iota_iterator() noexcept
        : current{} {}

    
    constexpr iota_iterator(const value_type init) noexcept
        : current{init} {}

    
    constexpr iota_iterator &operator++() noexcept {
        return ++current, *this;
    }

    
    constexpr iota_iterator operator++(int) noexcept {
        const iota_iterator orig = *this;
        return ++(*this), orig;
    }

    
    [[nodiscard]] constexpr reference operator*() const noexcept {
        return current;
    }

private:
    value_type current;
};


template<typename Type>
[[nodiscard]] constexpr bool operator==(const iota_iterator<Type> &lhs, const iota_iterator<Type> &rhs) noexcept {
    return *lhs == *rhs;
}


template<typename Type>
[[nodiscard]] constexpr bool operator!=(const iota_iterator<Type> &lhs, const iota_iterator<Type> &rhs) noexcept {
    return !(lhs == rhs);
}


template<typename It, typename Sentinel = It>
struct iterable_adaptor final {
    
    using value_type = typename std::iterator_traits<It>::value_type;
    
    using iterator = It;
    
    using sentinel = Sentinel;

    
    constexpr iterable_adaptor() noexcept(std::is_nothrow_default_constructible_v<iterator> && std::is_nothrow_default_constructible_v<sentinel>)
        : first{},
          last{} {}

    
    constexpr iterable_adaptor(iterator from, sentinel to) noexcept(std::is_nothrow_move_constructible_v<iterator> && std::is_nothrow_move_constructible_v<sentinel>)
        : first{std::move(from)},
          last{std::move(to)} {}

    
    [[nodiscard]] constexpr iterator begin() const noexcept {
        return first;
    }

    
    [[nodiscard]] constexpr sentinel end() const noexcept {
        return last;
    }

    
    [[nodiscard]] constexpr iterator cbegin() const noexcept {
        return begin();
    }

    
    [[nodiscard]] constexpr sentinel cend() const noexcept {
        return end();
    }

private:
    It first;
    Sentinel last;
};

} 

#endif
