#ifndef ENTT_CORE_TYPE_INFO_HPP
#define ENTT_CORE_TYPE_INFO_HPP

#include <string_view>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "fwd.hpp"
#include "hashed_string.hpp"

namespace entt {


namespace internal {

struct ENTT_API type_index final {
    [[nodiscard]] static id_type next() noexcept {
        static ENTT_MAYBE_ATOMIC(id_type) value{};
        return value++;
    }
};

template<typename Type>
[[nodiscard]] constexpr const char *pretty_function() noexcept {
#if defined ENTT_PRETTY_FUNCTION
    return static_cast<const char *>(ENTT_PRETTY_FUNCTION);
#else
    return "";
#endif
}

template<typename Type>
[[nodiscard]] constexpr auto stripped_type_name() noexcept {
#if defined ENTT_PRETTY_FUNCTION
    const std::string_view full_name{pretty_function<Type>()};
    auto first = full_name.find_first_not_of(' ', full_name.find_first_of(ENTT_PRETTY_FUNCTION_PREFIX) + 1);
    auto value = full_name.substr(first, full_name.find_last_of(ENTT_PRETTY_FUNCTION_SUFFIX) - first);
    return value;
#else
    return std::string_view{};
#endif
}

template<typename Type, auto = stripped_type_name<Type>().find_first_of('.')>
[[nodiscard]] constexpr std::string_view type_name(int) noexcept {
    constexpr auto value = stripped_type_name<Type>();
    return value;
}

template<typename Type>
[[nodiscard]] std::string_view type_name(char) noexcept {
    static const auto value = stripped_type_name<Type>();
    return value;
}

template<typename Type, auto = stripped_type_name<Type>().find_first_of('.')>
[[nodiscard]] constexpr id_type type_hash(int) noexcept {
    constexpr auto stripped = stripped_type_name<Type>();
    constexpr auto value = hashed_string::value(stripped.data(), stripped.size());
    return value;
}

template<typename Type>
[[nodiscard]] id_type type_hash(char) noexcept {
    static const auto value = [](const auto stripped) {
        return hashed_string::value(stripped.data(), stripped.size());
    }(stripped_type_name<Type>());
    return value;
}

} 



template<typename Type, typename = void>
struct ENTT_API type_index final {
    
    [[nodiscard]] static id_type value() noexcept {
        static const id_type value = internal::type_index::next();
        return value;
    }

    
    [[nodiscard]] constexpr operator id_type() const noexcept {
        return value();
    }
};


template<typename Type, typename = void>
struct type_hash final {
    
#if defined ENTT_PRETTY_FUNCTION
    [[nodiscard]] static constexpr id_type value() noexcept {
        return internal::type_hash<Type>(0);
#else
    [[nodiscard]] static constexpr id_type value() noexcept {
        return type_index<Type>::value();
#endif
    }

    
    [[nodiscard]] constexpr operator id_type() const noexcept {
        return value();
    }
};


template<typename Type, typename = void>
struct type_name final {
    
    [[nodiscard]] static constexpr std::string_view value() noexcept {
        return internal::type_name<Type>(0);
    }

    
    [[nodiscard]] constexpr operator std::string_view() const noexcept {
        return value();
    }
};


struct type_info final {
    
    template<typename Type>
    
    constexpr type_info(std::in_place_type_t<Type>) noexcept
        : seq{type_index<std::remove_const_t<std::remove_reference_t<Type>>>::value()},
          identifier{type_hash<std::remove_const_t<std::remove_reference_t<Type>>>::value()},
          alias{type_name<std::remove_const_t<std::remove_reference_t<Type>>>::value()} {}
    

    
    [[nodiscard]] constexpr id_type index() const noexcept {
        return seq;
    }

    
    [[nodiscard]] constexpr id_type hash() const noexcept {
        return identifier;
    }

    
    [[nodiscard]] constexpr std::string_view name() const noexcept {
        return alias;
    }

private:
    id_type seq;
    id_type identifier;
    std::string_view alias;
};


[[nodiscard]] constexpr bool operator==(const type_info &lhs, const type_info &rhs) noexcept {
    return lhs.hash() == rhs.hash();
}


[[nodiscard]] constexpr bool operator!=(const type_info &lhs, const type_info &rhs) noexcept {
    return !(lhs == rhs);
}


[[nodiscard]] constexpr bool operator<(const type_info &lhs, const type_info &rhs) noexcept {
    return lhs.index() < rhs.index();
}


[[nodiscard]] constexpr bool operator<=(const type_info &lhs, const type_info &rhs) noexcept {
    return !(rhs < lhs);
}


[[nodiscard]] constexpr bool operator>(const type_info &lhs, const type_info &rhs) noexcept {
    return rhs < lhs;
}


[[nodiscard]] constexpr bool operator>=(const type_info &lhs, const type_info &rhs) noexcept {
    return !(lhs < rhs);
}


template<typename Type>
[[nodiscard]] const type_info &type_id() noexcept {
    if constexpr(std::is_same_v<Type, std::remove_const_t<std::remove_reference_t<Type>>>) {
        static const type_info instance{std::in_place_type<Type>};
        return instance;
    } else {
        return type_id<std::remove_const_t<std::remove_reference_t<Type>>>();
    }
}


template<typename Type>

[[nodiscard]] const type_info &type_id(Type &&) noexcept {
    return type_id<std::remove_const_t<std::remove_reference_t<Type>>>();
}

} 

#endif
