#pragma once

#include "ix.hpp"
#include "ix_memory.hpp"
#include "ix_type_traits.hpp"
#include "ix_utility.hpp"

template <size_t, typename>
struct ix_FunctionN;

template <size_t N, typename ReturnType, typename... Args>
struct ix_FunctionN<N, ReturnType(Args...)>
{
    ReturnType (*m_invoker)(const ix_FunctionN *, Args...) = nullptr;
    alignas(8) uint8_t m_buffer[N];

    constexpr ix_FunctionN() = default;

    template <typename Lambda
#if 0
              // If you have a function that is overloaded over multiple `ix_FunctionN`s,
              // you need the following line.
              , typename = ix_enable_if_t<ix_is_same_v<decltype(declval<Lambda>()(declval<Args>()...)), ReturnType>>
#endif
              >
    constexpr ix_FunctionN(Lambda &&f) // cppcheck-suppress noExplicitConstructor; NOLINT
        : m_invoker(+[](const ix_FunctionN *function, Args... args) -> ReturnType {
              const Lambda *f_restored = reinterpret_cast<const Lambda *>(function->m_buffer);
              return f_restored->operator()(ix_forward<Args>(args)...);
          })
    {
        static_assert(sizeof(Lambda) <= N);
        static_assert((8 % alignof(Lambda)) == 0);
        // static_assert(ix_is_lambda_v<F>); // Not available...
        static_assert(ix_is_trivially_copy_constructible_v<Lambda>);
        static_assert(ix_is_trivially_destructible_v<Lambda>);
        ix_memcpy(m_buffer, &f, sizeof(Lambda));
    }

    constexpr ix_FunctionN(ReturnType (*f)(Args...)) // cppcheck-suppress noExplicitConstructor; NOLINT
        : ix_FunctionN([f](Args... args) -> ReturnType { return f(args...); })
    {
    }

    constexpr ReturnType operator()(Args... args) const
    {
        return m_invoker(this, ix_forward<Args>(args)...);
    }

    constexpr void mark_dead()
    {
        m_invoker = nullptr;
    }

    constexpr bool is_dead() const
    {
        return (m_invoker == nullptr);
    }
};
