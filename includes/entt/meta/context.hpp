#ifndef ENTT_META_CTX_HPP
#define ENTT_META_CTX_HPP

#include <memory>
#include "../container/dense_map.hpp"
#include "../core/fwd.hpp"
#include "../core/utility.hpp"
#include "fwd.hpp"

namespace entt {


namespace internal {

struct meta_type_node;

struct meta_context {
    dense_map<id_type, std::unique_ptr<meta_type_node>, identity> value;

    [[nodiscard]] inline static meta_context &from(meta_ctx &);
    [[nodiscard]] inline static const meta_context &from(const meta_ctx &);
};

} 



class meta_ctx_arg_t final {};


inline constexpr meta_ctx_arg_t meta_ctx_arg{};


class meta_ctx: private internal::meta_context {
    
    friend struct internal::meta_context;
};


[[nodiscard]] inline internal::meta_context &internal::meta_context::from(meta_ctx &ctx) {
    return ctx;
}

[[nodiscard]] inline const internal::meta_context &internal::meta_context::from(const meta_ctx &ctx) {
    return ctx;
}


} 

#endif
