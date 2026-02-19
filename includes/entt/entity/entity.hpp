#ifndef ENTT_ENTITY_ENTITY_HPP
#define ENTT_ENTITY_ENTITY_HPP

#include <cstddef>
#include <cstdint>
#include <type_traits>
#include "../config/config.h"
#include "../core/bit.hpp"
#include "fwd.hpp"

namespace entt {


namespace internal {

template<typename, typename = void>
struct entt_traits;

template<typename Type>
struct entt_traits<Type, std::enable_if_t<std::is_enum_v<Type>>>
    : entt_traits<std::underlying_type_t<Type>> {
    using value_type = Type;
};

template<typename Type>
struct entt_traits<Type, std::enable_if_t<std::is_class_v<Type>>>
    : entt_traits<typename Type::entity_type> {
    using value_type = Type;
};

template<>
struct entt_traits<std::uint32_t> {
    using value_type = std::uint32_t;

    using entity_type = std::uint32_t;
    using version_type = std::uint16_t;

    static constexpr entity_type entity_mask = 0xFFFFF;
    static constexpr entity_type version_mask = 0xFFF;
};

template<>
struct entt_traits<std::uint64_t> {
    using value_type = std::uint64_t;

    using entity_type = std::uint64_t;
    using version_type = std::uint32_t;

    static constexpr entity_type entity_mask = 0xFFFFFFFF;
    static constexpr entity_type version_mask = 0xFFFFFFFF;
};

} 



template<typename Traits>
class basic_entt_traits {
    static constexpr auto length = popcount(Traits::entity_mask);

    static_assert(Traits::entity_mask && ((Traits::entity_mask & (Traits::entity_mask + 1)) == 0), "Invalid entity mask");
    static_assert((Traits::version_mask & (Traits::version_mask + 1)) == 0, "Invalid version mask");

public:
    
    using value_type = typename Traits::value_type;
    
    using entity_type = typename Traits::entity_type;
    
    using version_type = typename Traits::version_type;

    
    static constexpr entity_type entity_mask = Traits::entity_mask;
    
    static constexpr entity_type version_mask = Traits::version_mask;

    
    [[nodiscard]] static constexpr entity_type to_integral(const value_type value) noexcept {
        return static_cast<entity_type>(value);
    }

    
    [[nodiscard]] static constexpr entity_type to_entity(const value_type value) noexcept {
        return (to_integral(value) & entity_mask);
    }

    
    [[nodiscard]] static constexpr version_type to_version(const value_type value) noexcept {
        if constexpr(Traits::version_mask == 0u) {
            return version_type{};
        } else {
            return (static_cast<version_type>(to_integral(value) >> length) & version_mask);
        }
    }

    
    [[nodiscard]] static constexpr value_type next(const value_type value) noexcept {
        const auto vers = to_version(value) + 1;
        return construct(to_integral(value), static_cast<version_type>(vers + (vers == version_mask)));
    }

    
    [[nodiscard]] static constexpr value_type construct(const entity_type entity, const version_type version) noexcept {
        if constexpr(Traits::version_mask == 0u) {
            return value_type{entity & entity_mask};
        } else {
            return value_type{(entity & entity_mask) | (static_cast<entity_type>(version & version_mask) << length)};
        }
    }

    
    [[nodiscard]] static constexpr value_type combine(const entity_type lhs, const entity_type rhs) noexcept {
        if constexpr(Traits::version_mask == 0u) {
            return value_type{lhs & entity_mask};
        } else {
            return value_type{(lhs & entity_mask) | (rhs & (version_mask << length))};
        }
    }
};


template<typename Type>
struct entt_traits: basic_entt_traits<internal::entt_traits<Type>> {
    
    using base_type = basic_entt_traits<internal::entt_traits<Type>>;
    
    static constexpr std::size_t page_size = ENTT_SPARSE_PAGE;
};


template<typename Entity>
[[nodiscard]] constexpr typename entt_traits<Entity>::entity_type to_integral(const Entity value) noexcept {
    return entt_traits<Entity>::to_integral(value);
}


template<typename Entity>
[[nodiscard]] constexpr typename entt_traits<Entity>::entity_type to_entity(const Entity value) noexcept {
    return entt_traits<Entity>::to_entity(value);
}


template<typename Entity>
[[nodiscard]] constexpr typename entt_traits<Entity>::version_type to_version(const Entity value) noexcept {
    return entt_traits<Entity>::to_version(value);
}


struct null_t {
    
    template<typename Entity>
    [[nodiscard]] constexpr operator Entity() const noexcept {
        using traits_type = entt_traits<Entity>;
        constexpr auto value = traits_type::construct(traits_type::entity_mask, traits_type::version_mask);
        return value;
    }

    
    [[nodiscard]] constexpr bool operator==([[maybe_unused]] const null_t other) const noexcept {
        return true;
    }

    
    [[nodiscard]] constexpr bool operator!=([[maybe_unused]] const null_t other) const noexcept {
        return false;
    }

    
    template<typename Entity>
    [[nodiscard]] constexpr bool operator==(const Entity entity) const noexcept {
        using traits_type = entt_traits<Entity>;
        return traits_type::to_entity(entity) == traits_type::to_entity(*this);
    }

    
    template<typename Entity>
    [[nodiscard]] constexpr bool operator!=(const Entity entity) const noexcept {
        return !(entity == *this);
    }
};


template<typename Entity>
[[nodiscard]] constexpr bool operator==(const Entity lhs, const null_t rhs) noexcept {
    return rhs.operator==(lhs);
}


template<typename Entity>
[[nodiscard]] constexpr bool operator!=(const Entity lhs, const null_t rhs) noexcept {
    return !(rhs == lhs);
}


struct tombstone_t {
    
    template<typename Entity>
    [[nodiscard]] constexpr operator Entity() const noexcept {
        using traits_type = entt_traits<Entity>;
        constexpr auto value = traits_type::construct(traits_type::entity_mask, traits_type::version_mask);
        return value;
    }

    
    [[nodiscard]] constexpr bool operator==([[maybe_unused]] const tombstone_t other) const noexcept {
        return true;
    }

    
    [[nodiscard]] constexpr bool operator!=([[maybe_unused]] const tombstone_t other) const noexcept {
        return false;
    }

    
    template<typename Entity>
    [[nodiscard]] constexpr bool operator==(const Entity entity) const noexcept {
        using traits_type = entt_traits<Entity>;

        if constexpr(traits_type::version_mask == 0u) {
            return false;
        } else {
            return (traits_type::to_version(entity) == traits_type::to_version(*this));
        }
    }

    
    template<typename Entity>
    [[nodiscard]] constexpr bool operator!=(const Entity entity) const noexcept {
        return !(entity == *this);
    }
};


template<typename Entity>
[[nodiscard]] constexpr bool operator==(const Entity lhs, const tombstone_t rhs) noexcept {
    return rhs.operator==(lhs);
}


template<typename Entity>
[[nodiscard]] constexpr bool operator!=(const Entity lhs, const tombstone_t rhs) noexcept {
    return !(rhs == lhs);
}


inline constexpr null_t null{};


inline constexpr tombstone_t tombstone{};

} 

#endif
