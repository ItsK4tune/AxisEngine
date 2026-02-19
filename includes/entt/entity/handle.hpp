#ifndef ENTT_ENTITY_HANDLE_HPP
#define ENTT_ENTITY_HANDLE_HPP

#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/iterator.hpp"
#include "../core/type_traits.hpp"
#include "entity.hpp"
#include "fwd.hpp"

namespace entt {


namespace internal {

template<typename It>
class handle_storage_iterator final {
    template<typename Other>
    friend class handle_storage_iterator;

    using underlying_type = std::remove_reference_t<typename It::value_type::second_type>;
    using entity_type = typename underlying_type::entity_type;

public:
    using value_type = typename std::iterator_traits<It>::value_type;
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

    constexpr handle_storage_iterator() noexcept
        : entt{null},
          it{},
          last{} {}

    constexpr handle_storage_iterator(entity_type value, It from, It to) noexcept
        : entt{value},
          it{from},
          last{to} {
        while(it != last && !it->second.contains(entt)) {
            ++it;
        }
    }

    constexpr handle_storage_iterator &operator++() noexcept {
        for(++it; it != last && !it->second.contains(entt); ++it) {}
        return *this;
    }

    constexpr handle_storage_iterator operator++(int) noexcept {
        const handle_storage_iterator orig = *this;
        return ++(*this), orig;
    }

    [[nodiscard]] constexpr reference operator*() const noexcept {
        return *it;
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept {
        return operator*();
    }

    template<typename ILhs, typename IRhs>
    friend constexpr bool operator==(const handle_storage_iterator<ILhs> &, const handle_storage_iterator<IRhs> &) noexcept;

private:
    entity_type entt;
    It it;
    It last;
};

template<typename ILhs, typename IRhs>
[[nodiscard]] constexpr bool operator==(const handle_storage_iterator<ILhs> &lhs, const handle_storage_iterator<IRhs> &rhs) noexcept {
    return lhs.it == rhs.it;
}

template<typename ILhs, typename IRhs>
[[nodiscard]] constexpr bool operator!=(const handle_storage_iterator<ILhs> &lhs, const handle_storage_iterator<IRhs> &rhs) noexcept {
    return !(lhs == rhs);
}

} 



template<typename Registry, typename... Scope>
class basic_handle {
    using traits_type = entt_traits<typename Registry::entity_type>;

    [[nodiscard]] auto &owner_or_assert() const noexcept {
        ENTT_ASSERT(owner != nullptr, "Invalid pointer to registry");
        return static_cast<Registry &>(*owner);
    }

public:
    
    using registry_type = Registry;
    
    using entity_type = typename traits_type::value_type;
    
    using version_type = typename traits_type::version_type;
    
    using size_type = std::size_t;
    
    using iterable = iterable_adaptor<internal::handle_storage_iterator<typename decltype(std::declval<registry_type>().storage())::iterator>>;

    
    basic_handle() noexcept
        : owner{},
          entt{null} {}

    
    basic_handle(registry_type &ref, entity_type value) noexcept
        : owner{&ref},
          entt{value} {}

    
    [[nodiscard]] iterable storage() const noexcept {
        auto underlying = owner_or_assert().storage();
        return iterable{{entt, underlying.begin(), underlying.end()}, {entt, underlying.end(), underlying.end()}};
    }

    
    [[nodiscard]] explicit operator bool() const noexcept {
        return owner && owner->valid(entt);
    }

    
    [[nodiscard]] bool valid() const {
        return static_cast<bool>(*this);
    }

    
    [[nodiscard]] registry_type *registry() const noexcept {
        return owner;
    }

    
    [[nodiscard]] entity_type entity() const noexcept {
        return entt;
    }

    
    [[nodiscard]] operator entity_type() const noexcept {
        return entity();
    }

    
    void destroy() {
        owner_or_assert().destroy(std::exchange(entt, null));
    }

    
    void destroy(const version_type version) {
        owner_or_assert().destroy(std::exchange(entt, null), version);
    }

    
    template<typename Type, typename... Args>
    
    decltype(auto) emplace(Args &&...args) const {
        static_assert(((sizeof...(Scope) == 0) || ... || std::is_same_v<Type, Scope>), "Invalid type");
        return owner_or_assert().template emplace<Type>(entt, std::forward<Args>(args)...);
    }

    
    template<typename Type, typename... Args>
    decltype(auto) emplace_or_replace(Args &&...args) const {
        static_assert(((sizeof...(Scope) == 0) || ... || std::is_same_v<Type, Scope>), "Invalid type");
        return owner_or_assert().template emplace_or_replace<Type>(entt, std::forward<Args>(args)...);
    }

    
    template<typename Type, typename... Func>
    decltype(auto) patch(Func &&...func) const {
        static_assert(((sizeof...(Scope) == 0) || ... || std::is_same_v<Type, Scope>), "Invalid type");
        return owner_or_assert().template patch<Type>(entt, std::forward<Func>(func)...);
    }

    
    template<typename Type, typename... Args>
    decltype(auto) replace(Args &&...args) const {
        static_assert(((sizeof...(Scope) == 0) || ... || std::is_same_v<Type, Scope>), "Invalid type");
        return owner_or_assert().template replace<Type>(entt, std::forward<Args>(args)...);
    }

    
    template<typename... Type>
    
    size_type remove() const {
        static_assert(sizeof...(Scope) == 0 || (type_list_contains_v<type_list<Scope...>, Type> && ...), "Invalid type");
        return owner_or_assert().template remove<Type...>(entt);
    }

    
    template<typename... Type>
    void erase() const {
        static_assert(sizeof...(Scope) == 0 || (type_list_contains_v<type_list<Scope...>, Type> && ...), "Invalid type");
        owner_or_assert().template erase<Type...>(entt);
    }

    
    template<typename... Type>
    [[nodiscard]] decltype(auto) all_of() const {
        return owner_or_assert().template all_of<Type...>(entt);
    }

    
    template<typename... Type>
    [[nodiscard]] decltype(auto) any_of() const {
        return owner_or_assert().template any_of<Type...>(entt);
    }

    
    template<typename... Type>
    [[nodiscard]] decltype(auto) get() const {
        static_assert(sizeof...(Scope) == 0 || (type_list_contains_v<type_list<Scope...>, Type> && ...), "Invalid type");
        return owner_or_assert().template get<Type...>(entt);
    }

    
    template<typename Type, typename... Args>
    [[nodiscard]] decltype(auto) get_or_emplace(Args &&...args) const {
        static_assert(((sizeof...(Scope) == 0) || ... || std::is_same_v<Type, Scope>), "Invalid type");
        return owner_or_assert().template get_or_emplace<Type>(entt, std::forward<Args>(args)...);
    }

    
    template<typename... Type>
    [[nodiscard]] auto try_get() const {
        static_assert(sizeof...(Scope) == 0 || (type_list_contains_v<type_list<Scope...>, Type> && ...), "Invalid type");
        return owner_or_assert().template try_get<Type...>(entt);
    }

    
    [[nodiscard]] bool orphan() const {
        return owner_or_assert().orphan(entt);
    }

    
    template<typename Other, typename... Args>
    operator basic_handle<Other, Args...>() const noexcept {
        static_assert(std::is_same_v<Other, Registry> || std::is_same_v<std::remove_const_t<Other>, Registry>, "Invalid conversion between different handles");
        static_assert((sizeof...(Scope) == 0 || ((sizeof...(Args) != 0 && sizeof...(Args) <= sizeof...(Scope)) && ... && (type_list_contains_v<type_list<Scope...>, Args>))), "Invalid conversion between different handles");
        return owner ? basic_handle<Other, Args...>{*owner, entt} : basic_handle<Other, Args...>{};
    }

private:
    registry_type *owner;
    entity_type entt;
};


template<typename... Args, typename... Other>
[[nodiscard]] bool operator==(const basic_handle<Args...> &lhs, const basic_handle<Other...> &rhs) noexcept {
    return lhs.registry() == rhs.registry() && lhs.entity() == rhs.entity();
}


template<typename... Args, typename... Other>
[[nodiscard]] bool operator!=(const basic_handle<Args...> &lhs, const basic_handle<Other...> &rhs) noexcept {
    return !(lhs == rhs);
}


template<typename... Args>
[[nodiscard]] constexpr bool operator==(const basic_handle<Args...> &lhs, const null_t rhs) noexcept {
    return (lhs.entity() == rhs);
}


template<typename... Args>
[[nodiscard]] constexpr bool operator==(const null_t lhs, const basic_handle<Args...> &rhs) noexcept {
    return (rhs == lhs);
}


template<typename... Args>
[[nodiscard]] constexpr bool operator!=(const basic_handle<Args...> &lhs, const null_t rhs) noexcept {
    return (lhs.entity() != rhs);
}


template<typename... Args>
[[nodiscard]] constexpr bool operator!=(const null_t lhs, const basic_handle<Args...> &rhs) noexcept {
    return (rhs != lhs);
}

} 

#endif
