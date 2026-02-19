#ifndef ENTT_ENTITY_SNAPSHOT_HPP
#define ENTT_ENTITY_SNAPSHOT_HPP

#include <cstddef>
#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "../container/dense_map.hpp"
#include "../core/type_traits.hpp"
#include "entity.hpp"
#include "fwd.hpp"
#include "view.hpp"

namespace entt {


namespace internal {

template<typename Registry>
void orphans(Registry &registry) {
    auto &storage = registry.template storage<typename Registry::entity_type>();

    for(auto entt: storage) {
        if(registry.orphan(entt)) {
            storage.erase(entt);
        }
    }
}

} 



template<typename Registry>
class basic_snapshot {
    static_assert(!std::is_const_v<Registry>, "Non-const registry type required");
    using traits_type = entt_traits<typename Registry::entity_type>;

public:
    
    using registry_type = Registry;
    
    using entity_type = typename registry_type::entity_type;

    
    basic_snapshot(const registry_type &source) noexcept
        : reg{&source} {}

    
    basic_snapshot(const basic_snapshot &) = delete;

    
    basic_snapshot(basic_snapshot &&) noexcept = default;

    
    ~basic_snapshot() = default;

    
    basic_snapshot &operator=(const basic_snapshot &) = delete;

    
    basic_snapshot &operator=(basic_snapshot &&) noexcept = default;

    
    template<typename Type, typename Archive>
    const basic_snapshot &get(Archive &archive, const id_type id = type_hash<Type>::value()) const {
        if(const auto *storage = reg->template storage<Type>(id); storage) {
            const typename registry_type::common_type &base = *storage;

            archive(static_cast<typename traits_type::entity_type>(storage->size()));

            if constexpr(std::is_same_v<Type, entity_type>) {
                archive(static_cast<typename traits_type::entity_type>(storage->free_list()));

                for(auto first = base.rbegin(), last = base.rend(); first != last; ++first) {
                    archive(*first);
                }
            } else if constexpr(registry_type::template storage_for_type<Type>::storage_policy == deletion_policy::in_place) {
                for(auto it = base.rbegin(), last = base.rend(); it != last; ++it) {
                    if(const auto entt = *it; entt == tombstone) {
                        archive(static_cast<entity_type>(null));
                    } else {
                        archive(entt);
                        std::apply([&archive](auto &&...args) { (archive(std::forward<decltype(args)>(args)), ...); }, storage->get_as_tuple(entt));
                    }
                }
            } else {
                for(auto elem: storage->reach()) {
                    std::apply([&archive](auto &&...args) { (archive(std::forward<decltype(args)>(args)), ...); }, elem);
                }
            }
        } else {
            archive(typename traits_type::entity_type{});
        }

        return *this;
    }

    
    template<typename Type, typename Archive, typename It>
    const basic_snapshot &get(Archive &archive, It first, It last, const id_type id = type_hash<Type>::value()) const {
        static_assert(!std::is_same_v<Type, entity_type>, "Entity types not supported");

        if(const auto *storage = reg->template storage<Type>(id); storage && !storage->empty()) {
            archive(static_cast<typename traits_type::entity_type>(std::distance(first, last)));

            for(; first != last; ++first) {
                if(const auto entt = *first; storage->contains(entt)) {
                    archive(entt);
                    std::apply([&archive](auto &&...args) { (archive(std::forward<decltype(args)>(args)), ...); }, storage->get_as_tuple(entt));
                } else {
                    archive(static_cast<entity_type>(null));
                }
            }
        } else {
            archive(typename traits_type::entity_type{});
        }

        return *this;
    }

private:
    const registry_type *reg;
};


template<typename Registry>
class basic_snapshot_loader {
    static_assert(!std::is_const_v<Registry>, "Non-const registry type required");
    using traits_type = entt_traits<typename Registry::entity_type>;

public:
    
    using registry_type = Registry;
    
    using entity_type = typename registry_type::entity_type;

    
    basic_snapshot_loader(registry_type &source) noexcept
        : reg{&source} {
        
        ENTT_ASSERT(reg->template storage<entity_type>().free_list() == 0u, "Registry must be empty");
    }

    
    basic_snapshot_loader(const basic_snapshot_loader &) = delete;

    
    basic_snapshot_loader(basic_snapshot_loader &&) noexcept = default;

    
    ~basic_snapshot_loader() = default;

    
    basic_snapshot_loader &operator=(const basic_snapshot_loader &) = delete;

    
    basic_snapshot_loader &operator=(basic_snapshot_loader &&) noexcept = default;

    
    template<typename Type, typename Archive>
    basic_snapshot_loader &get(Archive &archive, const id_type id = type_hash<Type>::value()) {
        auto &storage = reg->template storage<Type>(id);
        typename traits_type::entity_type length{};

        archive(length);

        if constexpr(std::is_same_v<Type, entity_type>) {
            typename traits_type::entity_type count{};
            entity_type placeholder{};

            storage.reserve(length);
            archive(count);

            for(entity_type entity = null; length; --length) {
                archive(entity);
                storage.generate(entity);
                placeholder = (entity > placeholder) ? entity : placeholder;
            }

            storage.start_from(traits_type::next(placeholder));
            storage.free_list(count);
        } else {
            auto &other = reg->template storage<entity_type>();
            entity_type entt{null};

            while(length--) {
                if(archive(entt); entt != null) {
                    const auto entity = other.contains(entt) ? entt : other.generate(entt);
                    ENTT_ASSERT(entity == entt, "Entity not available for use");

                    if constexpr(std::tuple_size_v<decltype(storage.get_as_tuple({}))> == 0u) {
                        storage.emplace(entity);
                    } else {
                        Type elem{};
                        archive(elem);
                        storage.emplace(entity, std::move(elem));
                    }
                }
            }
        }

        return *this;
    }

    
    basic_snapshot_loader &orphans() {
        internal::orphans(*reg);
        return *this;
    }

private:
    registry_type *reg;
};


template<typename Registry>
class basic_continuous_loader {
    static_assert(!std::is_const_v<Registry>, "Non-const registry type required");
    using traits_type = entt_traits<typename Registry::entity_type>;

    void restore(typename Registry::entity_type entt) {
        if(const auto entity = to_entity(entt); remloc.contains(entity) && remloc[entity].first == entt) {
            if(!reg->valid(remloc[entity].second)) {
                remloc[entity].second = reg->create();
            }
        } else {
            remloc.insert_or_assign(entity, std::make_pair(entt, reg->create()));
        }
    }

    template<typename Container>
    auto update(int, Container &container) -> decltype(typename Container::mapped_type{}, void()) {
        
        Container other;

        for(auto &&pair: container) {
            using first_type = std::remove_const_t<typename std::decay_t<decltype(pair)>::first_type>;
            using second_type = typename std::decay_t<decltype(pair)>::second_type;

            if constexpr(std::is_same_v<first_type, entity_type> && std::is_same_v<second_type, entity_type>) {
                other.emplace(map(pair.first), map(pair.second));
            } else if constexpr(std::is_same_v<first_type, entity_type>) {
                other.emplace(map(pair.first), std::move(pair.second));
            } else {
                static_assert(std::is_same_v<second_type, entity_type>, "Neither the key nor the value are of entity type");
                other.emplace(std::move(pair.first), map(pair.second));
            }
        }

        using std::swap;
        swap(container, other);
    }

    template<typename Container>
    auto update(char, Container &container) -> decltype(typename Container::value_type{}, void()) {
        
        static_assert(std::is_same_v<typename Container::value_type, entity_type>, "Invalid value type");

        for(auto &&entt: container) {
            entt = map(entt);
        }
    }

    template<typename Component, typename Other, typename Member>
    void update([[maybe_unused]] Component &instance, [[maybe_unused]] Member Other::*member) {
        if constexpr(!std::is_same_v<Component, Other>) {
            return;
        } else if constexpr(std::is_same_v<Member, entity_type>) {
            instance.*member = map(instance.*member);
        } else {
            
            update(0, instance.*member);
        }
    }

public:
    
    using registry_type = Registry;
    
    using entity_type = typename registry_type::entity_type;

    
    basic_continuous_loader(registry_type &source) noexcept
        : remloc{source.get_allocator()},
          reg{&source} {}

    
    basic_continuous_loader(const basic_continuous_loader &) = delete;

    
    basic_continuous_loader(basic_continuous_loader &&) noexcept = default;

    
    ~basic_continuous_loader() = default;

    
    basic_continuous_loader &operator=(const basic_continuous_loader &) = delete;

    
    basic_continuous_loader &operator=(basic_continuous_loader &&) noexcept = default;

    
    template<typename Type, typename Archive>
    basic_continuous_loader &get(Archive &archive, const id_type id = type_hash<Type>::value()) {
        auto &storage = reg->template storage<Type>(id);
        typename traits_type::entity_type length{};
        entity_type entt{null};

        archive(length);

        if constexpr(std::is_same_v<Type, entity_type>) {
            typename traits_type::entity_type in_use{};

            storage.reserve(length);
            archive(in_use);

            for(std::size_t pos{}; pos < in_use; ++pos) {
                archive(entt);
                restore(entt);
            }

            for(std::size_t pos = in_use; pos < length; ++pos) {
                archive(entt);

                if(const auto entity = to_entity(entt); remloc.contains(entity)) {
                    if(reg->valid(remloc[entity].second)) {
                        reg->destroy(remloc[entity].second);
                    }

                    remloc.erase(entity);
                }
            }
        } else {
            for(auto &&ref: remloc) {
                storage.remove(ref.second.second);
            }

            while(length--) {
                if(archive(entt); entt != null) {
                    restore(entt);

                    if constexpr(std::tuple_size_v<decltype(storage.get_as_tuple({}))> == 0u) {
                        storage.emplace(map(entt));
                    } else {
                        Type elem{};
                        archive(elem);
                        storage.emplace(map(entt), std::move(elem));
                    }
                }
            }
        }

        return *this;
    }

    
    basic_continuous_loader &orphans() {
        internal::orphans(*reg);
        return *this;
    }

    
    [[nodiscard]] bool contains(entity_type entt) const noexcept {
        const auto it = remloc.find(to_entity(entt));
        return it != remloc.cend() && it->second.first == entt;
    }

    
    [[nodiscard]] entity_type map(entity_type entt) const noexcept {
        if(const auto it = remloc.find(to_entity(entt)); it != remloc.cend() && it->second.first == entt) {
            return it->second.second;
        }

        return null;
    }

private:
    dense_map<typename traits_type::entity_type, std::pair<entity_type, entity_type>> remloc;
    registry_type *reg;
};

} 

#endif
