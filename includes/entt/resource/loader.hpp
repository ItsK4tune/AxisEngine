#ifndef ENTT_RESOURCE_LOADER_HPP
#define ENTT_RESOURCE_LOADER_HPP

#include <memory>
#include <utility>
#include "fwd.hpp"

namespace entt {


template<typename Type>
struct resource_loader {
    
    using result_type = std::shared_ptr<Type>;

    
    template<typename... Args>
    result_type operator()(Args &&...args) const {
        return std::make_shared<Type>(std::forward<Args>(args)...);
    }
};

} 

#endif
