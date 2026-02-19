#ifndef ENTT_ENTITY_HELPER_HPP
#define ENTT_ENTITY_HELPER_HPP

#include <memory>
#include <type_traits>
#include <utility>
#include "../core/fwd.hpp"
#include "../core/type_traits.hpp"
#include "component.hpp"
#include "fwd.hpp"
#include "group.hpp"
#include "storage.hpp"
#include "view.hpp"

namespace entt {


template<typename Registry>
class as_view {
    template<typename... Get, typename... Exclude>
    [[nodiscard]] auto dispatch(get_t<Get...>, exclude_t<Exclude...>) const {
        return reg->template view<constness_as_t<typename Get::element_type, Get>...>(exclude_t<constness_as_t<typename Exclude::element_type, Exclude>...>{});
    }

public:
    
    using registry_type = Registry;
    
    using entity_type = typename registry_type::entity_type;

    
    as_view(registry_type &source) noexcept
        : reg{&source} {}

    
    template<typename Get, typename Exclude>
    operator basic_view<Get, Exclude>() const {
        return dispatch(Get{}, Exclude{});
    }

private:
    registry_type *reg;
};


template<typename Registry>
class as_group {
    template<typename... Owned, typename... Get, typename... Exclude>
    [[nodiscard]] auto dispatch(owned_t<Owned...>, get_t<Get...>, exclude_t<Exclude...>) const {
        if constexpr(std::is_const_v<registry_type>) {
            return reg->template group_if_exists<typename Owned::element_type...>(get_t<typename Get::element_type...>{}, exclude_t<typename Exclude::element_type...>{});
        } else {
            return reg->template group<constness_as_t<typename Owned::element_type, Owned>...>(get_t<constness_as_t<typename Get::element_type, Get>...>{}, exclude_t<constness_as_t<typename Exclude::element_type, Exclude>...>{});
        }
    }

public:
    
    using registry_type = Registry;
    
    using entity_type = typename registry_type::entity_type;

    
    as_group(registry_type &source) noexcept
        : reg{&source} {}

    
    template<typename Owned, typename Get, typename Exclude>
    operator basic_group<Owned, Get, Exclude>() const {
        return dispatch(Owned{}, Get{}, Exclude{});
    }

private:
    registry_type *reg;
};


template<auto Member, typename Registry = std::decay_t<nth_argument_t<0u, decltype(Member)>>>
void invoke(Registry &reg, const typename Registry::entity_type entt) {
    static_assert(std::is_member_function_pointer_v<decltype(Member)>, "Invalid pointer to non-static member function");
    (reg.template get<member_class_t<decltype(Member)>>(entt).*Member)(reg, entt);
}


template<typename... Args>
typename basic_storage<Args...>::entity_type to_entity(const basic_storage<Args...> &storage, const typename basic_storage<Args...>::value_type &instance) {
    using traits_type = component_traits<typename basic_storage<Args...>::value_type, typename basic_storage<Args...>::entity_type>;
    static_assert(traits_type::page_size != 0u, "Unexpected page size");
    const auto *page = storage.raw();

    
    for(std::size_t pos{}, count = storage.size(); pos < count; pos += traits_type::page_size, ++page) {
        if(const auto dist = (std::addressof(instance) - *page); dist >= 0 && dist < static_cast<decltype(dist)>(traits_type::page_size)) {
            return *(static_cast<const typename basic_storage<Args...>::base_type &>(storage).rbegin() + static_cast<decltype(dist)>(pos) + dist);
        }
    }
    

    return null;
}


template<typename...>
struct sigh_helper;


template<typename Registry>
struct sigh_helper<Registry> {
    
    using registry_type = Registry;

    
    sigh_helper(registry_type &ref)
        : bucket{&ref} {}

    
    template<typename Type>
    auto with(const id_type id = type_hash<Type>::value()) noexcept {
        return sigh_helper<registry_type, Type>{*bucket, id};
    }

    
    [[nodiscard]] registry_type &registry() noexcept {
        return *bucket;
    }

private:
    registry_type *bucket;
};


template<typename Registry, typename Type>
struct sigh_helper<Registry, Type> final: sigh_helper<Registry> {
    
    using registry_type = Registry;

    
    sigh_helper(registry_type &ref, const id_type id = type_hash<Type>::value())
        : sigh_helper<Registry>{ref},
          name{id} {}

    
    template<auto Candidate, typename... Args>
    auto on_construct(Args &&...args) {
        this->registry().template on_construct<Type>(name).template connect<Candidate>(std::forward<Args>(args)...);
        return *this;
    }

    
    template<auto Candidate, typename... Args>
    auto on_update(Args &&...args) {
        this->registry().template on_update<Type>(name).template connect<Candidate>(std::forward<Args>(args)...);
        return *this;
    }

    
    template<auto Candidate, typename... Args>
    auto on_destroy(Args &&...args) {
        this->registry().template on_destroy<Type>(name).template connect<Candidate>(std::forward<Args>(args)...);
        return *this;
    }

private:
    id_type name;
};


template<typename Registry>
sigh_helper(Registry &) -> sigh_helper<Registry>;

} 

#endif
