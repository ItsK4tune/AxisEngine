#ifndef ENTT_RESOURCE_RESOURCE_HPP
#define ENTT_RESOURCE_RESOURCE_HPP

#include <memory>
#include <type_traits>
#include <utility>
#include "fwd.hpp"

namespace entt {


template<typename Type>
class resource {
    template<typename>
    friend class resource;

    template<typename Other>
    static constexpr bool is_acceptable = !std::is_same_v<Type, Other> && std::is_constructible_v<Type &, Other &>;

public:
    
    using element_type = Type;
    
    using handle_type = std::shared_ptr<element_type>;

    
    resource() noexcept
        : value{} {}

    
    explicit resource(handle_type res) noexcept
        : value{std::move(res)} {}

    
    resource(const resource &) noexcept = default;

    
    resource(resource &&) noexcept = default;

    
    template<typename Other>
    resource(const resource<Other> &other, element_type &res) noexcept
        : value{other.value, std::addressof(res)} {}

    
    template<typename Other, typename = std::enable_if_t<is_acceptable<Other>>>
    resource(const resource<Other> &other) noexcept
        : value{other.value} {}

    
    template<typename Other, typename = std::enable_if_t<is_acceptable<Other>>>
    resource(resource<Other> &&other) noexcept
        : value{std::move(other.value)} {}

    
    ~resource() = default;

    
    resource &operator=(const resource &) noexcept = default;

    
    resource &operator=(resource &&) noexcept = default;

    
    template<typename Other, typename = std::enable_if_t<is_acceptable<Other>>>
    resource &operator=(const resource<Other> &other) noexcept {
        value = other.value;
        return *this;
    }

    
    template<typename Other, typename = std::enable_if_t<is_acceptable<Other>>>
    resource &operator=(resource<Other> &&other) noexcept {
        value = std::move(other.value);
        return *this;
    }

    
    void swap(resource &other) noexcept {
        using std::swap;
        swap(value, other.value);
    }

    
    [[nodiscard]] element_type &operator*() const noexcept {
        return *value;
    }

    
    [[nodiscard]] operator element_type &() const noexcept {
        return *value;
    }

    
    [[nodiscard]] element_type *operator->() const noexcept {
        return value.get();
    }

    
    [[nodiscard]] explicit operator bool() const noexcept {
        return static_cast<bool>(value);
    }

    
    void reset() {
        value.reset();
    }

    
    void reset(handle_type other) {
        value = std::move(other);
    }

    
    [[nodiscard]] handle_type handle() const noexcept {
        return value;
    }

private:
    handle_type value;
};


template<typename Lhs, typename Rhs>
[[nodiscard]] bool operator==(const resource<Lhs> &lhs, const resource<Rhs> &rhs) noexcept {
    return (std::addressof(*lhs) == std::addressof(*rhs));
}


template<typename Lhs, typename Rhs>
[[nodiscard]] bool operator!=(const resource<Lhs> &lhs, const resource<Rhs> &rhs) noexcept {
    return !(lhs == rhs);
}


template<typename Lhs, typename Rhs>
[[nodiscard]] bool operator<(const resource<Lhs> &lhs, const resource<Rhs> &rhs) noexcept {
    return (std::addressof(*lhs) < std::addressof(*rhs));
}


template<typename Lhs, typename Rhs>
[[nodiscard]] bool operator>(const resource<Lhs> &lhs, const resource<Rhs> &rhs) noexcept {
    return rhs < lhs;
}


template<typename Lhs, typename Rhs>
[[nodiscard]] bool operator<=(const resource<Lhs> &lhs, const resource<Rhs> &rhs) noexcept {
    return !(lhs > rhs);
}


template<typename Lhs, typename Rhs>
[[nodiscard]] bool operator>=(const resource<Lhs> &lhs, const resource<Rhs> &rhs) noexcept {
    return !(lhs < rhs);
}

} 

#endif
