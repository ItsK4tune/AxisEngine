#ifndef ENTT_PROCESS_PROCESS_HPP
#define ENTT_PROCESS_PROCESS_HPP

#include <cstdint>
#include <memory>
#include <type_traits>
#include <utility>
#include "../core/compressed_pair.hpp"
#include "../core/type_traits.hpp"
#include "fwd.hpp"

namespace entt {


namespace internal {

template<typename, typename, typename>
struct process_adaptor;

} 



template<typename Delta, typename Allocator>
class basic_process: public std::enable_shared_from_this<basic_process<Delta, Allocator>> {
    enum class state : std::uint8_t {
        idle = 0,
        running,
        paused,
        succeeded,
        failed,
        aborted,
        finished,
        rejected
    };

    virtual void update(const Delta, void *) {
        abort();
    }

    virtual void succeeded() {}
    virtual void failed() {}
    virtual void aborted() {}

public:
    
    using allocator_type = Allocator;
    
    using delta_type = Delta;
    
    using handle_type = std::shared_ptr<basic_process>;

    
    basic_process()
        : basic_process{allocator_type{}} {}

    
    explicit basic_process(const allocator_type &allocator)
        : next{nullptr, allocator},
          current{state::idle} {}

    
    basic_process(const basic_process &) = delete;

    
    basic_process(basic_process &&) = delete;

    
    virtual ~basic_process() = default;

    
    basic_process &operator=(const basic_process &) = delete;

    
    basic_process &operator=(basic_process &&) = delete;

    
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return next.second();
    }

    
    void abort() {
        if(alive()) {
            current = state::aborted;
        }
    }

    
    void succeed() noexcept {
        if(alive()) {
            current = state::succeeded;
        }
    }

    
    void fail() noexcept {
        if(alive()) {
            current = state::failed;
        }
    }

    
    void pause() noexcept {
        if(alive()) {
            current = state::paused;
        }
    }

    
    void unpause() noexcept {
        if(alive()) {
            current = state::running;
        }
    }

    
    [[nodiscard]] bool alive() const noexcept {
        return current == state::running || current == state::paused;
    }

    
    [[nodiscard]] bool finished() const noexcept {
        return current == state::finished;
    }

    
    [[nodiscard]] bool paused() const noexcept {
        return current == state::paused;
    }

    
    [[nodiscard]] bool rejected() const noexcept {
        return current == state::rejected;
    }

    
    template<typename Type, typename... Args>
    basic_process &then(Args &&...args) {
        const auto &allocator = next.second();
        return *(next.first() = std::allocate_shared<Type>(allocator, allocator, std::forward<Args>(args)...));
    }

    
    template<typename Func>
    basic_process &then(Func func) {
        const auto &allocator = next.second();
        using process_type = internal::process_adaptor<delta_type, Func, allocator_type>;
        return *(next.first() = std::allocate_shared<process_type>(allocator, allocator, std::move(func)));
    }

    
    handle_type peek() {
        return next.first();
    }

    
    void tick(const Delta delta, void *data = nullptr) {
        switch(current) {
        case state::idle:
        case state::running:
            current = state::running;
            update(delta, data);
            break;
        default:
            
            break;
        }

        
        switch(current) {
        case state::succeeded:
            succeeded();
            current = state::finished;
            break;
        case state::failed:
            failed();
            current = state::rejected;
            break;
        case state::aborted:
            aborted();
            current = state::rejected;
            break;
        default:
            
            break;
        }
    }

private:
    compressed_pair<handle_type, allocator_type> next;
    state current;
};


namespace internal {

template<typename Delta, typename Func, typename Allocator>
struct process_adaptor: public basic_process<Delta, Allocator> {
    using allocator_type = Allocator;
    using base_type = basic_process<Delta, Allocator>;
    using delta_type = typename base_type::delta_type;

    process_adaptor(const allocator_type &allocator, Func proc)
        : base_type{allocator},
          func{std::move(proc)} {}

    void update(const delta_type delta, void *data) override {
        func(*this, delta, data);
    }

private:
    Func func;
};

} 


} 

#endif
