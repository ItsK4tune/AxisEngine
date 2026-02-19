#ifndef ENTT_CORE_MONOSTATE_HPP
#define ENTT_CORE_MONOSTATE_HPP

#include "../config/config.h"
#include "fwd.hpp"

namespace entt {


template<id_type>
struct monostate {
    
    template<typename Type>
    monostate &operator=(Type val) noexcept {
        value<Type> = val;
        return *this;
    }

    
    template<typename Type>
    operator Type() const noexcept {
        return value<Type>;
    }

private:
    template<typename Type>
    
    inline static ENTT_MAYBE_ATOMIC(Type) value{};
};


template<id_type Value>

inline monostate<Value> monostate_v{};

} 

#endif
