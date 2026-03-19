#ifndef ENTT_GRAPH_FWD_HPP
#define ENTT_GRAPH_FWD_HPP

#include <cstddef>
#include <memory>
#include "../core/fwd.hpp"

namespace entt {


struct directed_tag {};


struct undirected_tag: directed_tag {};

template<typename, typename = std::allocator<std::size_t>>
class adjacency_matrix;

template<typename = std::allocator<id_type>>
class basic_flow;


using flow = basic_flow<>;

} 

#endif
