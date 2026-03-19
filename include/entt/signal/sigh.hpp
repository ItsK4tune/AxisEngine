#ifndef ENTT_SIGNAL_SIGH_HPP
#define ENTT_SIGNAL_SIGH_HPP

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include "delegate.hpp"
#include "fwd.hpp"

namespace entt {


template<typename Type>
class sink;


template<typename Type, typename Allocator>
class sigh;


template<typename Ret, typename... Args, typename Allocator>
class sigh<Ret(Args...), Allocator> {
    friend class sink<sigh<Ret(Args...), Allocator>>;

    using alloc_traits = std::allocator_traits<Allocator>;
    using delegate_type = delegate<Ret(Args...)>;
    using container_type = std::vector<delegate_type, typename alloc_traits::template rebind_alloc<delegate_type>>;

public:
    
    using allocator_type = Allocator;
    
    using size_type = std::size_t;
    
    using sink_type = sink<sigh<Ret(Args...), Allocator>>;

    
    sigh() noexcept(noexcept(allocator_type{}))
        : sigh{allocator_type{}} {}

    
    explicit sigh(const allocator_type &allocator) noexcept
        : calls{allocator} {}

    
    sigh(const sigh &other)
        : calls{other.calls} {}

    
    sigh(const sigh &other, const allocator_type &allocator)
        : calls{other.calls, allocator} {}

    
    sigh(sigh &&other) noexcept
        : calls{std::move(other.calls)} {}

    
    sigh(sigh &&other, const allocator_type &allocator)
        : calls{std::move(other.calls), allocator} {}

    
    ~sigh() = default;

    
    sigh &operator=(const sigh &other) {
        calls = other.calls;
        return *this;
    }

    
    sigh &operator=(sigh &&other) noexcept {
        swap(other);
        return *this;
    }

    
    void swap(sigh &other) noexcept {
        using std::swap;
        swap(calls, other.calls);
    }

    
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return calls.get_allocator();
    }

    
    [[nodiscard]] size_type size() const noexcept {
        return calls.size();
    }

    
    [[nodiscard]] bool empty() const noexcept {
        return calls.empty();
    }

    
    void publish(Args... args) const {
        for(auto pos = calls.size(); pos; --pos) {
            calls[pos - 1u](args...);
        }
    }

    
    template<typename Func>
    void collect(Func func, Args... args) const {
        for(auto pos = calls.size(); pos; --pos) {
            if constexpr(std::is_void_v<Ret> || !std::is_invocable_v<Func, Ret>) {
                calls[pos - 1u](args...);

                if constexpr(std::is_invocable_r_v<bool, Func>) {
                    if(func()) {
                        break;
                    }
                } else {
                    func();
                }
            } else {
                if constexpr(std::is_invocable_r_v<bool, Func, Ret>) {
                    if(func(calls[pos - 1u](args...))) {
                        break;
                    }
                } else {
                    func(calls[pos - 1u](args...));
                }
            }
        }
    }

private:
    container_type calls;
};


class connection {
    template<typename>
    friend class sink;

    connection(delegate<void(void *)> fn, void *ref)
        : disconnect{fn}, signal{ref} {}

public:
    
    connection()
        : signal{} {}

    
    [[nodiscard]] explicit operator bool() const noexcept {
        return static_cast<bool>(disconnect);
    }

    
    void release() {
        if(disconnect) {
            disconnect(signal);
            disconnect.reset();
        }
    }

private:
    delegate<void(void *)> disconnect;
    void *signal;
};


struct scoped_connection {
    
    scoped_connection() = default;

    
    scoped_connection(const connection &other)
        : conn{other} {}

    
    scoped_connection(const scoped_connection &) = delete;

    
    scoped_connection(scoped_connection &&other) noexcept
        : conn{std::exchange(other.conn, {})} {}

    
    ~scoped_connection() {
        conn.release();
    }

    
    scoped_connection &operator=(const scoped_connection &) = delete;

    
    scoped_connection &operator=(scoped_connection &&other) noexcept {
        conn = std::exchange(other.conn, {});
        return *this;
    }

    
    scoped_connection &operator=(connection other) {
        conn = other;
        return *this;
    }

    
    [[nodiscard]] explicit operator bool() const noexcept {
        return static_cast<bool>(conn);
    }

    
    void release() {
        conn.release();
    }

private:
    connection conn;
};


template<typename Ret, typename... Args, typename Allocator>
class sink<sigh<Ret(Args...), Allocator>> {
    using signal_type = sigh<Ret(Args...), Allocator>;
    using delegate_type = typename signal_type::delegate_type;
    using difference_type = typename signal_type::container_type::difference_type;

    template<auto Candidate, typename Type>
    static void release(Type value_or_instance, void *signal) {
        sink{*static_cast<signal_type *>(signal)}.disconnect<Candidate>(value_or_instance);
    }

    template<auto Candidate>
    static void release(void *signal) {
        sink{*static_cast<signal_type *>(signal)}.disconnect<Candidate>();
    }

    template<typename Func>
    void disconnect_if(Func callback) {
        auto &ref = signal_or_assert();

        for(auto pos = ref.calls.size(); pos; --pos) {
            if(auto &elem = ref.calls[pos - 1u]; callback(elem)) {
                elem = std::move(ref.calls.back());
                ref.calls.pop_back();
            }
        }
    }

    [[nodiscard]] auto &signal_or_assert() const noexcept {
        ENTT_ASSERT(signal != nullptr, "Invalid pointer to signal");
        return *signal;
    }

public:
    
    sink() noexcept
        : signal{} {}

    
    sink(sigh<Ret(Args...), Allocator> &ref) noexcept
        : signal{&ref} {}

    
    [[nodiscard]] bool empty() const noexcept {
        return signal_or_assert().calls.empty();
    }

    
    template<auto Candidate>
    connection connect() {
        disconnect<Candidate>();

        delegate_type call{};
        call.template connect<Candidate>();
        signal_or_assert().calls.push_back(std::move(call));

        delegate<void(void *)> conn{};
        conn.template connect<&release<Candidate>>();
        return {conn, signal};
    }

    
    template<auto Candidate, typename Type>
    connection connect(Type &value_or_instance) {
        disconnect<Candidate>(value_or_instance);

        delegate_type call{};
        call.template connect<Candidate>(value_or_instance);
        signal_or_assert().calls.push_back(std::move(call));

        delegate<void(void *)> conn{};
        conn.template connect<&release<Candidate, Type &>>(value_or_instance);
        return {conn, signal};
    }

    
    template<auto Candidate, typename Type>
    connection connect(Type *value_or_instance) {
        disconnect<Candidate>(value_or_instance);

        delegate_type call{};
        call.template connect<Candidate>(value_or_instance);
        signal_or_assert().calls.push_back(std::move(call));

        delegate<void(void *)> conn{};
        conn.template connect<&release<Candidate, Type *>>(value_or_instance);
        return {conn, signal};
    }

    
    template<auto Candidate>
    void disconnect() {
        delegate_type call{};
        call.template connect<Candidate>();
        disconnect_if([&call](const auto &elem) { return elem == call; });
    }

    
    template<auto Candidate, typename Type>
    void disconnect(Type &value_or_instance) {
        delegate_type call{};
        call.template connect<Candidate>(value_or_instance);
        disconnect_if([&call](const auto &elem) { return elem == call; });
    }

    
    template<auto Candidate, typename Type>
    void disconnect(Type *value_or_instance) {
        delegate_type call{};
        call.template connect<Candidate>(value_or_instance);
        disconnect_if([&call](const auto &elem) { return elem == call; });
    }

    
    void disconnect(const void *value_or_instance) {
        ENTT_ASSERT(value_or_instance != nullptr, "Invalid value or instance");
        disconnect_if([value_or_instance](const auto &elem) { return elem.data() == value_or_instance; });
    }

    
    void disconnect() {
        signal_or_assert().calls.clear();
    }

    
    [[nodiscard]] explicit operator bool() const noexcept {
        return signal != nullptr;
    }

private:
    signal_type *signal;
};


template<typename Ret, typename... Args, typename Allocator>
sink(sigh<Ret(Args...), Allocator> &) -> sink<sigh<Ret(Args...), Allocator>>;

} 

#endif
