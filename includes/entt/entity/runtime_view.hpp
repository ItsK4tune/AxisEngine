#ifndef ENTT_ENTITY_RUNTIME_VIEW_HPP
#define ENTT_ENTITY_RUNTIME_VIEW_HPP

#include <algorithm>
#include <cstddef>
#include <iterator>
#include <utility>
#include <vector>
#include "entity.hpp"
#include "fwd.hpp"

namespace entt {


namespace internal {

template<typename Set>
class runtime_view_iterator final {
    using iterator_type = typename Set::iterator;
    using iterator_traits = std::iterator_traits<iterator_type>;

    [[nodiscard]] bool valid() const {
        return (!tombstone_check || *it != tombstone)
               && std::all_of(++pools->begin(), pools->end(), [entt = *it](const auto *curr) { return curr->contains(entt); })
               && std::none_of(filter->cbegin(), filter->cend(), [entt = *it](const auto *curr) { return curr && curr->contains(entt); });
    }

public:
    using value_type = typename iterator_traits::value_type;
    using pointer = typename iterator_traits::pointer;
    using reference = typename iterator_traits::reference;
    using difference_type = typename iterator_traits::difference_type;
    using iterator_category = std::bidirectional_iterator_tag;

    constexpr runtime_view_iterator() noexcept
        : pools{},
          filter{},
          it{},
          tombstone_check{} {}

    runtime_view_iterator(const std::vector<Set *> &cpools, iterator_type curr, const std::vector<Set *> &ignore) noexcept
        : pools{&cpools},
          filter{&ignore},
          it{curr},
          tombstone_check{pools->size() == 1u && (*pools)[0u]->policy() == deletion_policy::in_place} {
        if(it != (*pools)[0]->end() && !valid()) {
            ++(*this);
        }
    }

    runtime_view_iterator &operator++() {
        ++it;
        for(const auto last = (*pools)[0]->end(); it != last && !valid(); ++it) {}
        return *this;
    }

    runtime_view_iterator operator++(int) {
        const runtime_view_iterator orig = *this;
        return ++(*this), orig;
    }

    runtime_view_iterator &operator--() {
        --it;
        for(const auto first = (*pools)[0]->begin(); it != first && !valid(); --it) {}
        return *this;
    }

    runtime_view_iterator operator--(int) {
        const runtime_view_iterator orig = *this;
        return operator--(), orig;
    }

    [[nodiscard]] pointer operator->() const noexcept {
        return it.operator->();
    }

    [[nodiscard]] reference operator*() const noexcept {
        return *operator->();
    }

    [[nodiscard]] constexpr bool operator==(const runtime_view_iterator &other) const noexcept {
        return it == other.it;
    }

    [[nodiscard]] constexpr bool operator!=(const runtime_view_iterator &other) const noexcept {
        return !(*this == other);
    }

private:
    const std::vector<Set *> *pools;
    const std::vector<Set *> *filter;
    iterator_type it;
    bool tombstone_check;
};

} 



template<typename Type, typename Allocator>
class basic_runtime_view {
    using alloc_traits = std::allocator_traits<Allocator>;
    static_assert(std::is_same_v<typename alloc_traits::value_type, Type *>, "Invalid value type");
    using container_type = std::vector<Type *, Allocator>;

    [[nodiscard]] auto offset() const noexcept {
        ENTT_ASSERT(!pools.empty(), "Invalid view");
        const auto &leading = *pools.front();
        return (leading.policy() == deletion_policy::swap_only) ? leading.free_list() : leading.size();
    }

public:
    
    using allocator_type = Allocator;
    
    using entity_type = typename Type::entity_type;
    
    using size_type = std::size_t;
    
    using difference_type = std::ptrdiff_t;
    
    using common_type = Type;
    
    using iterator = internal::runtime_view_iterator<common_type>;

    
    basic_runtime_view() noexcept
        : basic_runtime_view{allocator_type{}} {}

    
    explicit basic_runtime_view(const allocator_type &allocator)
        : pools{allocator},
          filter{allocator} {}

    
    basic_runtime_view(const basic_runtime_view &) = default;

    
    basic_runtime_view(const basic_runtime_view &other, const allocator_type &allocator)
        : pools{other.pools, allocator},
          filter{other.filter, allocator} {}

    
    basic_runtime_view(basic_runtime_view &&) noexcept = default;

    
    basic_runtime_view(basic_runtime_view &&other, const allocator_type &allocator)
        : pools{std::move(other.pools), allocator},
          filter{std::move(other.filter), allocator} {}

    
    ~basic_runtime_view() = default;

    
    basic_runtime_view &operator=(const basic_runtime_view &) = default;

    
    basic_runtime_view &operator=(basic_runtime_view &&) noexcept = default;

    
    void swap(basic_runtime_view &other) noexcept {
        using std::swap;
        swap(pools, other.pools);
        swap(filter, other.filter);
    }

    
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return pools.get_allocator();
    }

    
    void clear() {
        pools.clear();
        filter.clear();
    }

    
    basic_runtime_view &iterate(common_type &base) {
        if(pools.empty() || !(base.size() < pools.front()->size())) {
            pools.push_back(&base);
        } else {
            pools.push_back(std::exchange(pools.front(), &base));
        }

        return *this;
    }

    
    basic_runtime_view &exclude(common_type &base) {
        filter.push_back(&base);
        return *this;
    }

    
    [[nodiscard]] size_type size_hint() const {
        return pools.empty() ? size_type{} : offset();
    }

    
    [[nodiscard]] iterator begin() const {
        return pools.empty() ? iterator{} : iterator{pools, pools.front()->end() - static_cast<difference_type>(offset()), filter};
    }

    
    [[nodiscard]] iterator end() const {
        return pools.empty() ? iterator{} : iterator{pools, pools.front()->end(), filter};
    }

    
    [[nodiscard]] explicit operator bool() const noexcept {
        return !(pools.empty() && filter.empty());
    }

    
    [[nodiscard]] bool contains(const entity_type entt) const {
        return !pools.empty()
               && std::all_of(pools.cbegin(), pools.cend(), [entt](const auto *curr) { return curr->contains(entt); })
               && std::none_of(filter.cbegin(), filter.cend(), [entt](const auto *curr) { return curr && curr->contains(entt); })
               && pools.front()->index(entt) < offset();
    }

    
    template<typename Func>
    void each(Func func) const {
        for(const auto entity: *this) {
            func(entity);
        }
    }

private:
    container_type pools;
    container_type filter;
};

} 

#endif
