#ifndef ENTT_CORE_HASHED_STRING_HPP
#define ENTT_CORE_HASHED_STRING_HPP

#include <cstddef>
#include <cstdint>
#include "fwd.hpp"

namespace entt {


namespace internal {

template<typename = id_type>
struct fnv_1a_params;

template<>
struct fnv_1a_params<std::uint32_t> {
    static constexpr auto offset = 2166136261;
    static constexpr auto prime = 16777619;
};

template<>
struct fnv_1a_params<std::uint64_t> {
    static constexpr auto offset = 14695981039346656037ull;
    static constexpr auto prime = 1099511628211ull;
};

template<typename Char>
struct basic_hashed_string {
    using value_type = Char;
    using size_type = std::size_t;
    using hash_type = id_type;

    const value_type *repr{};
    hash_type hash{fnv_1a_params<>::offset};
    size_type length{};
};

} 



template<typename Char>
class basic_hashed_string: internal::basic_hashed_string<Char> {
    using base_type = internal::basic_hashed_string<Char>;
    using params = internal::fnv_1a_params<>;

    struct const_wrapper {
        
        constexpr const_wrapper(const typename base_type::value_type *str) noexcept
            : repr{str} {}

        const typename base_type::value_type *repr;
    };

public:
    
    using value_type = typename base_type::value_type;
    
    using size_type = typename base_type::size_type;
    
    using hash_type = typename base_type::hash_type;

    
    [[nodiscard]] static constexpr hash_type value(const value_type *str, const size_type len) noexcept {
        return basic_hashed_string{str, len};
    }

    
    template<std::size_t N>
    
    [[nodiscard]] static ENTT_CONSTEVAL hash_type value(const value_type (&str)[N]) noexcept {
        return basic_hashed_string{str};
    }

    
    [[nodiscard]] static constexpr hash_type value(const_wrapper wrapper) noexcept {
        return basic_hashed_string{wrapper};
    }

    
    constexpr basic_hashed_string() noexcept
        : basic_hashed_string{nullptr, 0u} {}

    
    constexpr basic_hashed_string(const value_type *str, const size_type len) noexcept
        
        : base_type{str} {
        
        for(; base_type::length < len; ++base_type::length) {
            base_type::hash = (base_type::hash ^ static_cast<id_type>(str[base_type::length])) * params::prime;
        }
        
    }

    
    template<std::size_t N>
    
    ENTT_CONSTEVAL basic_hashed_string(const value_type (&str)[N]) noexcept
        
        : base_type{str} {
        for(; str[base_type::length]; ++base_type::length) {
            base_type::hash = (base_type::hash ^ static_cast<id_type>(str[base_type::length])) * params::prime;
        }
    }

    
    explicit constexpr basic_hashed_string(const_wrapper wrapper) noexcept
        : base_type{wrapper.repr} {
        
        for(; wrapper.repr[base_type::length]; ++base_type::length) {
            base_type::hash = (base_type::hash ^ static_cast<id_type>(wrapper.repr[base_type::length])) * params::prime;
        }
        
    }

    
    [[nodiscard]] constexpr size_type size() const noexcept {
        return base_type::length;
    }

    
    [[nodiscard]] constexpr const value_type *data() const noexcept {
        return base_type::repr;
    }

    
    [[nodiscard]] constexpr hash_type value() const noexcept {
        return base_type::hash;
    }

    
    [[nodiscard]] explicit constexpr operator const value_type *() const noexcept {
        return data();
    }

    
    [[nodiscard]] constexpr operator hash_type() const noexcept {
        return value();
    }
};


template<typename Char>
basic_hashed_string(const Char *str, std::size_t len) -> basic_hashed_string<Char>;


template<typename Char, std::size_t N>

basic_hashed_string(const Char (&str)[N]) -> basic_hashed_string<Char>;


template<typename Char>
[[nodiscard]] constexpr bool operator==(const basic_hashed_string<Char> &lhs, const basic_hashed_string<Char> &rhs) noexcept {
    return lhs.value() == rhs.value();
}


template<typename Char>
[[nodiscard]] constexpr bool operator!=(const basic_hashed_string<Char> &lhs, const basic_hashed_string<Char> &rhs) noexcept {
    return !(lhs == rhs);
}


template<typename Char>
[[nodiscard]] constexpr bool operator<(const basic_hashed_string<Char> &lhs, const basic_hashed_string<Char> &rhs) noexcept {
    return lhs.value() < rhs.value();
}


template<typename Char>
[[nodiscard]] constexpr bool operator<=(const basic_hashed_string<Char> &lhs, const basic_hashed_string<Char> &rhs) noexcept {
    return !(rhs < lhs);
}


template<typename Char>
[[nodiscard]] constexpr bool operator>(const basic_hashed_string<Char> &lhs, const basic_hashed_string<Char> &rhs) noexcept {
    return rhs < lhs;
}


template<typename Char>
[[nodiscard]] constexpr bool operator>=(const basic_hashed_string<Char> &lhs, const basic_hashed_string<Char> &rhs) noexcept {
    return !(lhs < rhs);
}

inline namespace literals {


[[nodiscard]] ENTT_CONSTEVAL hashed_string operator""_hs(const char *str, std::size_t) noexcept {
    return hashed_string{str};
}


[[nodiscard]] ENTT_CONSTEVAL hashed_wstring operator""_hws(const wchar_t *str, std::size_t) noexcept {
    return hashed_wstring{str};
}

} 

} 

#endif
