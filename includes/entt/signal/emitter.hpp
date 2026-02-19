#ifndef ENTT_SIGNAL_EMITTER_HPP
#define ENTT_SIGNAL_EMITTER_HPP

#include <functional>
#include <type_traits>
#include <utility>
#include "../container/dense_map.hpp"
#include "../core/compressed_pair.hpp"
#include "../core/fwd.hpp"
#include "../core/type_info.hpp"
#include "../core/utility.hpp"
#include "fwd.hpp"

namespace entt {


template<typename Derived, typename Allocator>
class emitter {
    using key_type = id_type;
    using mapped_type = std::function<void(void *)>;

    using alloc_traits = std::allocator_traits<Allocator>;
    using container_allocator = typename alloc_traits::template rebind_alloc<std::pair<const key_type, mapped_type>>;
    using container_type = dense_map<key_type, mapped_type, identity, std::equal_to<>, container_allocator>;

public:
    
    using allocator_type = Allocator;
    
    using size_type = std::size_t;

    
    emitter()
        : emitter{allocator_type{}} {}

    
    explicit emitter(const allocator_type &allocator)
        : handlers{allocator, allocator} {}

    
    emitter(const emitter &) = delete;

    
    emitter(emitter &&other) noexcept
        : handlers{std::move(other.handlers)} {}

    
    emitter(emitter &&other, const allocator_type &allocator)
        : handlers{container_type{std::move(other.handlers.first()), allocator}, allocator} {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || handlers.second() == other.handlers.second(), "Copying an emitter is not allowed");
    }

    
    virtual ~emitter() {
        static_assert(std::is_base_of_v<emitter<Derived, Allocator>, Derived>, "Invalid emitter type");
    }

    
    emitter &operator=(const emitter &) = delete;

    
    emitter &operator=(emitter &&other) noexcept {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || handlers.second() == other.handlers.second(), "Copying an emitter is not allowed");
        swap(other);
        return *this;
    }

    
    void swap(emitter &other) noexcept {
        using std::swap;
        swap(handlers, other.handlers);
    }

    
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return handlers.second();
    }

    
    template<typename Type>
    void publish(Type value) {
        if(const auto id = type_id<Type>().hash(); handlers.first().contains(id)) {
            handlers.first()[id](&value);
        }
    }

    
    template<typename Type>
    void on(std::function<void(Type &, Derived &)> func) {
        handlers.first().insert_or_assign(type_id<Type>().hash(), [func = std::move(func), this](void *value) {
            func(*static_cast<Type *>(value), static_cast<Derived &>(*this));
        });
    }

    
    template<typename Type>
    void erase() {
        handlers.first().erase(type_hash<std::remove_const_t<std::remove_reference_t<Type>>>::value());
    }

    
    void clear() noexcept {
        handlers.first().clear();
    }

    
    template<typename Type>
    [[nodiscard]] bool contains() const {
        return handlers.first().contains(type_hash<std::remove_const_t<std::remove_reference_t<Type>>>::value());
    }

    
    [[nodiscard]] bool empty() const noexcept {
        return handlers.first().empty();
    }

private:
    compressed_pair<container_type, allocator_type> handlers;
};

} 

#endif
