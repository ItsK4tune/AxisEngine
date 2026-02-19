#ifndef ENTT_CORE_FWD_HPP
#define ENTT_CORE_FWD_HPP

#include <cstddef>
#include <cstdint>
#include "../config/config.h"

namespace entt {


enum class any_policy : std::uint8_t {
    
    empty,
    
    dynamic,
    
    embedded,
    
    ref,
    
    cref
};


template<std::size_t Len = sizeof(double[2]), std::size_t = alignof(double[2])>
class basic_any;


using id_type = ENTT_ID_TYPE;


using any = basic_any<>;

template<typename, typename>
class compressed_pair;

template<typename>
class basic_hashed_string;


using hashed_string = basic_hashed_string<char>;


using hashed_wstring = basic_hashed_string<wchar_t>;


struct type_info;

} 

#endif
