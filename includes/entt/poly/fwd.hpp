#ifndef ENTT_POLY_FWD_HPP
#define ENTT_POLY_FWD_HPP

#include <cstddef>

namespace entt {


template<typename, std::size_t Len = sizeof(double[2]), std::size_t = alignof(double[2])>
class basic_poly;


template<typename Concept>
using poly = basic_poly<Concept>;

} 

#endif
