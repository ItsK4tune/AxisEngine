#ifndef ENTT_META_FACTORY_HPP
#define ENTT_META_FACTORY_HPP

#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "../core/bit.hpp"
#include "../core/fwd.hpp"
#include "../core/hashed_string.hpp"
#include "../core/type_info.hpp"
#include "../core/type_traits.hpp"
#include "../locator/locator.hpp"
#include "context.hpp"
#include "fwd.hpp"
#include "meta.hpp"
#include "node.hpp"
#include "policy.hpp"
#include "range.hpp"
#include "resolve.hpp"
#include "utility.hpp"

namespace entt {


namespace internal {

class basic_meta_factory {
    using invoke_type = std::remove_pointer_t<decltype(meta_func_node::invoke)>;

    [[nodiscard]] auto &fetch_node() noexcept {
        return *meta_context::from(*ctx).value[parent];
    }

    [[nodiscard]] auto *find_member_or_assert() {
        auto *member = find_member<&meta_data_node::id>(fetch_node().details->data, bucket);
        ENTT_ASSERT(member != nullptr, "Cannot find member");
        return member;
    }

    [[nodiscard]] auto *find_overload_or_assert() {
        auto *overload = find_overload(find_member<&meta_func_node::id>(fetch_node().details->func, bucket), invoke);
        ENTT_ASSERT(overload != nullptr, "Cannot find overload");
        return overload;
    }

    void reset_bucket(const id_type id, invoke_type *const ref = nullptr) {
        invoke = ref;
        bucket = id;
    }

protected:
    void type(const id_type id, const char *name) noexcept {
        reset_bucket(parent);
        auto &elem = fetch_node();
        ENTT_ASSERT(elem.id == id || !resolve(*ctx, id), "Duplicate identifier");
        elem.name = name;
        elem.id = id;
    }

    template<typename Type>
    void insert_or_assign(Type node) {
        auto &elem = fetch_node();

        reset_bucket(parent);

        if constexpr(std::is_same_v<Type, meta_base_node>) {
            auto *member = find_member<&meta_base_node::type>(elem.details->base, node.type);
            member ? (*member = node) : elem.details->base.emplace_back(node);
        } else if constexpr(std::is_same_v<Type, meta_conv_node>) {
            auto *member = find_member<&meta_conv_node::type>(elem.details->conv, node.type);
            member ? (*member = node) : elem.details->conv.emplace_back(node);
        } else {
            static_assert(std::is_same_v<Type, meta_ctor_node>, "Unexpected type");
            auto *member = find_member<&meta_ctor_node::id>(elem.details->ctor, node.id);
            member ? (*member = node) : elem.details->ctor.emplace_back(node);
        }
    }

    void data(meta_data_node node) {
        auto &elem = fetch_node();

        reset_bucket(node.id);

        if(auto *member = find_member<&meta_data_node::id>(elem.details->data, node.id); member == nullptr) {
            elem.details->data.emplace_back(std::move(node));
        } else if(member->set != node.set || member->get != node.get) {
            *member = std::move(node);
        }
    }

    void func(meta_func_node node) {
        auto &elem = fetch_node();

        reset_bucket(node.id, node.invoke);

        if(auto *member = find_member<&meta_func_node::id>(elem.details->func, node.id); member == nullptr) {
            elem.details->func.emplace_back(std::move(node));
        } else if(auto *overload = find_overload(member, node.invoke); overload == nullptr) {
            while(member->next != nullptr) { member = member->next.get(); }
            member->next = std::make_unique<meta_func_node>(std::move(node));
        }
    }

    void traits(const meta_traits value, const bool unset) {
        auto set_or_unset_on = [=](auto &node) {
            node.traits = (unset ? (node.traits & ~value) : (node.traits | value));
        };

        if(bucket == parent) {
            set_or_unset_on(fetch_node());
        } else if(invoke == nullptr) {
            set_or_unset_on(*find_member_or_assert());
        } else {
            set_or_unset_on(*find_overload_or_assert());
        }
    }

    void custom(meta_custom_node node) {
        if(bucket == parent) {
            fetch_node().custom = std::move(node);
        } else if(invoke == nullptr) {
            find_member_or_assert()->custom = std::move(node);
        } else {
            find_overload_or_assert()->custom = std::move(node);
        }
    }

public:
    basic_meta_factory(meta_ctx &area, meta_type_node node)
        : ctx{&area},
          parent{node.info->hash()},
          bucket{parent} {
        if(auto *curr = meta_context::from(*ctx).value.try_emplace(parent, std::make_unique<meta_type_node>(std::move(node))).first->second.get(); curr->details == nullptr) {
            curr->details = std::make_unique<meta_type_descriptor>();
        }
    }

private:
    meta_ctx *ctx{};
    id_type parent{};
    id_type bucket{};
    invoke_type *invoke{};
};

} 



template<typename Type>
class meta_factory: private internal::basic_meta_factory {
    using base_type = internal::basic_meta_factory;

public:
    
    using element_type = Type;

    
    meta_factory() noexcept
        : meta_factory{locator<meta_ctx>::value_or()} {}

    
    meta_factory(meta_ctx &area) noexcept
        : internal::basic_meta_factory{area, internal::setup_node_for<Type>()} {}

    
    meta_factory type(const char *name) noexcept {
        return type(hashed_string::value(name), name);
    }

    
    meta_factory type(const id_type id, const char *name = nullptr) noexcept {
        base_type::type(id, name);
        return *this;
    }

    
    template<typename Base>
    meta_factory base() noexcept {
        static_assert(!std::is_same_v<Type, Base> && std::is_base_of_v<Base, Type>, "Invalid base type");
        auto *const op = +[](const void *instance) noexcept { return static_cast<const void *>(static_cast<const Base *>(static_cast<const Type *>(instance))); };
        base_type::insert_or_assign(internal::meta_base_node{type_id<Base>().hash(), &internal::resolve<Base>, op});
        return *this;
    }

    
    template<auto Candidate>
    auto conv() noexcept {
        using conv_type = std::remove_const_t<std::remove_reference_t<std::invoke_result_t<decltype(Candidate), Type &>>>;
        auto *const op = +[](const meta_ctx &area, const void *instance) { return forward_as_meta(area, std::invoke(Candidate, *static_cast<const Type *>(instance))); };
        base_type::insert_or_assign(internal::meta_conv_node{type_id<conv_type>().hash(), op});
        return *this;
    }

    
    template<typename To>
    meta_factory conv() noexcept {
        using conv_type = std::remove_const_t<std::remove_reference_t<To>>;
        auto *const op = +[](const meta_ctx &area, const void *instance) { return forward_as_meta(area, static_cast<To>(*static_cast<const Type *>(instance))); };
        base_type::insert_or_assign(internal::meta_conv_node{type_id<conv_type>().hash(), op});
        return *this;
    }

    
    template<auto Candidate, typename Policy = as_value_t>
    meta_factory ctor() noexcept {
        using descriptor = meta_function_helper_t<Type, decltype(Candidate)>;
        static_assert(Policy::template value<typename descriptor::return_type>, "Invalid return type for the given policy");
        static_assert(std::is_same_v<std::remove_const_t<std::remove_reference_t<typename descriptor::return_type>>, Type>, "The function doesn't return an object of the required type");
        base_type::insert_or_assign(internal::meta_ctor_node{type_id<typename descriptor::args_type>().hash(), descriptor::args_type::size, &meta_arg<typename descriptor::args_type>, &meta_construct<Type, Candidate, Policy>});
        return *this;
    }

    
    template<typename... Args>
    meta_factory ctor() noexcept {
        
        if constexpr(sizeof...(Args) != 0u) {
            using descriptor = meta_function_helper_t<Type, Type (*)(Args...)>;
            base_type::insert_or_assign(internal::meta_ctor_node{type_id<typename descriptor::args_type>().hash(), descriptor::args_type::size, &meta_arg<typename descriptor::args_type>, &meta_construct<Type, Args...>});
        }

        return *this;
    }

    
    template<auto Data, typename Policy = as_value_t>
    meta_factory data(const char *name) noexcept {
        return data<Data, Policy>(hashed_string::value(name), name);
    }

    
    template<auto Data, typename Policy = as_value_t>
    meta_factory data(const id_type id, const char *name = nullptr) noexcept {
        if constexpr(std::is_member_object_pointer_v<decltype(Data)>) {
            using data_type = std::invoke_result_t<decltype(Data), Type &>;
            static_assert(Policy::template value<data_type>, "Invalid return type for the given policy");

            base_type::data(
                internal::meta_data_node{
                    id,
                    name,
                    
                    std::is_const_v<std::remove_reference_t<data_type>> ? internal::meta_traits::is_const : internal::meta_traits::is_none,
                    1u,
                    &internal::resolve<std::remove_const_t<std::remove_reference_t<data_type>>>,
                    &meta_arg<type_list<std::remove_const_t<std::remove_reference_t<data_type>>>>,
                    &meta_setter<Type, Data>,
                    &meta_getter<Type, Data, Policy>});
        } else {
            using data_type = std::remove_pointer_t<decltype(Data)>;

            if constexpr(std::is_pointer_v<decltype(Data)>) {
                static_assert(Policy::template value<decltype(*Data)>, "Invalid return type for the given policy");
            } else {
                static_assert(Policy::template value<data_type>, "Invalid return type for the given policy");
            }

            base_type::data(
                internal::meta_data_node{
                    id,
                    name,
                    ((!std::is_pointer_v<decltype(Data)> || std::is_const_v<data_type>) ? internal::meta_traits::is_const : internal::meta_traits::is_none) | internal::meta_traits::is_static,
                    1u,
                    &internal::resolve<std::remove_const_t<std::remove_reference_t<data_type>>>,
                    &meta_arg<type_list<std::remove_const_t<std::remove_reference_t<data_type>>>>,
                    &meta_setter<Type, Data>,
                    &meta_getter<Type, Data, Policy>});
        }

        return *this;
    }

    
    template<auto Setter, auto Getter, typename Policy = as_value_t>
    meta_factory data(const char *name) noexcept {
        return data<Setter, Getter, Policy>(hashed_string::value(name), name);
    }

    
    template<auto Setter, auto Getter, typename Policy = as_value_t>
    meta_factory data(const id_type id, const char *name = nullptr) noexcept {
        using descriptor = meta_function_helper_t<Type, decltype(Getter)>;
        static_assert(Policy::template value<typename descriptor::return_type>, "Invalid return type for the given policy");

        if constexpr(std::is_same_v<decltype(Setter), std::nullptr_t>) {
            base_type::data(
                internal::meta_data_node{
                    id,
                    name,
                    
                    internal::meta_traits::is_const,
                    0u,
                    &internal::resolve<std::remove_const_t<std::remove_reference_t<typename descriptor::return_type>>>,
                    &meta_arg<type_list<>>,
                    &meta_setter<Type, Setter>,
                    &meta_getter<Type, Getter, Policy>});
        } else {
            using args_type = typename meta_function_helper_t<Type, decltype(Setter)>::args_type;

            base_type::data(
                internal::meta_data_node{
                    id,
                    name,
                    
                    internal::meta_traits::is_none,
                    1u,
                    &internal::resolve<std::remove_const_t<std::remove_reference_t<typename descriptor::return_type>>>,
                    &meta_arg<type_list<type_list_element_t<static_cast<std::size_t>(args_type::size != 1u), args_type>>>,
                    &meta_setter<Type, Setter>,
                    &meta_getter<Type, Getter, Policy>});
        }

        return *this;
    }

    
    template<auto Candidate, typename Policy = as_value_t>
    meta_factory func(const char *name) noexcept {
        return func<Candidate, Policy>(hashed_string::value(name), name);
    }

    
    template<auto Candidate, typename Policy = as_value_t>
    meta_factory func(const id_type id, const char *name = nullptr) noexcept {
        using descriptor = meta_function_helper_t<Type, decltype(Candidate)>;
        static_assert(Policy::template value<typename descriptor::return_type>, "Invalid return type for the given policy");

        base_type::func(
            internal::meta_func_node{
                id,
                name,
                (descriptor::is_const ? internal::meta_traits::is_const : internal::meta_traits::is_none) | (descriptor::is_static ? internal::meta_traits::is_static : internal::meta_traits::is_none),
                descriptor::args_type::size,
                &internal::resolve<std::conditional_t<std::is_same_v<Policy, as_void_t>, void, std::remove_const_t<std::remove_reference_t<typename descriptor::return_type>>>>,
                &meta_arg<typename descriptor::args_type>,
                &meta_invoke<Type, Candidate, Policy>});

        return *this;
    }

    
    template<typename Value>
    meta_factory traits(const Value value, const bool unset = false) {
        static_assert(std::is_enum_v<Value>, "Invalid enum type");
        base_type::traits(internal::user_to_meta_traits(value), unset);
        return *this;
    }

    
    template<typename Value, typename... Args>
    meta_factory custom(Args &&...args) {
        base_type::custom(internal::meta_custom_node{type_id<Value>().hash(), std::make_shared<Value>(std::forward<Args>(args)...)});
        return *this;
    }
};


inline void meta_reset(meta_ctx &ctx, const id_type id) noexcept {
    auto &context = internal::meta_context::from(ctx);

    for(auto it = context.value.begin(); it != context.value.end();) {
        if(it->second->id == id) {
            it = context.value.erase(it);
        } else {
            ++it;
        }
    }
}


inline void meta_reset(const id_type id) noexcept {
    meta_reset(locator<meta_ctx>::value_or(), id);
}


template<typename Type>
void meta_reset(meta_ctx &ctx) noexcept {
    internal::meta_context::from(ctx).value.erase(type_id<Type>().hash());
}


template<typename Type>
void meta_reset() noexcept {
    meta_reset<Type>(locator<meta_ctx>::value_or());
}


inline void meta_reset(meta_ctx &ctx) noexcept {
    internal::meta_context::from(ctx).value.clear();
}


inline void meta_reset() noexcept {
    meta_reset(locator<meta_ctx>::value_or());
}

} 

#endif
