#ifndef ENTT_META_POLICY_HPP
#define ENTT_META_POLICY_HPP

#include <type_traits>

namespace entt {


namespace internal {

struct meta_policy {};

} 



struct as_value_t final: private internal::meta_policy {
    
    template<typename>
    static constexpr bool value = true;
    
};


struct as_void_t final: private internal::meta_policy {
    
    template<typename>
    static constexpr bool value = true;
    
};


struct as_ref_t final: private internal::meta_policy {
    
    template<typename Type>
    static constexpr bool value = std::is_reference_v<Type> && !std::is_const_v<std::remove_reference_t<Type>>;
    
};


struct as_cref_t final: private internal::meta_policy {
    
    template<typename Type>
    static constexpr bool value = std::is_reference_v<Type>;
    
};


struct as_is_t final: private internal::meta_policy {
    
    template<typename>
    static constexpr bool value = true;
    
};


template<typename Type>
struct is_meta_policy
    : std::bool_constant<std::is_base_of_v<internal::meta_policy, Type>> {};


template<typename Type>
inline constexpr bool is_meta_policy_v = is_meta_policy<Type>::value;

} 

#endif
