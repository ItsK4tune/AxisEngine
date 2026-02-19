#ifndef ENTT_META_RESOLVE_HPP
#define ENTT_META_RESOLVE_HPP

#include <type_traits>
#include "../core/type_info.hpp"
#include "../locator/locator.hpp"
#include "context.hpp"
#include "meta.hpp"
#include "node.hpp"
#include "range.hpp"

namespace entt {


template<typename Type>
[[nodiscard]] meta_type resolve(const meta_ctx &ctx) noexcept {
    const auto &context = internal::meta_context::from(ctx);
    return {ctx, internal::resolve<std::remove_const_t<std::remove_reference_t<Type>>>(context)};
}


template<typename Type>
[[nodiscard]] meta_type resolve() noexcept {
    return resolve<Type>(locator<meta_ctx>::value_or());
}


[[nodiscard]] inline meta_range<meta_type, typename decltype(internal::meta_context::value)::const_iterator> resolve(const meta_ctx &ctx) noexcept {
    const auto &context = internal::meta_context::from(ctx);
    return {{ctx, context.value.cbegin()}, {ctx, context.value.cend()}};
}


[[nodiscard]] inline meta_range<meta_type, typename decltype(internal::meta_context::value)::const_iterator> resolve() noexcept {
    return resolve(locator<meta_ctx>::value_or());
}


[[nodiscard]] inline meta_type resolve(const meta_ctx &ctx, const id_type id) noexcept {
    for(auto &&curr: resolve(ctx)) {
        if(curr.second.id() == id) {
            return curr.second;
        }
    }

    return meta_type{};
}


[[nodiscard]] inline meta_type resolve(const id_type id) noexcept {
    return resolve(locator<meta_ctx>::value_or(), id);
}


[[nodiscard]] inline meta_type resolve(const meta_ctx &ctx, const type_info &info) noexcept {
    const auto &context = internal::meta_context::from(ctx);
    const auto *elem = internal::try_resolve(context, info);
    return (elem != nullptr) ? meta_type{ctx, *elem} : meta_type{};
}


[[nodiscard]] inline meta_type resolve(const type_info &info) noexcept {
    return resolve(locator<meta_ctx>::value_or(), info);
}

} 

#endif
