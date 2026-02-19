#ifndef ENTT_SIGNAL_DISPATCHER_HPP
#define ENTT_SIGNAL_DISPATCHER_HPP

#include <cstddef>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include "../container/dense_map.hpp"
#include "../core/compressed_pair.hpp"
#include "../core/fwd.hpp"
#include "../core/type_info.hpp"
#include "../core/utility.hpp"
#include "fwd.hpp"
#include "sigh.hpp"

namespace entt {


namespace internal {

struct basic_dispatcher_handler {
    virtual ~basic_dispatcher_handler() = default;
    virtual void publish() = 0;
    virtual void disconnect(void *) = 0;
    virtual void clear() noexcept = 0;
    [[nodiscard]] virtual std::size_t size() const noexcept = 0;
};

template<typename Type, typename Allocator>
class dispatcher_handler final: public basic_dispatcher_handler {
    static_assert(std::is_same_v<Type, std::decay_t<Type>>, "Invalid type");

    using alloc_traits = std::allocator_traits<Allocator>;
    using signal_type = sigh<void(Type &), Allocator>;
    using container_type = std::vector<Type, typename alloc_traits::template rebind_alloc<Type>>;

public:
    using allocator_type = Allocator;

    dispatcher_handler(const allocator_type &allocator)
        : signal{allocator},
          events{allocator} {}

    void publish() override {
        const auto length = events.size();

        for(std::size_t pos{}; pos < length; ++pos) {
            signal.publish(events[pos]);
        }

        events.erase(events.cbegin(), events.cbegin() + static_cast<typename container_type::difference_type>(length));
    }

    void disconnect(void *instance) override {
        bucket().disconnect(instance);
    }

    void clear() noexcept override {
        events.clear();
    }

    [[nodiscard]] auto bucket() noexcept {
        return typename signal_type::sink_type{signal};
    }

    void trigger(Type event) {
        signal.publish(event);
    }

    template<typename... Args>
    void enqueue(Args &&...args) {
        if constexpr(std::is_aggregate_v<Type> && (sizeof...(Args) != 0u || !std::is_default_constructible_v<Type>)) {
            events.push_back(Type{std::forward<Args>(args)...});
        } else {
            events.emplace_back(std::forward<Args>(args)...);
        }
    }

    [[nodiscard]] std::size_t size() const noexcept override {
        return events.size();
    }

private:
    signal_type signal;
    container_type events;
};

} 



template<typename Allocator>
class basic_dispatcher {
    template<typename Type>
    using handler_type = internal::dispatcher_handler<Type, Allocator>;

    using key_type = id_type;
    
    using mapped_type = std::shared_ptr<internal::basic_dispatcher_handler>;

    using alloc_traits = std::allocator_traits<Allocator>;
    using container_allocator = typename alloc_traits::template rebind_alloc<std::pair<const key_type, mapped_type>>;
    using container_type = dense_map<key_type, mapped_type, identity, std::equal_to<>, container_allocator>;

    template<typename Type>
    [[nodiscard]] handler_type<Type> &assure(const id_type id) {
        static_assert(std::is_same_v<Type, std::decay_t<Type>>, "Non-decayed types not allowed");
        auto &&ptr = pools.first()[id];

        if(!ptr) {
            const auto &allocator = get_allocator();
            ptr = std::allocate_shared<handler_type<Type>>(allocator, allocator);
        }

        return static_cast<handler_type<Type> &>(*ptr);
    }

    template<typename Type>
    [[nodiscard]] const handler_type<Type> *assure(const id_type id) const {
        static_assert(std::is_same_v<Type, std::decay_t<Type>>, "Non-decayed types not allowed");

        if(auto it = pools.first().find(id); it != pools.first().cend()) {
            return static_cast<const handler_type<Type> *>(it->second.get());
        }

        return nullptr;
    }

public:
    
    using allocator_type = Allocator;
    
    using size_type = std::size_t;

    
    basic_dispatcher()
        : basic_dispatcher{allocator_type{}} {}

    
    explicit basic_dispatcher(const allocator_type &allocator)
        : pools{allocator, allocator} {}

    
    basic_dispatcher(const basic_dispatcher &) = delete;

    
    basic_dispatcher(basic_dispatcher &&other) noexcept
        : pools{std::move(other.pools)} {}

    
    basic_dispatcher(basic_dispatcher &&other, const allocator_type &allocator)
        : pools{container_type{std::move(other.pools.first()), allocator}, allocator} {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || get_allocator() == other.get_allocator(), "Copying a dispatcher is not allowed");
    }

    
    ~basic_dispatcher() = default;

    
    basic_dispatcher &operator=(const basic_dispatcher &) = delete;

    
    basic_dispatcher &operator=(basic_dispatcher &&other) noexcept {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || get_allocator() == other.get_allocator(), "Copying a dispatcher is not allowed");
        swap(other);
        return *this;
    }

    
    void swap(basic_dispatcher &other) noexcept {
        using std::swap;
        swap(pools, other.pools);
    }

    
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return pools.second();
    }

    
    template<typename Type>
    [[nodiscard]] size_type size(const id_type id = type_hash<Type>::value()) const noexcept {
        const auto *cpool = assure<std::decay_t<Type>>(id);
        return cpool ? cpool->size() : 0u;
    }

    
    [[nodiscard]] size_type size() const noexcept {
        size_type count{};

        for(auto &&cpool: pools.first()) {
            count += cpool.second->size();
        }

        return count;
    }

    
    template<typename Type>
    [[nodiscard]] auto sink(const id_type id = type_hash<Type>::value()) {
        return assure<Type>(id).bucket();
    }

    
    template<typename Type>
    void trigger(Type &&value = {}) {
        trigger(type_hash<std::decay_t<Type>>::value(), std::forward<Type>(value));
    }

    
    template<typename Type>
    void trigger(const id_type id, Type &&value = {}) {
        assure<std::decay_t<Type>>(id).trigger(std::forward<Type>(value));
    }

    
    template<typename Type, typename... Args>
    void enqueue(Args &&...args) {
        enqueue_hint<Type>(type_hash<Type>::value(), std::forward<Args>(args)...);
    }

    
    template<typename Type>
    void enqueue(Type &&value) {
        enqueue_hint(type_hash<std::decay_t<Type>>::value(), std::forward<Type>(value));
    }

    
    template<typename Type, typename... Args>
    void enqueue_hint(const id_type id, Args &&...args) {
        assure<Type>(id).enqueue(std::forward<Args>(args)...);
    }

    
    template<typename Type>
    void enqueue_hint(const id_type id, Type &&value) {
        assure<std::decay_t<Type>>(id).enqueue(std::forward<Type>(value));
    }

    
    template<typename Type>
    void disconnect(Type &value_or_instance) {
        disconnect(&value_or_instance);
    }

    
    template<typename Type>
    void disconnect(Type *value_or_instance) {
        for(auto &&cpool: pools.first()) {
            cpool.second->disconnect(value_or_instance);
        }
    }

    
    template<typename Type>
    void clear(const id_type id = type_hash<Type>::value()) {
        assure<Type>(id).clear();
    }

    
    void clear() noexcept {
        for(auto &&cpool: pools.first()) {
            cpool.second->clear();
        }
    }

    
    template<typename Type>
    void update(const id_type id = type_hash<Type>::value()) {
        assure<Type>(id).publish();
    }

    
    void update() const {
        for(auto &&cpool: pools.first()) {
            cpool.second->publish();
        }
    }

private:
    compressed_pair<container_type, allocator_type> pools;
};

} 

#endif
