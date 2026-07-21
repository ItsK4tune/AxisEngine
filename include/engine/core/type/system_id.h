#pragma once

#include <cstdint>
#include <string_view>

struct SystemId
{
    uint64_t value = 0;

    static constexpr SystemId FromName(std::string_view name)
    {
        uint64_t hash = 14695981039346656037ULL;
        for (const char character : name)
        {
            hash ^= static_cast<unsigned char>(character);
            hash *= 1099511628211ULL;
        }
        return {hash};
    }

    constexpr explicit operator bool() const
    {
        return value != 0;
    }
    constexpr bool operator==(const SystemId&) const = default;
};
