#ifndef ENTT_META_ADL_POINTER_HPP
#define ENTT_META_ADL_POINTER_HPP

namespace entt {


template<typename Type>
decltype(auto) dereference_meta_pointer_like(const Type &value) {
    return *value;
}


template<typename Type>
struct adl_meta_pointer_like {
    
    static decltype(auto) dereference(const Type &value) {
        return dereference_meta_pointer_like(value);
    }
};

} 

#endif
