#ifndef ENTT_CORE_FAMILY_HPP
#define ENTT_CORE_FAMILY_HPP

#include "../config/config.h"
#include "fwd.hpp"

namespace entt {


template<typename...>
class family {
    static auto identifier() noexcept {
        static ENTT_MAYBE_ATOMIC(id_type) value{};
        return value++;
    }

public:
    
    using value_type = id_type;

    
    template<typename... Type>
    
    inline static const value_type value = identifier();
};

} 

#endif
