

#ifndef ENTT_META_TEMPLATE_HPP
#define ENTT_META_TEMPLATE_HPP

#include "../core/type_traits.hpp"

namespace entt {


template<template<typename...> class>
struct meta_class_template_tag {};


template<template<typename...> class Clazz, typename... Args>
struct meta_template_traits<Clazz<Args...>> {
    
    using class_type = meta_class_template_tag<Clazz>;
    
    using args_type = type_list<Args...>;
};

} 

#endif
