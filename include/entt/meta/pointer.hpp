

#ifndef ENTT_META_POINTER_HPP
#define ENTT_META_POINTER_HPP

#include <memory>
#include <type_traits>
#include "type_traits.hpp"

namespace entt {


template<typename Type>
struct is_meta_pointer_like<Type *>
    : std::true_type {};


template<typename Type, std::size_t N>

struct is_meta_pointer_like<Type (*)[N]>
    : std::false_type {};


template<typename Type>
struct is_meta_pointer_like<std::shared_ptr<Type>>
    : std::true_type {};


template<typename Type, typename... Args>
struct is_meta_pointer_like<std::unique_ptr<Type, Args...>>
    : std::true_type {};


template<typename Type>
struct is_meta_pointer_like<Type, std::void_t<typename Type::is_meta_pointer_like>>
    : std::true_type {};

} 

#endif
