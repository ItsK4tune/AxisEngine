#ifndef ENTT_CORE_BIT_HPP
#define ENTT_CORE_BIT_HPP

#include <cstddef>
#include <limits>
#include <type_traits>
#include "../config/config.h"

namespace entt {


template<typename Type>
[[nodiscard]] constexpr std::enable_if_t<std::is_unsigned_v<Type>, int> popcount(const Type value) noexcept {
    return value ? (int(value & 1) + popcount(static_cast<Type>(value >> 1))) : 0;
}


template<typename Type>
[[nodiscard]] constexpr std::enable_if_t<std::is_unsigned_v<Type>, bool> has_single_bit(const Type value) noexcept {
    return value && ((value & (value - 1)) == 0);
}


template<typename Type>
[[nodiscard]] constexpr std::enable_if_t<std::is_unsigned_v<Type>, Type> next_power_of_two(const Type value) noexcept {
    
    ENTT_ASSERT_CONSTEXPR(value < (Type{1u} << (std::numeric_limits<Type>::digits - 1)), "Numeric limits exceeded");
    Type curr = value - (value != 0u);

    for(int next = 1; next < std::numeric_limits<Type>::digits; next = next * 2) {
        curr |= (curr >> next);
    }

    return ++curr;
}


template<typename Type>
[[nodiscard]] constexpr std::enable_if_t<std::is_unsigned_v<Type>, Type> fast_mod(const Type value, const std::size_t mod) noexcept {
    ENTT_ASSERT_CONSTEXPR(has_single_bit(mod), "Value must be a power of two");
    return static_cast<Type>(value & (mod - 1u));
}

} 

#endif
