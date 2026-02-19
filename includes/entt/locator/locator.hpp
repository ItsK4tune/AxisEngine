#ifndef ENTT_LOCATOR_LOCATOR_HPP
#define ENTT_LOCATOR_LOCATOR_HPP

#include <memory>
#include <utility>
#include "../config/config.h"

namespace entt {


template<typename Service>
class locator final {
    class service_handle {
        friend class locator<Service>;
        std::shared_ptr<Service> value{};
    };

public:
    
    using type = Service;
    
    using node_type = service_handle;

    
    locator() = delete;

    
    locator(const locator &) = delete;

    
    ~locator() = delete;

    
    locator &operator=(const locator &) = delete;

    
    [[nodiscard]] static bool has_value() noexcept {
        return (service != nullptr);
    }

    
    [[nodiscard]] static Service &value() noexcept {
        ENTT_ASSERT(has_value(), "Service not available");
        return *service;
    }

    
    template<typename Type = Service, typename... Args>
    [[nodiscard]] static Service &value_or(Args &&...args) {
        return service ? *service : emplace<Type>(std::forward<Args>(args)...);
    }

    
    template<typename Type = Service, typename... Args>
    static Service &emplace(Args &&...args) {
        service = std::make_shared<Type>(std::forward<Args>(args)...);
        return *service;
    }

    
    template<typename Type = Service, typename Allocator, typename... Args>
    static Service &emplace(std::allocator_arg_t, Allocator alloc, Args &&...args) {
        service = std::allocate_shared<Type>(alloc, std::forward<Args>(args)...);
        return *service;
    }

    
    static node_type handle() noexcept {
        node_type node{};
        node.value = service;
        return node;
    }

    
    static void reset(const node_type &other = {}) noexcept {
        service = other.value;
    }

    
    template<typename Type, typename Deleter = std::default_delete<Type>>
    static void reset(Type *elem, Deleter deleter = {}) {
        service = std::shared_ptr<Service>{elem, std::move(deleter)};
    }

private:
    
    
    inline static std::shared_ptr<Service> service{};
};

} 

#endif
