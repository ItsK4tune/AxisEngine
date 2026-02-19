#ifndef ENTT_GRAPH_ADJACENCY_MATRIX_HPP
#define ENTT_GRAPH_ADJACENCY_MATRIX_HPP

#include <cstddef>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>
#include "../config/config.h"
#include "../core/iterator.hpp"
#include "fwd.hpp"

namespace entt {


namespace internal {

template<typename It>
class edge_iterator {
    using size_type = std::size_t;

    void find_next() noexcept {
        for(; pos != last && !it[static_cast<typename It::difference_type>(pos)]; pos += offset) {}
    }

public:
    using value_type = std::pair<size_type, size_type>;
    using pointer = input_iterator_pointer<value_type>;
    using reference = value_type;
    using difference_type = std::ptrdiff_t;
    using iterator_category = std::input_iterator_tag;
    using iterator_concept = std::forward_iterator_tag;

    constexpr edge_iterator() noexcept = default;

    
    constexpr edge_iterator(It base, const size_type vertices, const size_type from, const size_type to, const size_type step) noexcept
        : it{std::move(base)},
          vert{vertices},
          pos{from},
          last{to},
          offset{step} {
        find_next();
    }

    constexpr edge_iterator &operator++() noexcept {
        pos += offset;
        find_next();
        return *this;
    }

    constexpr edge_iterator operator++(int) noexcept {
        const edge_iterator orig = *this;
        return ++(*this), orig;
    }

    [[nodiscard]] constexpr reference operator*() const noexcept {
        return *operator->();
    }

    [[nodiscard]] constexpr pointer operator->() const noexcept {
        return std::make_pair<size_type>(pos / vert, pos % vert);
    }

    template<typename Type>
    friend constexpr bool operator==(const edge_iterator<Type> &, const edge_iterator<Type> &) noexcept;

private:
    It it{};
    size_type vert{};
    size_type pos{};
    size_type last{};
    size_type offset{};
};

template<typename Container>
[[nodiscard]] constexpr bool operator==(const edge_iterator<Container> &lhs, const edge_iterator<Container> &rhs) noexcept {
    return lhs.pos == rhs.pos;
}

template<typename Container>
[[nodiscard]] constexpr bool operator!=(const edge_iterator<Container> &lhs, const edge_iterator<Container> &rhs) noexcept {
    return !(lhs == rhs);
}

} 



template<typename Category, typename Allocator>
class adjacency_matrix {
    using alloc_traits = std::allocator_traits<Allocator>;
    static_assert(std::is_base_of_v<directed_tag, Category>, "Invalid graph category");
    static_assert(std::is_same_v<typename alloc_traits::value_type, std::size_t>, "Invalid value type");
    using container_type = std::vector<std::size_t, typename alloc_traits::template rebind_alloc<std::size_t>>;

public:
    
    using allocator_type = Allocator;
    
    using size_type = std::size_t;
    
    using vertex_type = size_type;
    
    using edge_type = std::pair<vertex_type, vertex_type>;
    
    using vertex_iterator = iota_iterator<vertex_type>;
    
    using edge_iterator = internal::edge_iterator<typename container_type::const_iterator>;
    
    using out_edge_iterator = edge_iterator;
    
    using in_edge_iterator = edge_iterator;
    
    using graph_category = Category;

    
    adjacency_matrix() noexcept(noexcept(allocator_type{}))
        : adjacency_matrix{0u} {
    }

    
    explicit adjacency_matrix(const allocator_type &allocator) noexcept
        : adjacency_matrix{0u, allocator} {}

    
    adjacency_matrix(const size_type vertices, const allocator_type &allocator = allocator_type{})
        : matrix{vertices * vertices, allocator},
          vert{vertices} {}

    
    adjacency_matrix(const adjacency_matrix &) = default;

    
    adjacency_matrix(const adjacency_matrix &other, const allocator_type &allocator)
        : matrix{other.matrix, allocator},
          vert{other.vert} {}

    
    adjacency_matrix(adjacency_matrix &&) noexcept = default;

    
    adjacency_matrix(adjacency_matrix &&other, const allocator_type &allocator)
        : matrix{std::move(other.matrix), allocator},
          vert{other.vert} {}

    
    ~adjacency_matrix() = default;

    
    adjacency_matrix &operator=(const adjacency_matrix &) = default;

    
    adjacency_matrix &operator=(adjacency_matrix &&) noexcept = default;

    
    void swap(adjacency_matrix &other) noexcept {
        using std::swap;
        swap(matrix, other.matrix);
        swap(vert, other.vert);
    }

    
    [[nodiscard]] constexpr allocator_type get_allocator() const noexcept {
        return matrix.get_allocator();
    }

    
    void clear() noexcept {
        matrix.clear();
        vert = {};
    }

    
    [[nodiscard]] bool empty() const noexcept {
        const auto iterable = edges();
        return (iterable.begin() == iterable.end());
    }

    
    [[nodiscard]] size_type size() const noexcept {
        return vert;
    }

    
    [[nodiscard]] iterable_adaptor<vertex_iterator> vertices() const noexcept {
        return {0u, vert};
    }

    
    [[nodiscard]] iterable_adaptor<edge_iterator> edges() const noexcept {
        const auto it = matrix.cbegin();
        const auto sz = matrix.size();
        return {{it, vert, 0u, sz, 1u}, {it, vert, sz, sz, 1u}};
    }

    
    [[nodiscard]] iterable_adaptor<out_edge_iterator> out_edges(const vertex_type vertex) const noexcept {
        const auto it = matrix.cbegin();
        const auto from = vertex * vert;
        const auto to = from + vert;
        return {{it, vert, from, to, 1u}, {it, vert, to, to, 1u}};
    }

    
    [[nodiscard]] iterable_adaptor<in_edge_iterator> in_edges(const vertex_type vertex) const noexcept {
        const auto it = matrix.cbegin();
        const auto from = vertex;
        const auto to = vert * vert + from;
        return {{it, vert, from, to, vert}, {it, vert, to, to, vert}};
    }

    
    void resize(const size_type vertices) {
        adjacency_matrix other{vertices, get_allocator()};

        for(auto [lhs, rhs]: edges()) {
            other.insert(lhs, rhs);
        }

        other.swap(*this);
    }

    
    std::pair<edge_iterator, bool> insert(const vertex_type lhs, const vertex_type rhs) {
        const auto pos = lhs * vert + rhs;

        if constexpr(std::is_same_v<graph_category, undirected_tag>) {
            const auto rev = rhs * vert + lhs;
            ENTT_ASSERT(matrix[pos] == matrix[rev], "Something went really wrong");
            matrix[rev] = 1u;
        }

        const auto inserted = !std::exchange(matrix[pos], 1u);
        return {edge_iterator{matrix.cbegin(), vert, pos, matrix.size(), 1u}, inserted};
    }

    
    size_type erase(const vertex_type lhs, const vertex_type rhs) {
        const auto pos = lhs * vert + rhs;

        if constexpr(std::is_same_v<graph_category, undirected_tag>) {
            const auto rev = rhs * vert + lhs;
            ENTT_ASSERT(matrix[pos] == matrix[rev], "Something went really wrong");
            matrix[rev] = 0u;
        }

        return std::exchange(matrix[pos], 0u);
    }

    
    [[nodiscard]] bool contains(const vertex_type lhs, const vertex_type rhs) const {
        const auto pos = lhs * vert + rhs;
        return pos < matrix.size() && matrix[pos];
    }

private:
    container_type matrix;
    size_type vert;
};

} 

#endif
