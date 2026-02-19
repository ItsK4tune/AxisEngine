#ifndef ENTT_ENTITY_MIXIN_HPP
#define ENTT_ENTITY_MIXIN_HPP

#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/any.hpp"
#include "../core/type_info.hpp"
#include "../signal/sigh.hpp"
#include "entity.hpp"
#include "fwd.hpp"

namespace entt {


namespace internal {

template<typename, typename, typename = void>
struct has_on_construct final: std::false_type {};

template<typename Type, typename Registry>
struct has_on_construct<Type, Registry, std::void_t<decltype(Type::on_construct(std::declval<Registry &>(), std::declval<Registry>().create()))>>
    : std::true_type {};

template<typename, typename, typename = void>
struct has_on_update final: std::false_type {};

template<typename Type, typename Registry>
struct has_on_update<Type, Registry, std::void_t<decltype(Type::on_update(std::declval<Registry &>(), std::declval<Registry>().create()))>>
    : std::true_type {};

template<typename, typename, typename = void>
struct has_on_destroy final: std::false_type {};

template<typename Type, typename Registry>
struct has_on_destroy<Type, Registry, std::void_t<decltype(Type::on_destroy(std::declval<Registry &>(), std::declval<Registry>().create()))>>
    : std::true_type {};

} 



template<typename Type, typename Registry>
class basic_sigh_mixin final: public Type {
    using underlying_type = Type;
    using owner_type = Registry;

    using basic_registry_type = basic_registry<typename owner_type::entity_type, typename owner_type::allocator_type>;
    using sigh_type = sigh<void(owner_type &, const typename underlying_type::entity_type), typename underlying_type::allocator_type>;
    using underlying_iterator = typename underlying_type::base_type::basic_iterator;

    static_assert(std::is_base_of_v<basic_registry_type, owner_type>, "Invalid registry type");

    [[nodiscard]] auto &owner_or_assert() const noexcept {
        ENTT_ASSERT(owner != nullptr, "Invalid pointer to registry");
        return static_cast<owner_type &>(*owner);
    }

private:
    void pop(underlying_iterator first, underlying_iterator last) final {
        if(auto &reg = owner_or_assert(); destruction.empty()) {
            underlying_type::pop(first, last);
        } else {
            for(; first != last; ++first) {
                const auto entt = *first;
                destruction.publish(reg, entt);
                const auto it = underlying_type::find(entt);
                underlying_type::pop(it, it + 1u);
            }
        }
    }

    void pop_all() final {
        if(auto &reg = owner_or_assert(); !destruction.empty()) {
            if constexpr(std::is_same_v<typename underlying_type::element_type, entity_type>) {
                for(typename underlying_type::size_type pos{}, last = underlying_type::free_list(); pos < last; ++pos) {
                    destruction.publish(reg, underlying_type::base_type::operator[](pos));
                }
            } else {
                for(auto entt: static_cast<typename underlying_type::base_type &>(*this)) {
                    if constexpr(underlying_type::storage_policy == deletion_policy::in_place) {
                        if(entt != tombstone) {
                            destruction.publish(reg, entt);
                        }
                    } else {
                        destruction.publish(reg, entt);
                    }
                }
            }
        }

        underlying_type::pop_all();
    }

    underlying_iterator try_emplace(const typename underlying_type::entity_type entt, const bool force_back, const void *value) final {
        const auto it = underlying_type::try_emplace(entt, force_back, value);

        if(auto &reg = owner_or_assert(); it != underlying_type::base_type::end()) {
            construction.publish(reg, *it);
        }

        return it;
    }

    void bind_any(any value) noexcept final {
        owner = any_cast<basic_registry_type>(&value);

        if constexpr(!std::is_same_v<registry_type, basic_registry_type>) {
            if(owner == nullptr) {
                owner = any_cast<registry_type>(&value);
            }
        }

        underlying_type::bind_any(std::move(value));
    }

public:
    
    using allocator_type = typename underlying_type::allocator_type;
    
    using entity_type = typename underlying_type::entity_type;
    
    using registry_type = owner_type;

    
    basic_sigh_mixin()
        : basic_sigh_mixin{allocator_type{}} {}

    
    explicit basic_sigh_mixin(const allocator_type &allocator)
        : underlying_type{allocator},
          owner{},
          construction{allocator},
          destruction{allocator},
          update{allocator} {
        if constexpr(internal::has_on_construct<typename underlying_type::element_type, Registry>::value) {
            sink{construction}.template connect<&underlying_type::element_type::on_construct>();
        }

        if constexpr(internal::has_on_update<typename underlying_type::element_type, Registry>::value) {
            sink{update}.template connect<&underlying_type::element_type::on_update>();
        }

        if constexpr(internal::has_on_destroy<typename underlying_type::element_type, Registry>::value) {
            sink{destruction}.template connect<&underlying_type::element_type::on_destroy>();
        }
    }

    
    basic_sigh_mixin(const basic_sigh_mixin &) = delete;

    
    basic_sigh_mixin(basic_sigh_mixin &&other) noexcept
        : underlying_type{static_cast<underlying_type &&>(other)},
          owner{other.owner},
          construction{std::move(other.construction)},
          destruction{std::move(other.destruction)},
          update{std::move(other.update)} {}

    
    basic_sigh_mixin(basic_sigh_mixin &&other, const allocator_type &allocator)
        : underlying_type{static_cast<underlying_type &&>(other), allocator},
          owner{other.owner},
          construction{std::move(other.construction), allocator},
          destruction{std::move(other.destruction), allocator},
          update{std::move(other.update), allocator} {}

    
    ~basic_sigh_mixin() override = default;

    
    basic_sigh_mixin &operator=(const basic_sigh_mixin &) = delete;

    
    basic_sigh_mixin &operator=(basic_sigh_mixin &&other) noexcept {
        swap(other);
        return *this;
    }

    
    void swap(basic_sigh_mixin &other) noexcept {
        using std::swap;
        swap(owner, other.owner);
        swap(construction, other.construction);
        swap(destruction, other.destruction);
        swap(update, other.update);
        underlying_type::swap(other);
    }

    
    [[nodiscard]] auto on_construct() noexcept {
        return sink{construction};
    }

    
    [[nodiscard]] auto on_update() noexcept {
        return sink{update};
    }

    
    [[nodiscard]] auto on_destroy() noexcept {
        return sink{destruction};
    }

    
    [[nodiscard]] explicit operator bool() const noexcept {
        return (owner != nullptr);
    }

    
    [[nodiscard]] const registry_type &registry() const noexcept {
        return owner_or_assert();
    }

    
    [[nodiscard]] registry_type &registry() noexcept {
        return owner_or_assert();
    }

    
    auto generate() {
        const auto entt = underlying_type::generate();
        construction.publish(owner_or_assert(), entt);
        return entt;
    }

    
    entity_type generate(const entity_type hint) {
        const auto entt = underlying_type::generate(hint);
        construction.publish(owner_or_assert(), entt);
        return entt;
    }

    
    template<typename It>
    void generate(It first, It last) {
        underlying_type::generate(first, last);

        if(auto &reg = owner_or_assert(); !construction.empty()) {
            for(; first != last; ++first) {
                construction.publish(reg, *first);
            }
        }
    }

    
    template<typename... Args>
    decltype(auto) emplace(const entity_type entt, Args &&...args) {
        underlying_type::emplace(entt, std::forward<Args>(args)...);
        construction.publish(owner_or_assert(), entt);
        return this->get(entt);
    }

    
    template<typename... Func>
    decltype(auto) patch(const entity_type entt, Func &&...func) {
        underlying_type::patch(entt, std::forward<Func>(func)...);
        update.publish(owner_or_assert(), entt);
        return this->get(entt);
    }

    
    template<typename It, typename... Args>
    void insert(It first, It last, Args &&...args) {
        auto from = underlying_type::size();
        underlying_type::insert(first, last, std::forward<Args>(args)...);

        if(auto &reg = owner_or_assert(); !construction.empty()) {
            
            for(const auto to = underlying_type::size(); from != to; ++from) {
                construction.publish(reg, underlying_type::operator[](from));
            }
        }
    }

private:
    basic_registry_type *owner;
    sigh_type construction;
    sigh_type destruction;
    sigh_type update;
};


template<typename Type, typename Registry>
class basic_reactive_mixin final: public Type {
    using underlying_type = Type;
    using owner_type = Registry;

    using alloc_traits = std::allocator_traits<typename underlying_type::allocator_type>;
    using basic_registry_type = basic_registry<typename owner_type::entity_type, typename owner_type::allocator_type>;
    using container_type = std::vector<connection, typename alloc_traits::template rebind_alloc<connection>>;

    static_assert(std::is_base_of_v<basic_registry_type, owner_type>, "Invalid registry type");

    [[nodiscard]] auto &owner_or_assert() const noexcept {
        ENTT_ASSERT(owner != nullptr, "Invalid pointer to registry");
        return static_cast<owner_type &>(*owner);
    }

    void emplace_element(const Registry &, typename underlying_type::entity_type entity) {
        if(!underlying_type::contains(entity)) {
            underlying_type::emplace(entity);
        }
    }

private:
    void bind_any(any value) noexcept final {
        owner = any_cast<basic_registry_type>(&value);

        if constexpr(!std::is_same_v<registry_type, basic_registry_type>) {
            if(owner == nullptr) {
                owner = any_cast<registry_type>(&value);
            }
        }

        underlying_type::bind_any(std::move(value));
    }

public:
    
    using allocator_type = typename underlying_type::allocator_type;
    
    using entity_type = typename underlying_type::entity_type;
    
    using registry_type = owner_type;

    
    basic_reactive_mixin()
        : basic_reactive_mixin{allocator_type{}} {}

    
    explicit basic_reactive_mixin(const allocator_type &allocator)
        : underlying_type{allocator},
          owner{},
          conn{allocator} {
    }

    
    basic_reactive_mixin(const basic_reactive_mixin &) = delete;

    
    basic_reactive_mixin(basic_reactive_mixin &&other) noexcept
        : underlying_type{static_cast<underlying_type &&>(other)},
          owner{other.owner},
          conn{std::move(other.conn)} {
    }

    
    basic_reactive_mixin(basic_reactive_mixin &&other, const allocator_type &allocator)
        : underlying_type{static_cast<underlying_type &&>(other), allocator},
          owner{other.owner},
          conn{std::move(other.conn), allocator} {
    }

    
    ~basic_reactive_mixin() override = default;

    
    basic_reactive_mixin &operator=(const basic_reactive_mixin &) = delete;

    
    basic_reactive_mixin &operator=(basic_reactive_mixin &&other) noexcept {
        underlying_type::swap(other);
        return *this;
    }

    
    template<typename Clazz, auto Candidate = &basic_reactive_mixin::emplace_element>
    basic_reactive_mixin &on_construct(const id_type id = type_hash<Clazz>::value()) {
        auto curr = owner_or_assert().template storage<Clazz>(id).on_construct().template connect<Candidate>(*this);
        conn.push_back(std::move(curr));
        return *this;
    }

    
    template<typename Clazz, auto Candidate = &basic_reactive_mixin::emplace_element>
    basic_reactive_mixin &on_update(const id_type id = type_hash<Clazz>::value()) {
        auto curr = owner_or_assert().template storage<Clazz>(id).on_update().template connect<Candidate>(*this);
        conn.push_back(std::move(curr));
        return *this;
    }

    
    template<typename Clazz, auto Candidate = &basic_reactive_mixin::emplace_element>
    basic_reactive_mixin &on_destroy(const id_type id = type_hash<Clazz>::value()) {
        auto curr = owner_or_assert().template storage<Clazz>(id).on_destroy().template connect<Candidate>(*this);
        conn.push_back(std::move(curr));
        return *this;
    }

    
    [[nodiscard]] explicit operator bool() const noexcept {
        return (owner != nullptr);
    }

    
    [[nodiscard]] const registry_type &registry() const noexcept {
        return owner_or_assert();
    }

    
    [[nodiscard]] registry_type &registry() noexcept {
        return owner_or_assert();
    }

    
    template<typename... Get, typename... Exclude>
    [[nodiscard]] basic_view<get_t<const basic_reactive_mixin, typename basic_registry_type::template storage_for_type<const Get>...>, exclude_t<typename basic_registry_type::template storage_for_type<const Exclude>...>>
    view(exclude_t<Exclude...> = exclude_t{}) const {
        const owner_type &parent = owner_or_assert();
        basic_view<get_t<const basic_reactive_mixin, typename basic_registry_type::template storage_for_type<const Get>...>, exclude_t<typename basic_registry_type::template storage_for_type<const Exclude>...>> elem{};
        [&elem](const auto *...curr) { ((curr ? elem.storage(*curr) : void()), ...); }(parent.template storage<std::remove_const_t<Exclude>>()..., parent.template storage<std::remove_const_t<Get>>()..., this);
        return elem;
    }

    
    template<typename... Get, typename... Exclude>
    [[nodiscard]] basic_view<get_t<const basic_reactive_mixin, typename basic_registry_type::template storage_for_type<Get>...>, exclude_t<typename basic_registry_type::template storage_for_type<Exclude>...>>
    view(exclude_t<Exclude...> = exclude_t{}) {
        std::conditional_t<((std::is_const_v<Get> && ...) && (std::is_const_v<Exclude> && ...)), const owner_type, owner_type> &parent = owner_or_assert();
        return {*this, parent.template storage<std::remove_const_t<Get>>()..., parent.template storage<std::remove_const_t<Exclude>>()...};
    }

    
    void reset() {
        for(auto &&curr: conn) {
            curr.release();
        }

        conn.clear();
    }

private:
    basic_registry_type *owner;
    container_type conn;
};

} 

#endif
