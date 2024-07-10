#pragma once

#include "ix.hpp"

// Based on Microsoft's STL (https://github.com/microsoft/STL, Apache License v2.0 with LLVM Exception).

ix_DISABLE_MSVC_WARNING_BEGIN
ix_DISABLE_MSVC_WARNING(4180) // Don't put const/volatile to function types.

template <typename T>
T &&declval();

template <class...>
using ix_void_t = void;

template <bool b>
struct ix_bool_constant
{
    static constexpr bool value = b;
};

using ix_true_type = ix_bool_constant<true>;
using ix_false_type = ix_bool_constant<false>;

using ix_nullptr_t = decltype(nullptr);

template <bool Test, class T = void>
struct ix_enable_if
{
};

template <class T>
struct ix_enable_if<true, T>
{
    using type = T;
};

template <bool Test, class T = void>
using ix_enable_if_t = typename ix_enable_if<Test, T>::type;

#if ix_COMPILER(CLANG)

template <class T1, class T2>
constexpr bool ix_is_same_v = __is_same(T1, T2);

#else

template <class, class>
constexpr bool ix_is_same_v = false;

template <class T>
constexpr bool ix_is_same_v<T, T> = true;

#endif

template <class T, class = void>
struct ix_add_reference
{
    using lvalue = T;
    using rvalue = T;
};

template <class T>
struct ix_add_reference<T, ix_void_t<T &>>
{
    using lvalue = T &;
    using rvalue = T &&;
};

template <class T>
using ix_add_lvalue_reference_t = typename ix_add_reference<T>::lvalue;

template <class T>
using ix_add_rvalue_reference_t = typename ix_add_reference<T>::rvalue;

template <class T>
struct ix_remove_reference
{
    using type = T;
};

template <class T>
struct ix_remove_reference<T &>
{
    using type = T;
};

template <class T>
struct ix_remove_reference<T &&>
{
    using type = T;
};

template <class T>
using ix_remove_reference_t = typename ix_remove_reference<T>::type;

template <class T>
struct ix_remove_cv
{
    using type = T;
};

template <class T>
struct ix_remove_cv<const T>
{
    using type = T;
};

template <class T>
struct ix_remove_cv<volatile T>
{
    using type = T;
};

template <class T>
struct ix_remove_cv<const volatile T>
{
    using type = T;
};

template <class T>
using ix_remove_cv_t = typename ix_remove_cv<T>::type;

template <class T>
struct ix_remove_all_extents
{
    using type = T;
};

template <class T, size_t n>
struct ix_remove_all_extents<T[n]>
{
    using type = typename ix_remove_all_extents<T>::type;
};

template <class T>
struct ix_remove_all_extents<T[]>
{
    using type = typename ix_remove_all_extents<T>::type;
};

template <class T>
using ix_remove_all_extents_t = typename ix_remove_all_extents<T>::type;

template <typename T>
constexpr bool ix_is_void_v = ix_is_same_v<ix_remove_cv_t<T>, void>;

template <typename T>
constexpr bool ix_is_unbounded_array_v = false;

template <typename T>
constexpr bool ix_is_unbounded_array_v<T[]> = true;

template <class T>
constexpr bool ix_is_integral_v_impl =   //
    ix_is_same_v<T, bool> ||             //
    ix_is_same_v<T, char> ||             //
    ix_is_same_v<T, signed char> ||      //
    ix_is_same_v<T, unsigned char> ||    //
    ix_is_same_v<T, wchar_t> ||          //
#ifdef __cpp_char8_t                     //
    ix_is_same_v<T, char8_t> ||          //
#endif                                   //
    ix_is_same_v<T, char16_t> ||         //
    ix_is_same_v<T, char32_t> ||         //
    ix_is_same_v<T, short> ||            //
    ix_is_same_v<T, unsigned short> ||   //
    ix_is_same_v<T, int> ||              //
    ix_is_same_v<T, unsigned int> ||     //
    ix_is_same_v<T, long> ||             //
    ix_is_same_v<T, unsigned long> ||    //
    ix_is_same_v<T, long long> ||        //
    ix_is_same_v<T, unsigned long long>; //

template <class T>
constexpr bool ix_is_integral_v = ix_is_integral_v_impl<ix_remove_cv_t<T>>;

template <class T>
constexpr bool ix_is_floating_point_v_impl = //
    ix_is_same_v<T, float> ||                //
    ix_is_same_v<T, double> ||               //
    ix_is_same_v<T, long double>;            //

template <class T>
constexpr bool ix_is_floating_point_v = ix_is_floating_point_v_impl<ix_remove_cv_t<T>>;

template <class T>
constexpr bool ix_is_enum_v = __is_enum(T);

template <class T>
constexpr bool ix_is_arithmetic_v = ix_is_integral_v<T> || ix_is_floating_point_v<T>;

template <class>
constexpr bool ix_is_pointer_v = false;

template <class T>
constexpr bool ix_is_pointer_v<T *> = true;

template <class T>
constexpr bool ix_is_pointer_v<T *const> = true;

template <class T>
constexpr bool ix_is_pointer_v<T *volatile> = true;

template <class T>
constexpr bool ix_is_pointer_v<T *const volatile> = true;

template <class>
constexpr bool ix_is_const_v = false;

template <class T>
constexpr bool ix_is_const_v<const T> = true;

template <class>
constexpr bool ix_is_reference_v = false;

template <class T>
constexpr bool ix_is_reference_v<T &> = true;

template <class T>
constexpr bool ix_is_reference_v<T &&> = true;

template <class T>
constexpr bool ix_is_function_v = !ix_is_const_v<const T> && !ix_is_reference_v<T>;

template <class T>
constexpr bool ix_is_null_pointer_v = ix_is_same_v<ix_remove_cv_t<T>, ix_nullptr_t>;

#if ix_COMPILER(CLANG)

template <class T>
constexpr bool ix_is_member_pointer_v = __is_member_pointer(T);

#else

template <class>
struct ix_is_member_object_pointer_impl
{
    static constexpr bool value = false;
};

template <class T1, class T2>
struct ix_is_member_object_pointer_impl<T1 T2::*>
{
    static constexpr bool value = !ix_is_function_v<T1>;
};

template <class T>
constexpr bool ix_is_member_object_pointer_v = ix_is_member_object_pointer_impl<ix_remove_cv_t<T>>::value;

template <class T>
struct ix_is_member_function_pointer_impl
{
    static constexpr bool value = false;
};

template <class T1, class T2>
struct ix_is_member_function_pointer_impl<T1 T2::*>
{
    static constexpr bool value = ix_is_function_v<T1>;
};

template <typename T>
constexpr bool ix_is_member_function_pointer_v = ix_is_member_function_pointer_impl<ix_remove_cv_t<T>>::value;

template <class T>
constexpr bool ix_is_member_pointer_v = ix_is_member_object_pointer_v<T> || ix_is_member_function_pointer_v<T>;

#endif

template <class T>
constexpr bool ix_is_scalar_v =  //
    ix_is_arithmetic_v<T> ||     //
    ix_is_enum_v<T> ||           //
    ix_is_pointer_v<T> ||        //
    ix_is_member_pointer_v<T> || //
    ix_is_null_pointer_v<T>;     //

template <class>
constexpr bool ix_is_lvalue_reference_v = false;

template <class T>
constexpr bool ix_is_lvalue_reference_v<T &> = true;

template <class>
constexpr bool ix_is_rvalue_reference_v = false;

template <class T>
constexpr bool ix_is_rvalue_reference_v<T &&> = true;

template <class T>
constexpr bool ix_is_default_constructible_v = __is_constructible(T);

template <class T>
constexpr bool ix_is_copy_constructible_v = __is_constructible(T, ix_add_lvalue_reference_t<const T>);

template <class T>
constexpr bool ix_is_move_constructible_v = __is_constructible(T, T);

template <class T>
constexpr bool ix_is_trivially_constructible_v = __is_trivially_constructible(T);

template <class T>
constexpr bool ix_is_trivially_copy_constructible_v =
    __is_trivially_constructible(T, ix_add_rvalue_reference_t<const T>);

template <class T>
constexpr bool ix_is_trivially_move_constructible_v = __is_trivially_constructible(T, T);

template <class T>
constexpr bool ix_is_copy_assignable_v =
    __is_assignable(ix_add_lvalue_reference_t<T>, ix_add_lvalue_reference_t<const T>);

template <class T>
constexpr bool ix_is_move_assignable_v = __is_assignable(ix_add_lvalue_reference_t<T>, T);

template <class T>
constexpr bool ix_is_trivially_copy_assignable_v =
    __is_trivially_assignable(ix_add_lvalue_reference_t<T>, ix_add_lvalue_reference_t<const T>);

template <class T>
constexpr bool ix_is_trivially_move_assignable_v = __is_trivially_assignable(ix_add_lvalue_reference_t<T>, T);

#if ix_PLATFORM(WIN) && (ix_COMPILER(CLANG) || ix_COMPILER(MSVC))

ix_DISABLE_CLANG_WARNING_BEGIN
ix_DISABLE_CLANG_WARNING("-Wlanguage-extension-token")

template <class T>
constexpr bool ix_is_destructible_v = __is_destructible(T);

ix_DISABLE_CLANG_WARNING_END

#else

template <typename T, typename = decltype(declval<T &>().~T())>
constexpr ix_true_type ix_is_destructible_v_impl_helper(int);

template <typename>
constexpr ix_false_type ix_is_destructible_v_impl_helper(...);

template <typename T>
constexpr bool ix_is_destructible_v_impl = decltype(ix_is_destructible_v_impl_helper<T>(0))::value;

template <typename T>
constexpr bool ix_is_destructible_v = !(ix_is_void_v<T> || ix_is_unbounded_array_v<T> || ix_is_function_v<T>)&&(
    ix_is_reference_v<T> || ix_is_scalar_v<T> || ix_is_destructible_v_impl<ix_remove_all_extents_t<T>>);

#endif

#if ix_COMPILER(CLANG) || ix_COMPILER(MSVC)

template <class T>
constexpr bool ix_is_trivially_destructible_v = __is_trivially_destructible(T);

#else

template <class T>
constexpr bool ix_is_trivially_destructible_v = ix_is_destructible_v<T> && __has_trivial_destructor(T);

#endif

template <bool Cond, class T, class F>
struct ix_conditional
{
    using type = T;
};

template <class T, class F>
struct ix_conditional<false, T, F>
{
    using type = F;
};

template <bool Cond, class T, class F>
using ix_conditional_t = typename ix_conditional<Cond, T, F>::type;

template <class T, class = void>
struct ix_add_pointer_helper
{
    using type = T;
};

template <class T>
struct ix_add_pointer_helper<T, ix_void_t<ix_remove_reference_t<T> *>>
{
    using type = ix_remove_reference_t<T> *;
};

template <class T>
struct ix_add_pointer
{
    using type = typename ix_add_pointer_helper<T>::type;
};

template <class T>
using ix_add_pointer_t = typename ix_add_pointer_helper<T>::type;

template <class T>
constexpr bool ix_is_class_v = __is_class(T);

template <class T>
constexpr bool ix_is_union_v = __is_union(T);

ix_DISABLE_MSVC_WARNING_END
