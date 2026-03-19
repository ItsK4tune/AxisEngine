#ifndef ENTT_PROCESS_SCHEDULER_HPP
#define ENTT_PROCESS_SCHEDULER_HPP

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "../core/compressed_pair.hpp"
#include "fwd.hpp"
#include "process.hpp"

namespace entt {


template<typename Delta, typename Allocator>
class basic_scheduler {
    using base_type = basic_process<Delta, Allocator>;
    using alloc_traits = std::allocator_traits<Allocator>;
    using container_allocator = typename alloc_traits::template rebind_alloc<std::shared_ptr<base_type>>;
    using container_type = std::vector<std::shared_ptr<base_type>, container_allocator>;

public:
    
    using type = base_type;
    
    using allocator_type = Allocator;
    
    using size_type = std::size_t;
    
    using delta_type = Delta;

    
    basic_scheduler()
        : basic_scheduler{allocator_type{}} {}

    
    explicit basic_scheduler(const allocator_type &allocator)
        : handlers{allocator, allocator} {}

    
    basic_scheduler(const basic_scheduler &) = delete;

    
    basic_scheduler(basic_scheduler &&other) noexcept
        : handlers{std::move(other.handlers)} {}

    
    basic_scheduler(basic_scheduler &&other, const allocator_type &allocator)
        : handlers{container_type{std::move(other.handlers.first()), allocator}, allocator} {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || get_allocator() == other.get_allocator(), "Copying a scheduler is not allowed");
    }

    
    ~basic_scheduler() = default;

    
    basic_scheduler &operator=(const basic_scheduler &) = delete;

    
    basic_scheduler &operator=(basic_scheduler &&other) noexcept {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || get_allocator() == other.get_allocator(), "Copying a scheduler is not allowed");
        swap(other);
        return *this;
    }

    
    void swap(basic_scheduler &other) noexcept {
        using std::swap;
        swap(handlers, other.handlers);
    }

    
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return handlers.second();
    }

    
    [[nodiscard]] size_type size() const noexcept {
        return handlers.first().size();
    }

    
    [[nodiscard]] bool empty() const noexcept {
        return handlers.first().empty();
    }

    
    void clear() {
        handlers.first().clear();
    }

    
    template<typename Type, typename... Args>
    type &attach(Args &&...args) {
        const auto &allocator = handlers.second();
        return *handlers.first().emplace_back(std::allocate_shared<Type>(allocator, allocator, std::forward<Args>(args)...));
    }

    
    template<typename Func>
    type &attach(Func func) {
        const auto &allocator = handlers.second();
        using process_type = internal::process_adaptor<delta_type, Func, allocator_type>;
        return *handlers.first().emplace_back(std::allocate_shared<process_type>(allocator, allocator, std::move(func)));
    }

    
    void update(const delta_type delta, void *data = nullptr) {
        for(auto next = handlers.first().size(); next; --next) {
            const auto pos = next - 1u;
            handlers.first()[pos]->tick(delta, data);
            
            auto &elem = handlers.first()[pos];

            if(elem->finished()) {
                elem = elem->peek();
            }

            if(!elem || elem->rejected()) {
                elem = std::move(handlers.first().back());
                handlers.first().pop_back();
            }
        }
    }

    
    void abort(const bool immediate = false) {
        for(auto &&curr: handlers.first()) {
            curr->abort();

            if(immediate) {
                curr->tick({});
            }
        }
    }

private:
    compressed_pair<container_type, allocator_type> handlers;
};

} 

#endif
