#ifndef ENTT_CORE_ANY_HPP
#define ENTT_CORE_ANY_HPP

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include "../config/config.h"
#include "fwd.hpp"
#include "type_info.hpp"
#include "type_traits.hpp"
#include "utility.hpp"

namespace entt {


namespace internal {

enum class any_request : std::uint8_t {
    info,
    transfer,
    assign,
    compare,
    copy,
    move
};

template<std::size_t Len, std::size_t Align>
struct basic_any_storage {
    static constexpr bool has_buffer = true;
    union {
        const void *instance{};
        
        alignas(Align) std::byte buffer[Len];
    };
};

template<std::size_t Align>
struct basic_any_storage<0u, Align> {
    static constexpr bool has_buffer = false;
    const void *instance{};
};

template<typename Type, std::size_t Len, std::size_t Align>

struct in_situ: std::bool_constant<(Len != 0u) && alignof(Type) <= Align && sizeof(Type) <= Len && std::is_nothrow_move_constructible_v<Type>> {};

template<std::size_t Len, std::size_t Align>
struct in_situ<void, Len, Align>: std::false_type {};

} 



template<std::size_t Len, std::size_t Align>
class basic_any: private internal::basic_any_storage<Len, Align> {
    using request = internal::any_request;
    using base_type = internal::basic_any_storage<Len, Align>;
    using vtable_type = const void *(const request, const basic_any &, const void *);
    using deleter_type = void(const basic_any &);

    template<typename Type>
    static constexpr bool in_situ_v = internal::in_situ<Type, Len, Align>::value;

    template<typename Type>
    static const void *basic_vtable(const request req, const basic_any &value, const void *other) {
        static_assert(std::is_same_v<std::remove_const_t<std::remove_reference_t<Type>>, Type>, "Invalid type");

        switch(const auto *elem = static_cast<const Type *>(value.data()); req) {
        case request::info:
            return &type_id<Type>();
        case request::transfer:
            if constexpr(std::is_move_assignable_v<Type>) {
                
                *const_cast<Type *>(elem) = std::move(*static_cast<Type *>(const_cast<void *>(other)));
                return other;
            }
            [[fallthrough]];
        case request::assign:
            if constexpr(std::is_copy_assignable_v<Type>) {
                *const_cast<Type *>(elem) = *static_cast<const Type *>(other);
                return other;
            }
            break;
        case request::compare:
            if constexpr(!std::is_function_v<Type> && !std::is_array_v<Type> && is_equality_comparable_v<Type>) {
                return (*elem == *static_cast<const Type *>(other)) ? other : nullptr;
            } else {
                return (elem == other) ? other : nullptr;
            }
        case request::copy:
            if constexpr(std::is_copy_constructible_v<Type>) {
                
                static_cast<basic_any *>(const_cast<void *>(other))->initialize<Type>(*elem);
            }
            break;
        case request::move:
            ENTT_ASSERT(value.mode == any_policy::embedded, "Unexpected policy");
            if constexpr(in_situ_v<Type>) {
                
                return ::new(&static_cast<basic_any *>(const_cast<void *>(other))->buffer) Type{std::move(*const_cast<Type *>(elem))};
            }
        }

        return nullptr;
    }

    template<typename Type>
    static void basic_deleter(const basic_any &value) {
        static_assert(std::is_same_v<std::remove_const_t<std::remove_reference_t<Type>>, Type>, "Invalid type");
        ENTT_ASSERT((value.mode == any_policy::dynamic) || ((value.mode == any_policy::embedded) && !std::is_trivially_destructible_v<Type>), "Unexpected policy");

        const auto *elem = static_cast<const Type *>(value.data());

        if constexpr(in_situ_v<Type>) {
            (value.mode == any_policy::embedded) ? elem->~Type() : (delete elem);
        } else if constexpr(std::is_array_v<Type>) {
            delete[] elem;
        } else {
            delete elem;
        }
    }

    template<typename Type, typename... Args>
    void initialize([[maybe_unused]] Args &&...args) {
        using plain_type = std::remove_const_t<std::remove_reference_t<Type>>;

        vtable = basic_vtable<plain_type>;
        underlying_type = type_hash<plain_type>::value();

        if constexpr(std::is_void_v<Type>) {
            deleter = nullptr;
            mode = any_policy::empty;
            this->instance = nullptr;
        } else if constexpr(std::is_lvalue_reference_v<Type>) {
            deleter = nullptr;
            mode = std::is_const_v<std::remove_reference_t<Type>> ? any_policy::cref : any_policy::ref;
            static_assert((std::is_lvalue_reference_v<Args> && ...) && (sizeof...(Args) == 1u), "Invalid arguments");
            
            this->instance = (std::addressof(args), ...);
        } else if constexpr(in_situ_v<plain_type>) {
            if constexpr(std::is_trivially_destructible_v<plain_type>) {
                deleter = nullptr;
            } else {
                deleter = &basic_deleter<plain_type>;
            }

            mode = any_policy::embedded;

            if constexpr(std::is_aggregate_v<plain_type> && (sizeof...(Args) != 0u || !std::is_default_constructible_v<plain_type>)) {
                ::new(&this->buffer) plain_type{std::forward<Args>(args)...};
            } else {
                
                ::new(&this->buffer) plain_type(std::forward<Args>(args)...);
            }
        } else {
            deleter = &basic_deleter<plain_type>;
            mode = any_policy::dynamic;

            if constexpr(std::is_aggregate_v<plain_type> && (sizeof...(Args) != 0u || !std::is_default_constructible_v<plain_type>)) {
                this->instance = new plain_type{std::forward<Args>(args)...};
            } else if constexpr(std::is_array_v<plain_type>) {
                static_assert(sizeof...(Args) == 0u, "Invalid arguments");
                this->instance = new plain_type[std::extent_v<plain_type>]();
            } else {
                this->instance = new plain_type(std::forward<Args>(args)...);
            }
        }
    }

    void invoke_deleter_if_exists() {
        if(deleter != nullptr) {
            deleter(*this);
        }
    }

public:
    
    static constexpr auto length = Len;
    
    static constexpr auto alignment = Align;

    
    constexpr basic_any() noexcept
        : basic_any{std::in_place_type<void>} {}

    
    template<typename Type, typename... Args>
    explicit basic_any(std::in_place_type_t<Type>, Args &&...args)
        : base_type{} {
        initialize<Type>(std::forward<Args>(args)...);
    }

    
    template<typename Type>
    explicit basic_any(std::in_place_t, Type *value)
        : base_type{} {
        static_assert(!std::is_const_v<Type> && !std::is_void_v<Type>, "Non-const non-void pointer required");

        if(value == nullptr) {
            initialize<void>();
        } else {
            initialize<Type &>(*value);
            deleter = &basic_deleter<Type>;
            mode = any_policy::dynamic;
        }
    }

    
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Type>, basic_any>>>
    basic_any(Type &&value)
        : basic_any{std::in_place_type<std::decay_t<Type>>, std::forward<Type>(value)} {}

    
    basic_any(const basic_any &other)
        : basic_any{} {
        other.vtable(request::copy, other, this);
    }

    
    basic_any(basic_any &&other) noexcept
        : base_type{},
          vtable{other.vtable},
          deleter{other.deleter},
          underlying_type{other.underlying_type},
          mode{other.mode} {
        if(other.mode == any_policy::embedded) {
            other.vtable(request::move, other, this);
        } else if(other.mode != any_policy::empty) {
            this->instance = std::exchange(other.instance, nullptr);
        }
    }

    
    ~basic_any() {
        invoke_deleter_if_exists();
    }

    
    basic_any &operator=(const basic_any &other) {
        if(this != &other) {
            invoke_deleter_if_exists();

            if(other) {
                other.vtable(request::copy, other, this);
            } else {
                initialize<void>();
            }
        }

        return *this;
    }

    
    basic_any &operator=(basic_any &&other) noexcept {
        if(this != &other) {
            invoke_deleter_if_exists();

            if(other.mode == any_policy::embedded) {
                other.vtable(request::move, other, this);
            } else if(other.mode != any_policy::empty) {
                this->instance = std::exchange(other.instance, nullptr);
            }

            vtable = other.vtable;
            deleter = other.deleter;
            underlying_type = other.underlying_type;
            mode = other.mode;
        }

        return *this;
    }

    
    template<typename Type, typename = std::enable_if_t<!std::is_same_v<std::decay_t<Type>, basic_any>>>
    basic_any &operator=(Type &&value) {
        emplace<std::decay_t<Type>>(std::forward<Type>(value));
        return *this;
    }

    
    [[nodiscard]] bool has_value() const noexcept {
        return (mode != any_policy::empty);
    }

    
    [[nodiscard]] bool has_value(const type_info &req) const noexcept {
        return (underlying_type == req.hash());
    }

    
    template<typename Type>
    [[nodiscard]] bool has_value() const noexcept {
        static_assert(std::is_same_v<std::remove_const_t<Type>, Type>, "Invalid type");
        return (underlying_type == type_hash<Type>::value());
    }

    
    [[nodiscard]] const type_info &info() const noexcept {
        return *static_cast<const type_info *>(vtable(request::info, *this, nullptr));
    }

    
    [[deprecated("use ::info instead")]] [[nodiscard]] const type_info &type() const noexcept {
        return info();
    }

    
    [[nodiscard]] const void *data() const noexcept {
        if constexpr(base_type::has_buffer) {
            return (mode == any_policy::embedded) ? &this->buffer : this->instance;
        } else {
            return this->instance;
        }
    }

    
    [[nodiscard]] const void *data(const type_info &req) const noexcept {
        return has_value(req) ? data() : nullptr;
    }

    
    template<typename Type>
    [[nodiscard]] const Type *data() const noexcept {
        return has_value<std::remove_const_t<Type>>() ? static_cast<const Type *>(data()) : nullptr;
    }

    
    [[nodiscard]] void *data() noexcept {
        return (mode == any_policy::cref) ? nullptr : const_cast<void *>(std::as_const(*this).data());
    }

    
    [[nodiscard]] void *data(const type_info &req) noexcept {
        return (mode == any_policy::cref) ? nullptr : const_cast<void *>(std::as_const(*this).data(req));
    }

    
    template<typename Type>
    [[nodiscard]] Type *data() noexcept {
        if constexpr(std::is_const_v<Type>) {
            return std::as_const(*this).template data<std::remove_const_t<Type>>();
        } else {
            return (mode == any_policy::cref) ? nullptr : const_cast<Type *>(std::as_const(*this).template data<std::remove_const_t<Type>>());
        }
    }

    
    template<typename Type, typename... Args>
    void emplace(Args &&...args) {
        invoke_deleter_if_exists();
        initialize<Type>(std::forward<Args>(args)...);
    }

    
    bool assign(const basic_any &other) {
        if(other && (mode != any_policy::cref) && (underlying_type == other.underlying_type)) {
            return (vtable(request::assign, *this, other.data()) != nullptr);
        }

        return false;
    }

    
    
    bool assign(basic_any &&other) {
        if(other && (mode != any_policy::cref) && (underlying_type == other.underlying_type)) {
            return (other.mode == any_policy::cref) ? (vtable(request::assign, *this, std::as_const(other).data()) != nullptr) : (vtable(request::transfer, *this, other.data()) != nullptr);
        }

        return false;
    }

    
    void reset() {
        invoke_deleter_if_exists();
        initialize<void>();
    }

    
    [[nodiscard]] explicit operator bool() const noexcept {
        return has_value();
    }

    
    [[nodiscard]] bool operator==(const basic_any &other) const noexcept {
        if(other && (underlying_type == other.underlying_type)) {
            return (vtable(request::compare, *this, other.data()) != nullptr);
        }

        return (!*this && !other);
    }

    
    [[nodiscard]] bool operator!=(const basic_any &other) const noexcept {
        return !(*this == other);
    }

    
    [[nodiscard]] basic_any as_ref() noexcept {
        basic_any other = std::as_const(*this).as_ref();
        other.mode = (mode == any_policy::cref ? any_policy::cref : any_policy::ref);
        return other;
    }

    
    [[nodiscard]] basic_any as_ref() const noexcept {
        basic_any other{};
        other.instance = data();
        other.vtable = vtable;
        other.underlying_type = underlying_type;
        other.mode = any_policy::cref;
        return other;
    }

    
    [[nodiscard]] bool owner() const noexcept {
        return (mode == any_policy::dynamic || mode == any_policy::embedded);
    }

    
    [[nodiscard]] any_policy policy() const noexcept {
        return mode;
    }

private:
    vtable_type *vtable{};
    deleter_type *deleter{};
    id_type underlying_type{};
    any_policy mode{};
};


template<typename Type, std::size_t Len, std::size_t Align>
[[nodiscard]] std::remove_const_t<Type> any_cast(const basic_any<Len, Align> &data) noexcept {
    const auto *const instance = any_cast<std::remove_reference_t<Type>>(&data);
    ENTT_ASSERT(instance, "Invalid instance");
    return static_cast<Type>(*instance);
}


template<typename Type, std::size_t Len, std::size_t Align>
[[nodiscard]] std::remove_const_t<Type> any_cast(basic_any<Len, Align> &data) noexcept {
    
    auto *const instance = any_cast<std::remove_reference_t<const Type>>(&data);
    ENTT_ASSERT(instance, "Invalid instance");
    return static_cast<Type>(*instance);
}


template<typename Type, std::size_t Len, std::size_t Align>

[[nodiscard]] std::remove_const_t<Type> any_cast(basic_any<Len, Align> &&data) noexcept {
    if constexpr(std::is_copy_constructible_v<std::remove_const_t<std::remove_reference_t<Type>>>) {
        if(auto *const instance = any_cast<std::remove_reference_t<Type>>(&data); instance) {
            return static_cast<Type>(std::move(*instance));
        }

        return any_cast<Type>(data);
    } else {
        auto *const instance = any_cast<std::remove_reference_t<Type>>(&data);
        ENTT_ASSERT(instance, "Invalid instance");
        return static_cast<Type>(std::move(*instance));
    }
}


template<typename Type, std::size_t Len, std::size_t Align>
[[nodiscard]] const Type *any_cast(const basic_any<Len, Align> *data) noexcept {
    return data->template data<std::remove_const_t<Type>>();
}


template<typename Type, std::size_t Len, std::size_t Align>
[[nodiscard]] Type *any_cast(basic_any<Len, Align> *data) noexcept {
    if constexpr(std::is_const_v<Type>) {
        
        return any_cast<Type>(&std::as_const(*data));
    } else {
        return data->template data<Type>();
    }
}


template<typename Type, std::size_t Len = basic_any<>::length, std::size_t Align = basic_any<Len>::alignment, typename... Args>
[[nodiscard]] basic_any<Len, Align> make_any(Args &&...args) {
    return basic_any<Len, Align>{std::in_place_type<Type>, std::forward<Args>(args)...};
}


template<std::size_t Len = basic_any<>::length, std::size_t Align = basic_any<Len>::alignment, typename Type>
[[nodiscard]] basic_any<Len, Align> forward_as_any(Type &&value) {
    return basic_any<Len, Align>{std::in_place_type<Type &&>, std::forward<Type>(value)};
}

} 

#endif
