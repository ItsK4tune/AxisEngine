#ifndef ENTT_META_TYPE_TRAITS_HPP
#define ENTT_META_TYPE_TRAITS_HPP

#include <type_traits>
#include <utility>

namespace entt {


template<typename>
struct meta_template_traits;


template<typename>
struct meta_sequence_container_traits;


template<typename>
struct meta_associative_container_traits;


template<typename, typename = void>
struct is_meta_pointer_like: std::false_type {};


template<typename Type>
struct is_meta_pointer_like<const Type>: is_meta_pointer_like<Type> {};


template<typename Type>
inline constexpr auto is_meta_pointer_like_v = is_meta_pointer_like<Type>::value;

} 

#endif
