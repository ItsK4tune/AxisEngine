#ifndef ENTT_ENTITY_SPARSE_SET_HPP
#define ENTT_ENTITY_SPARSE_SET_HPP

#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "../core/algorithm.hpp"
#include "../core/any.hpp"
#include "../core/bit.hpp"
#include "../core/type_info.hpp"
#include "entity.hpp"
#include "fwd.hpp"

namespace entt {


namespace internal {

template<typename Container>
struct sparse_set_iterator final {
    using value_type = typename Container::value_type;
    using pointer = typename Container::const_pointer;
    using reference = typename Container::const_reference;
    using difference_type = typename Container::difference_type;
    using iterator_category = std::random_access_iterator_tag;

    constexpr sparse_set_iterator() noexcept
        : packed{},
          offset{} {}

    constexpr sparse_set_iterator(const Container &ref, const difference_type idx) noexcept
        : packed{&ref},
          offset{idx} {}

    constexpr sparse_set_iterator &operator++() noexcept {
        return --offset, *this;
    }

    constexpr sparse_set_iterator operator++(int) noexcept {
        const sparse_set_iterator orig = *this;
        return ++(*this), orig;
    }

    constexpr sparse_set_iterator &operator--() noexcept {
        return ++offset, *this;
    }

    constexpr sparse_set_iterator operator--(int) noexcept {
        const sparse_set_iterator orig = *this;
        return operator--(), orig;
    }

    constexpr sparse_set_iterator &operator+=(const difference_type value) noexcept {
        offset -= value;
        return *this;
    }

    constexpr sparse_set_iterator operator+(const difference_type value) const noexcept {
        sparse_set_iterator copy = *this;
        return (copy += value);
    }

    constexpr sparse_set_iterator &operator-=(const difference_type value) noexcept {
        return (*this += -value);
    }

    constexpr sparse_set_iterator operator-(const difference_type value) const noexcept {
        return (*this + -value);
    }

    [[nodiscard]] constexpr reference operator[](const difference_type value) const noexcept {
        return (*packed)[static_cast<typename Container::size_type>(index() - value)];
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept {
        return std::addressof(operator[](0));
    }

    [[nodiscard]] constexpr reference operator*() const noexcept {
        return operator[](0);
    }

    [[nodiscard]] constexpr pointer data() const noexcept {
        return packed ? packed->data() : nullptr;
    }

    [[nodiscard]] constexpr difference_type index() const noexcept {
        return offset - 1;
    }

private:
    const Container *packed;
    difference_type offset;
};

template<typename Container>
[[nodiscard]] constexpr std::ptrdiff_t operator-(const sparse_set_iterator<Container> &lhs, const sparse_set_iterator<Container> &rhs) noexcept {
    return rhs.index() - lhs.index();
}

template<typename Container>
[[nodiscard]] constexpr bool operator==(const sparse_set_iterator<Container> &lhs, const sparse_set_iterator<Container> &rhs) noexcept {
    return lhs.index() == rhs.index();
}

template<typename Container>
[[nodiscard]] constexpr bool operator!=(const sparse_set_iterator<Container> &lhs, const sparse_set_iterator<Container> &rhs) noexcept {
    return !(lhs == rhs);
}

template<typename Container>
[[nodiscard]] constexpr bool operator<(const sparse_set_iterator<Container> &lhs, const sparse_set_iterator<Container> &rhs) noexcept {
    return lhs.index() > rhs.index();
}

template<typename Container>
[[nodiscard]] constexpr bool operator>(const sparse_set_iterator<Container> &lhs, const sparse_set_iterator<Container> &rhs) noexcept {
    return rhs < lhs;
}

template<typename Container>
[[nodiscard]] constexpr bool operator<=(const sparse_set_iterator<Container> &lhs, const sparse_set_iterator<Container> &rhs) noexcept {
    return !(lhs > rhs);
}

template<typename Container>
[[nodiscard]] constexpr bool operator>=(const sparse_set_iterator<Container> &lhs, const sparse_set_iterator<Container> &rhs) noexcept {
    return !(lhs < rhs);
}

} 



template<typename Entity, typename Allocator>
class basic_sparse_set {
    using alloc_traits = std::allocator_traits<Allocator>;
    static_assert(std::is_same_v<typename alloc_traits::value_type, Entity>, "Invalid value type");
    using sparse_container_type = std::vector<typename alloc_traits::pointer, typename alloc_traits::template rebind_alloc<typename alloc_traits::pointer>>;
    using packed_container_type = std::vector<Entity, Allocator>;
    using traits_type = entt_traits<Entity>;

    static constexpr auto max_size = static_cast<std::size_t>(traits_type::to_entity(null));

    
    [[nodiscard]] std::size_t policy_to_head() const noexcept {
        return static_cast<size_type>(max_size * static_cast<std::remove_const_t<decltype(max_size)>>(mode != deletion_policy::swap_only));
    }

    [[nodiscard]] auto entity_to_pos(const Entity entt) const noexcept {
        return static_cast<size_type>(traits_type::to_entity(entt));
    }

    [[nodiscard]] auto pos_to_page(const std::size_t pos) const noexcept {
        return static_cast<size_type>(pos / traits_type::page_size);
    }

    [[nodiscard]] auto sparse_ptr(const Entity entt) const {
        const auto pos = entity_to_pos(entt);
        const auto page = pos_to_page(pos);
        return (page < sparse.size() && sparse[page]) ? (sparse[page] + fast_mod(pos, traits_type::page_size)) : nullptr;
    }

    [[nodiscard]] auto &sparse_ref(const Entity entt) const {
        ENTT_ASSERT(sparse_ptr(entt), "Invalid element");
        const auto pos = entity_to_pos(entt);
        return sparse[pos_to_page(pos)][fast_mod(pos, traits_type::page_size)];
    }

    [[nodiscard]] auto to_iterator(const Entity entt) const {
        return --(end() - static_cast<difference_type>(index(entt)));
    }

    [[nodiscard]] auto &assure_at_least(const Entity entt) {
        const auto pos = entity_to_pos(entt);
        const auto page = pos_to_page(pos);

        if(!(page < sparse.size())) {
            sparse.resize(page + 1u, nullptr);
        }

        if(!sparse[page]) {
            constexpr entity_type init = null;
            auto page_allocator{packed.get_allocator()};
            sparse[page] = alloc_traits::allocate(page_allocator, traits_type::page_size);
            std::uninitialized_fill(sparse[page], sparse[page] + traits_type::page_size, init);
        }

        return sparse[page][fast_mod(pos, traits_type::page_size)];
    }

    void release_sparse_pages() {
        auto page_allocator{packed.get_allocator()};

        for(auto &&page: sparse) {
            if(page != nullptr) {
                std::destroy(page, page + traits_type::page_size);
                alloc_traits::deallocate(page_allocator, page, traits_type::page_size);
                page = nullptr;
            }
        }
    }

    void swap_at(const std::size_t lhs, const std::size_t rhs) {
        auto &from = packed[lhs];
        auto &to = packed[rhs];

        sparse_ref(from) = traits_type::combine(static_cast<typename traits_type::entity_type>(rhs), traits_type::to_integral(from));
        sparse_ref(to) = traits_type::combine(static_cast<typename traits_type::entity_type>(lhs), traits_type::to_integral(to));

        std::swap(from, to);
    }

private:
    [[nodiscard]] virtual const void *get_at(const std::size_t) const {
        return nullptr;
    }

    virtual void swap_or_move([[maybe_unused]] const std::size_t lhs, [[maybe_unused]] const std::size_t rhs) {
        ENTT_ASSERT((mode != deletion_policy::swap_only) || ((lhs < head) == (rhs < head)), "Cross swapping is not supported");
    }

protected:
    
    using basic_iterator = internal::sparse_set_iterator<packed_container_type>;

    
    void swap_only(const basic_iterator it) {
        ENTT_ASSERT(mode == deletion_policy::swap_only, "Deletion policy mismatch");
        const auto pos = index(*it);
        bump(traits_type::next(*it));
        swap_at(pos, head -= (pos < head));
    }

    
    void swap_and_pop(const basic_iterator it) {
        ENTT_ASSERT(mode == deletion_policy::swap_and_pop, "Deletion policy mismatch");
        auto &self = sparse_ref(*it);
        const auto entt = traits_type::to_entity(self);
        sparse_ref(packed.back()) = traits_type::combine(entt, traits_type::to_integral(packed.back()));
        packed[static_cast<size_type>(entt)] = packed.back();
        
        
        ENTT_ASSERT((packed.back() = null, true), "");
        
        self = null;
        packed.pop_back();
    }

    
    void in_place_pop(const basic_iterator it) {
        ENTT_ASSERT(mode == deletion_policy::in_place, "Deletion policy mismatch");
        const auto pos = entity_to_pos(std::exchange(sparse_ref(*it), null));
        packed[pos] = traits_type::combine(static_cast<typename traits_type::entity_type>(std::exchange(head, pos)), tombstone);
    }

    
    virtual void pop(basic_iterator first, basic_iterator last) {
        switch(mode) {
        case deletion_policy::swap_and_pop:
            for(; first != last; ++first) {
                swap_and_pop(first);
            }
            break;
        case deletion_policy::in_place:
            for(; first != last; ++first) {
                in_place_pop(first);
            }
            break;
        case deletion_policy::swap_only:
            for(; first != last; ++first) {
                swap_only(first);
            }
            break;
        }
    }

    
    virtual void pop_all() {
        switch(mode) {
        case deletion_policy::in_place:
            if(head != max_size) {
                for(auto &&elem: packed) {
                    if(elem != tombstone) {
                        sparse_ref(elem) = null;
                    }
                }
                break;
            }
            [[fallthrough]];
        case deletion_policy::swap_only:
        case deletion_policy::swap_and_pop:
            for(auto &&elem: packed) {
                sparse_ref(elem) = null;
            }
            break;
        }

        head = policy_to_head();
        packed.clear();
    }

    
    virtual basic_iterator try_emplace(const Entity entt, const bool force_back, const void * = nullptr) {
        ENTT_ASSERT(entt != null && entt != tombstone, "Invalid element");
        auto &elem = assure_at_least(entt);
        auto pos = size();

        switch(mode) {
        case deletion_policy::in_place:
            if(head != max_size && !force_back) {
                pos = head;
                ENTT_ASSERT(elem == null, "Slot not available");
                elem = traits_type::combine(static_cast<typename traits_type::entity_type>(head), traits_type::to_integral(entt));
                head = entity_to_pos(std::exchange(packed[pos], entt));
                break;
            }
            [[fallthrough]];
        case deletion_policy::swap_and_pop:
            packed.push_back(entt);
            ENTT_ASSERT(elem == null, "Slot not available");
            elem = traits_type::combine(static_cast<typename traits_type::entity_type>(packed.size() - 1u), traits_type::to_integral(entt));
            break;
        case deletion_policy::swap_only:
            if(elem == null) {
                packed.push_back(entt);
                elem = traits_type::combine(static_cast<typename traits_type::entity_type>(packed.size() - 1u), traits_type::to_integral(entt));
            } else {
                ENTT_ASSERT(!(entity_to_pos(elem) < head), "Slot not available");
                bump(entt);
            }

            pos = head++;
            swap_at(entity_to_pos(elem), pos);
            break;
        }

        return iterator{packed, static_cast<difference_type>(++pos)};
    }

    
    
    virtual void bind_any(any) noexcept {}

public:
    
    using allocator_type = Allocator;
    
    using entity_type = typename traits_type::value_type;
    
    using version_type = typename traits_type::version_type;
    
    using size_type = std::size_t;
    
    using difference_type = std::ptrdiff_t;
    
    using pointer = typename packed_container_type::const_pointer;
    
    using iterator = basic_iterator;
    
    using const_iterator = iterator;
    
    using reverse_iterator = std::reverse_iterator<iterator>;
    
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    
    basic_sparse_set()
        : basic_sparse_set{type_id<void>()} {}

    
    explicit basic_sparse_set(const allocator_type &allocator)
        : basic_sparse_set{deletion_policy::swap_and_pop, allocator} {}

    
    explicit basic_sparse_set(deletion_policy pol, const allocator_type &allocator = {})
        : basic_sparse_set{type_id<void>(), pol, allocator} {}

    
    explicit basic_sparse_set(const type_info &elem, deletion_policy pol = deletion_policy::swap_and_pop, const allocator_type &allocator = {})
        : sparse{allocator},
          packed{allocator},
          descriptor{&elem},
          mode{pol},
          head{policy_to_head()} {
        ENTT_ASSERT(traits_type::version_mask || mode != deletion_policy::in_place, "Policy does not support zero-sized versions");
    }

    
    basic_sparse_set(const basic_sparse_set &) = delete;

    
    basic_sparse_set(basic_sparse_set &&other) noexcept
        : sparse{std::move(other.sparse)},
          packed{std::move(other.packed)},
          descriptor{other.descriptor},
          mode{other.mode},
          head{std::exchange(other.head, policy_to_head())} {}

    
    basic_sparse_set(basic_sparse_set &&other, const allocator_type &allocator)
        : sparse{std::move(other.sparse), allocator},
          packed{std::move(other.packed), allocator},
          descriptor{other.descriptor},
          mode{other.mode},
          head{std::exchange(other.head, policy_to_head())} {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || get_allocator() == other.get_allocator(), "Copying a sparse set is not allowed");
    }

    
    virtual ~basic_sparse_set() {
        release_sparse_pages();
    }

    
    basic_sparse_set &operator=(const basic_sparse_set &) = delete;

    
    basic_sparse_set &operator=(basic_sparse_set &&other) noexcept {
        ENTT_ASSERT(alloc_traits::is_always_equal::value || get_allocator() == other.get_allocator(), "Copying a sparse set is not allowed");
        swap(other);
        return *this;
    }

    
    void swap(basic_sparse_set &other) noexcept {
        using std::swap;
        swap(sparse, other.sparse);
        swap(packed, other.packed);
        swap(descriptor, other.descriptor);
        swap(mode, other.mode);
        swap(head, other.head);
    }

    
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return packed.get_allocator();
    }

    
    [[nodiscard]] deletion_policy policy() const noexcept {
        return mode;
    }

    
    [[nodiscard]] size_type free_list() const noexcept {
        return head;
    }

    
    void free_list(const size_type value) noexcept {
        ENTT_ASSERT((mode == deletion_policy::swap_only) && !(value > packed.size()), "Invalid value");
        head = value;
    }

    
    virtual void reserve(const size_type cap) {
        packed.reserve(cap);
    }

    
    [[nodiscard]] virtual size_type capacity() const noexcept {
        return packed.capacity();
    }

    
    virtual void shrink_to_fit() {
        sparse_container_type other{sparse.get_allocator()};
        const auto len = sparse.size();
        size_type cnt{};

        other.reserve(len);

        for(auto &&elem: std::as_const(packed)) {
            if(elem != tombstone) {
                if(const auto page = pos_to_page(entity_to_pos(elem)); sparse[page] != nullptr) {
                    if(const auto sz = page + 1u; sz > other.size()) {
                        other.resize(sz, nullptr);
                    }

                    other[page] = std::exchange(sparse[page], nullptr);

                    if(++cnt == len) {
                        
                        break;
                    }
                }
            }
        }

        release_sparse_pages();
        sparse.swap(other);

        sparse.shrink_to_fit();
        packed.shrink_to_fit();
    }

    
    [[nodiscard]] size_type extent() const noexcept {
        return sparse.size() * traits_type::page_size;
    }

    
    [[nodiscard]] size_type size() const noexcept {
        return packed.size();
    }

    
    [[nodiscard]] bool empty() const noexcept {
        return packed.empty();
    }

    
    [[nodiscard]] bool contiguous() const noexcept {
        return (mode != deletion_policy::in_place) || (head == max_size);
    }

    
    [[nodiscard]] pointer data() const noexcept {
        return packed.data();
    }

    
    [[nodiscard]] iterator begin() const noexcept {
        const auto pos = static_cast<difference_type>(packed.size());
        return iterator{packed, pos};
    }

    
    [[nodiscard]] const_iterator cbegin() const noexcept {
        return begin();
    }

    
    [[nodiscard]] iterator end() const noexcept {
        return iterator{packed, {}};
    }

    
    [[nodiscard]] const_iterator cend() const noexcept {
        return end();
    }

    
    [[nodiscard]] reverse_iterator rbegin() const noexcept {
        return std::make_reverse_iterator(end());
    }

    
    [[nodiscard]] const_reverse_iterator crbegin() const noexcept {
        return rbegin();
    }

    
    [[nodiscard]] reverse_iterator rend() const noexcept {
        return std::make_reverse_iterator(begin());
    }

    
    [[nodiscard]] const_reverse_iterator crend() const noexcept {
        return rend();
    }

    
    [[nodiscard]] const_iterator find(const entity_type entt) const noexcept {
        return contains(entt) ? to_iterator(entt) : end();
    }

    
    [[nodiscard]] bool contains(const entity_type entt) const noexcept {
        const auto *elem = sparse_ptr(entt);
        constexpr auto cap = traits_type::entity_mask;
        constexpr auto mask = traits_type::to_integral(null) & ~cap;
        
        return elem && (((mask & traits_type::to_integral(entt)) ^ traits_type::to_integral(*elem)) < cap);
    }

    
    [[nodiscard]] version_type current(const entity_type entt) const noexcept {
        const auto *elem = sparse_ptr(entt);
        constexpr auto fallback = traits_type::to_version(tombstone);
        return elem ? traits_type::to_version(*elem) : fallback;
    }

    
    [[nodiscard]] size_type index(const entity_type entt) const noexcept {
        ENTT_ASSERT(contains(entt), "Set does not contain entity");
        return entity_to_pos(sparse_ref(entt));
    }

    
    [[nodiscard]] entity_type operator[](const size_type pos) const noexcept {
        ENTT_ASSERT(pos < packed.size(), "Index out of bounds");
        return packed[pos];
    }

    
    [[nodiscard]] const void *value(const entity_type entt) const noexcept {
        return get_at(index(entt));
    }

    
    [[nodiscard]] void *value(const entity_type entt) noexcept {
        return const_cast<void *>(std::as_const(*this).value(entt));
    }

    
    iterator push(const entity_type entt, const void *elem = nullptr) {
        return try_emplace(entt, false, elem);
    }

    
    template<typename It>
    iterator push(It first, It last) {
        auto curr = end();

        for(; first != last; ++first) {
            curr = try_emplace(*first, true);
        }

        return curr;
    }

    
    version_type bump(const entity_type entt) {
        auto &elem = sparse_ref(entt);
        ENTT_ASSERT(entt != null && elem != tombstone, "Cannot set the required version");
        elem = traits_type::combine(traits_type::to_integral(elem), traits_type::to_integral(entt));
        packed[entity_to_pos(elem)] = entt;
        return traits_type::to_version(entt);
    }

    
    void erase(const entity_type entt) {
        const auto it = to_iterator(entt);
        pop(it, it + 1u);
    }

    
    template<typename It>
    void erase(It first, It last) {
        if constexpr(std::is_same_v<It, basic_iterator>) {
            pop(first, last);
        } else {
            for(; first != last; ++first) {
                erase(*first);
            }
        }
    }

    
    bool remove(const entity_type entt) {
        return contains(entt) && (erase(entt), true);
    }

    
    template<typename It>
    size_type remove(It first, It last) {
        size_type count{};

        if constexpr(std::is_same_v<It, basic_iterator>) {
            while(first != last) {
                while(first != last && !contains(*first)) {
                    ++first;
                }

                const auto it = first;

                while(first != last && contains(*first)) {
                    ++first;
                }

                count += static_cast<size_type>(std::distance(it, first));
                erase(it, first);
            }
        } else {
            for(; first != last; ++first) {
                count += remove(*first);
            }
        }

        return count;
    }

    
    void compact() {
        if(mode == deletion_policy::in_place) {
            size_type from = packed.size();
            size_type pos = std::exchange(head, max_size);

            for(; from && packed[from - 1u] == tombstone; --from) {}

            while(pos != max_size) {
                if(const auto to = std::exchange(pos, entity_to_pos(packed[pos])); to < from) {
                    --from;
                    swap_or_move(from, to);

                    packed[to] = packed[from];
                    const auto elem = static_cast<typename traits_type::entity_type>(to);
                    sparse_ref(packed[to]) = traits_type::combine(elem, traits_type::to_integral(packed[to]));

                    for(; from && packed[from - 1u] == tombstone; --from) {}
                }
            }

            packed.erase(packed.begin() + static_cast<difference_type>(from), packed.end());
        }
    }

    
    void swap_elements(const entity_type lhs, const entity_type rhs) {
        const auto from = index(lhs);
        const auto to = index(rhs);

        
        swap_or_move(from, to);
        swap_at(from, to);
    }

    
    template<typename Compare, typename Sort = std_sort, typename... Args>
    void sort_n(const size_type length, Compare compare, Sort algo = Sort{}, Args &&...args) {
        ENTT_ASSERT((mode != deletion_policy::in_place) || (head == max_size), "Sorting with tombstones not allowed");
        ENTT_ASSERT(!(length > packed.size()), "Length exceeds the number of elements");

        algo(packed.rend() - static_cast<difference_type>(length), packed.rend(), std::move(compare), std::forward<Args>(args)...);

        for(size_type pos{}; pos < length; ++pos) {
            auto curr = pos;
            auto next = index(packed[curr]);

            while(curr != next) {
                const auto idx = index(packed[next]);
                const auto entt = packed[curr];

                swap_or_move(next, idx);
                const auto elem = static_cast<typename traits_type::entity_type>(curr);
                sparse_ref(entt) = traits_type::combine(elem, traits_type::to_integral(packed[curr]));
                curr = std::exchange(next, idx);
            }
        }
    }

    
    template<typename Compare, typename Sort = std_sort, typename... Args>
    void sort(Compare compare, Sort algo = Sort{}, Args &&...args) {
        const size_type len = (mode == deletion_policy::swap_only) ? head : packed.size();
        sort_n(len, std::move(compare), std::move(algo), std::forward<Args>(args)...);
    }

    
    template<typename It>
    iterator sort_as(It first, It last) {
        ENTT_ASSERT((mode != deletion_policy::in_place) || (head == max_size), "Sorting with tombstones not allowed");
        const size_type len = (mode == deletion_policy::swap_only) ? head : packed.size();
        auto it = end() - static_cast<difference_type>(len);

        for(const auto other = end(); (it != other) && (first != last); ++first) {
            if(const auto curr = *first; contains(curr)) {
                if(const auto entt = *it; entt != curr) {
                    
                    swap_elements(entt, curr);
                }

                ++it;
            }
        }

        return it;
    }

    
    void clear() {
        pop_all();
        
        ENTT_ASSERT((compact(), size()) == 0u, "Non-empty set");
        head = policy_to_head();
        packed.clear();
    }

    
    [[nodiscard]] const type_info &info() const noexcept {
        return *descriptor;
    }

    
    [[deprecated("use ::info instead")]] [[nodiscard]] const type_info &type() const noexcept {
        return info();
    }

    
    template<typename Type>
    void bind(Type &&value) noexcept {
        bind_any(forward_as_any(std::forward<Type>(value)));
    }

private:
    sparse_container_type sparse;
    packed_container_type packed;
    const type_info *descriptor;
    deletion_policy mode;
    size_type head;
};

} 

#endif
