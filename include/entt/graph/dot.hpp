#ifndef ENTT_GRAPH_DOT_HPP
#define ENTT_GRAPH_DOT_HPP

#include <ostream>
#include <type_traits>
#include "fwd.hpp"

namespace entt {


template<typename Graph, typename Writer>
void dot(std::ostream &out, const Graph &graph, Writer writer) {
    static_assert(std::is_base_of_v<directed_tag, typename Graph::graph_category>, "Invalid graph category");

    if constexpr(std::is_same_v<typename Graph::graph_category, undirected_tag>) {
        out << "graph{";
    } else {
        out << "digraph{";
    }

    for(auto &&vertex: graph.vertices()) {
        out << vertex << "[";
        writer(out, vertex);
        out << "];";
    }

    for(auto [lhs, rhs]: graph.edges()) {
        if constexpr(std::is_same_v<typename Graph::graph_category, undirected_tag>) {
            out << lhs << "--" << rhs << ";";
        } else {
            out << lhs << "->" << rhs << ";";
        }
    }

    out << "}";
}


template<typename Graph>
void dot(std::ostream &out, const Graph &graph) {
    return dot(out, graph, [](auto &&...) {});
}

} 

#endif
