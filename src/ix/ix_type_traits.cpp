#include "ix_type_traits.hpp"
#include "ix_DumbString.hpp"
#include "ix_doctest.hpp"
#include "ix_initializer_list.hpp"
#include "ix_utility.hpp"

#include <locale>
#include <memory>
#include <vector>

// Based on examples in https://cpprefjp.github.io/ (CC-BY 3.0)

// NOLINTBEGIN(cppcoreguidelines-special-member-functions,hicpp-special-member-functions,hicpp-use-equals-default)
// NOLINTBEGIN(cert-oop54-cpp, modernize-use-equals-default)

template <class To, class From>
decltype(static_cast<To>(declval<From>())) convert(From &&x)
{
    return static_cast<To>(ix_forward<From>(x));
}

ix_TEST_CASE("ix: declval")
{
    int x = 3;
    const double d = convert<double>(x);
    ix_EXPECT(d == doctest::Approx(3.0));
}

template <class, template <class> class, class = ix_void_t<>>
struct detect : ix_false_type
{
};

template <class T, template <class> class Check>
struct detect<T, Check, ix_void_t<Check<T>>> : ix_true_type
{
};

template <class T>
using has_iterator_type_impl = typename T::iterator;

template <class T>
using has_iterator_type = detect<T, has_iterator_type_impl>;

template <class T>
using is_equality_comparable_impl = decltype(declval<const T &>() == declval<const T &>());

template <class T>
using is_equality_comparable = detect<T, is_equality_comparable_impl>;

struct NonComparable
{
};

ix_TEST_CASE("ix_void_t")
{
    static_assert(!has_iterator_type<int>::value);
    static_assert(has_iterator_type<std::vector<int>>::value);
    static_assert(is_equality_comparable<int>::value);
    static_assert(!is_equality_comparable<NonComparable>::value);
}

ix_TEST_CASE("ix_bool_constant")
{
    static_assert(ix_bool_constant<true>::value);
    static_assert(!ix_bool_constant<false>::value);
    static_assert(ix_true_type::value);
    static_assert(!ix_false_type::value);
}

ix_TEST_CASE("ix: nullptr_t")
{
    static_assert(sizeof(ix_nullptr_t) == sizeof(nullptr));
    static_assert(sizeof(ix_nullptr_t) == sizeof(void *));
    // static_assert(is_object_v<ix_nullptr_t>);
    // static_assert(is_scalar_v<ix_nullptr_t>);
    // static_assert(!is_union_v<ix_nullptr_t>);
    // static_assert(!is_array_v<ix_nullptr_t>);
    // static_assert(!is_class_v<ix_nullptr_t>);
}

ix_TEST_CASE("ix_is_same_v")
{
    struct my_type
    {
    };

    using my_int = int;

    static_assert(ix_is_same_v<int, int>);
    static_assert(ix_is_same_v<int[], int[]>);
    static_assert(ix_is_same_v<int[2], int[2]>);
    static_assert(ix_is_same_v<bool, bool>);
    static_assert(!ix_is_same_v<int, const int>);
    static_assert(!ix_is_same_v<int, volatile int>);
    static_assert(!ix_is_same_v<int, int *>);
    static_assert(!ix_is_same_v<int, my_type>);
    static_assert(ix_is_same_v<int, my_int>);
}

ix_TEST_CASE("ix_add_lvalue_reference_t")
{
    static_assert(ix_is_same_v<ix_add_lvalue_reference_t<int>, int &>);
    static_assert(ix_is_same_v<ix_add_lvalue_reference_t<int &>, int &>);
    static_assert(ix_is_same_v<ix_add_lvalue_reference_t<int &&>, int &>);
    static_assert(ix_is_same_v<ix_add_lvalue_reference_t<const int>, const int &>);
    static_assert(ix_is_same_v<ix_add_lvalue_reference_t<const int &>, const int &>);
    static_assert(ix_is_same_v<ix_add_lvalue_reference_t<const int &&>, const int &>);
}

ix_TEST_CASE("ix_add_rvalue_reference_t")
{
    static_assert(ix_is_same_v<ix_add_rvalue_reference_t<int>, int &&>);
    static_assert(ix_is_same_v<ix_add_rvalue_reference_t<int &>, int &>);
    static_assert(ix_is_same_v<ix_add_rvalue_reference_t<int &&>, int &&>);
    static_assert(ix_is_same_v<ix_add_rvalue_reference_t<const int>, const int &&>);
    static_assert(ix_is_same_v<ix_add_rvalue_reference_t<const int &>, const int &>);
    static_assert(ix_is_same_v<ix_add_rvalue_reference_t<const int &&>, const int &&>);
}

ix_TEST_CASE("ix_remove_reference_t")
{
    static_assert(ix_is_same_v<ix_remove_reference_t<int>, int>);
    static_assert(ix_is_same_v<ix_remove_reference_t<int &>, int>);
    static_assert(ix_is_same_v<ix_remove_reference_t<int &&>, int>);
    static_assert(ix_is_same_v<ix_remove_reference_t<const int>, const int>);
    static_assert(ix_is_same_v<ix_remove_reference_t<const int &>, const int>);
    static_assert(ix_is_same_v<ix_remove_reference_t<const int &&>, const int>);
}

ix_TEST_CASE("ix_remove_cv_v")
{
    static_assert(ix_is_same_v<ix_remove_cv_t<const int>, int>);
    static_assert(ix_is_same_v<ix_remove_cv_t<const volatile int>, int>);
    static_assert(ix_is_same_v<ix_remove_cv_t<const int *>, const int *>);
    static_assert(ix_is_same_v<ix_remove_cv_t<const int &>, const int &>);
    static_assert(ix_is_same_v<ix_remove_cv_t<int *const>, int *>);
}

ix_TEST_CASE("ix_remove_all_extents_v")
{
    static_assert(ix_is_same_v<ix_remove_all_extents_t<int>, int>);
    static_assert(ix_is_same_v<ix_remove_all_extents_t<const int[2]>, const int>);
    static_assert(ix_is_same_v<ix_remove_all_extents_t<int[2][4]>, int>);
    static_assert(ix_is_same_v<ix_remove_all_extents_t<int[][2]>, int>);
}

ix_TEST_CASE("ix_is_void_v")
{
    static_assert(ix_is_void_v<void>);
    static_assert(!ix_is_void_v<int>);

    static_assert(ix_is_void_v<const void>);
    static_assert(ix_is_void_v<volatile void>);
    static_assert(ix_is_void_v<const volatile void>);

    static_assert(!ix_is_void_v<void *>);
    static_assert(!ix_is_void_v<void()>);
}

ix_TEST_CASE("ix_is_unbounded_array_v")
{
    static_assert(ix_is_unbounded_array_v<int[]>);
    static_assert(!ix_is_unbounded_array_v<int[3]>);
    static_assert(!ix_is_unbounded_array_v<int *>);
    static_assert(ix_is_unbounded_array_v<const int[]>);
    static_assert(!ix_is_unbounded_array_v<int(&)[]>);
}

ix_TEST_CASE("ix_is_integral_v")
{
    enum my_enum
    {
    };

    static_assert(ix_is_integral_v<int>);
    static_assert(!ix_is_integral_v<int *>);
    static_assert(ix_is_integral_v<bool>);
    static_assert(ix_is_integral_v<char>);
    static_assert(ix_is_integral_v<char32_t>);
    static_assert(ix_is_integral_v<const long long>);
    static_assert(ix_is_integral_v<volatile unsigned>);
    static_assert(!ix_is_integral_v<my_enum>);
    static_assert(!ix_is_integral_v<int &>);
    static_assert(!ix_is_integral_v<int[1]>);
    static_assert(!ix_is_integral_v<int()>);
    static_assert(!ix_is_integral_v<float>);
}

ix_TEST_CASE("ix_is_floating_point_v")
{
    static_assert(ix_is_floating_point_v<float>);
    static_assert(!ix_is_floating_point_v<int>);
    static_assert(ix_is_floating_point_v<double>);
    static_assert(ix_is_floating_point_v<long double>);
    static_assert(ix_is_floating_point_v<const float>);
    static_assert(ix_is_floating_point_v<volatile double>);
    static_assert(!ix_is_floating_point_v<unsigned>);
    static_assert(!ix_is_floating_point_v<float *>);
    static_assert(!ix_is_floating_point_v<double &>);
    static_assert(!ix_is_floating_point_v<long double[1]>);
}

ix_TEST_CASE("ix_is_pointer_v")
{
    struct s
    {
    };

    static_assert(ix_is_pointer_v<int *>);
    static_assert(!ix_is_pointer_v<int>);
    static_assert(ix_is_pointer_v<unsigned *>);
    static_assert(ix_is_pointer_v<const long *>);
    static_assert(ix_is_pointer_v<const double *const>);
    static_assert(ix_is_pointer_v<void (*)()>);
    static_assert(!ix_is_pointer_v<int[]>);
    static_assert(!ix_is_pointer_v<int *&>);
    static_assert(!ix_is_pointer_v<void()>);
    static_assert(!ix_is_pointer_v<void (s::*)()>);
    static_assert(!ix_is_pointer_v<int s::*>);
    static_assert(!ix_is_pointer_v<ix_nullptr_t>);
}

ix_TEST_CASE("ix_is_const_v")
{
    static_assert(ix_is_const_v<const int>);
    static_assert(!ix_is_const_v<int>);
    static_assert(ix_is_const_v<const volatile int>);
    static_assert(!ix_is_const_v<const int &>);
}

ix_TEST_CASE("ix_is_reference_v")
{
    static_assert(ix_is_reference_v<int &>);
    static_assert(!ix_is_reference_v<int>);
    static_assert(ix_is_reference_v<float &>);
    static_assert(ix_is_reference_v<unsigned &&>);
    static_assert(ix_is_reference_v<long &&>);
    static_assert(!ix_is_reference_v<int *>);
    static_assert(!ix_is_reference_v<void(int &)>);
    static_assert(!ix_is_reference_v < int && () >);
}

ix_TEST_CASE("ix_is_function_v")
{
    using f = void();

    static_assert(ix_is_function_v<f>);
    static_assert(!ix_is_function_v<int>);
    ix_DISABLE_CLANG_WARNING_BEGIN
    ix_DISABLE_CLANG_WARNING("-Wignored-qualifiers")
    ix_DISABLE_MSVC_WARNING_BEGIN
    ix_DISABLE_MSVC_WARNING(4180) // Don't put const/volatile to function types.
    static_assert(ix_is_function_v<const f>);
    static_assert(ix_is_function_v<volatile f>);
    static_assert(ix_is_function_v<const volatile f>);
    ix_DISABLE_MSVC_WARNING_END
    ix_DISABLE_CLANG_WARNING_END
    static_assert(!ix_is_function_v<f *>);
    static_assert(!ix_is_function_v<f &>);
    static_assert(!ix_is_function_v<f &&>);
}

ix_TEST_CASE("ix_is_null_pointer_v")
{
    static_assert(ix_is_null_pointer_v<ix_nullptr_t>);
    static_assert(ix_is_null_pointer_v<const ix_nullptr_t>);
    static_assert(!ix_is_null_pointer_v<int>);
}

ix_TEST_CASE("ix_is_member_pointer_v")
{
    struct s
    {
    };

    static_assert(ix_is_member_pointer_v<int s::*>);
    static_assert(!ix_is_member_pointer_v<int>);
    static_assert(ix_is_member_pointer_v<int s::*const>);
    static_assert(ix_is_member_pointer_v<const int s::*>);
    static_assert(ix_is_member_pointer_v<int * s::*>);
    static_assert(ix_is_member_pointer_v<void (s::*const)()>);
    static_assert(ix_is_member_pointer_v<void (s::*)() const>);
    static_assert(ix_is_member_pointer_v<const int *(s::*)()>);
}

ix_TEST_CASE("ix_is_scalar_v")
{
    static_assert(ix_is_scalar_v<int>);
    static_assert(!ix_is_scalar_v<void>);

    enum e
    {
    };
    class c
    {
    };
    union u
    {
    };

    static_assert(ix_is_scalar_v<e>);
    static_assert(!ix_is_scalar_v<c>);
    static_assert(!ix_is_scalar_v<u>);
    static_assert(ix_is_scalar_v<const volatile int>);
    static_assert(ix_is_scalar_v<int *>);
    static_assert(!ix_is_scalar_v<int[1]>);
    static_assert(!ix_is_scalar_v<int &>);
    static_assert(!ix_is_scalar_v<int()>);
    static_assert(ix_is_scalar_v<ix_nullptr_t>);

    struct s
    {
    };

    static_assert(ix_is_scalar_v<int s::*>);
    static_assert(ix_is_scalar_v<void (s::*)()>);
}

ix_TEST_CASE("ix_is_lvalue_reference_v")
{
    static_assert(ix_is_lvalue_reference_v<int &>);
    static_assert(!ix_is_lvalue_reference_v<int>);
    static_assert(ix_is_lvalue_reference_v<unsigned &>);
    static_assert(ix_is_lvalue_reference_v<const long &>);
    static_assert(ix_is_lvalue_reference_v<const double *&>);
    static_assert(ix_is_lvalue_reference_v<void (&)()>);
    static_assert(!ix_is_lvalue_reference_v<int *>);
    static_assert(!ix_is_lvalue_reference_v<int &&>);
    static_assert(!ix_is_lvalue_reference_v<void()>);
    static_assert(!ix_is_lvalue_reference_v<void(int &)>);
}

ix_TEST_CASE("ix_is_rvalue_reference_v")
{
    static_assert(ix_is_rvalue_reference_v<int &&>);
    static_assert(!ix_is_rvalue_reference_v<int>);
    static_assert(ix_is_rvalue_reference_v<unsigned &&>);
    static_assert(ix_is_rvalue_reference_v<const long &&>);
    static_assert(ix_is_rvalue_reference_v<const double *&&>);
#if !ix_CPPCHECK
    static_assert(ix_is_rvalue_reference_v<void (&&)()>);
#endif
    static_assert(!ix_is_rvalue_reference_v<int *>);
    static_assert(!ix_is_rvalue_reference_v<int &>);
    static_assert(!ix_is_rvalue_reference_v<void()>);
    static_assert(!ix_is_rvalue_reference_v<void(int &&)>);
}

ix_TEST_CASE("ix_is_default_constructible_v")
{
    struct s
    {
        s(int, int)
        {
        }
    };

    struct u
    {
    };

    ix_UNUSED(s(0, 0));
    static_assert(ix_is_default_constructible_v<int>);
    static_assert(!ix_is_default_constructible_v<s>);
    static_assert(ix_is_default_constructible_v<u>);
    static_assert(ix_is_default_constructible_v<double>);
    static_assert(ix_is_default_constructible_v<const int>);
    static_assert(ix_is_default_constructible_v<int[1]>);
    static_assert(ix_is_default_constructible_v<void *>);
    static_assert(!ix_is_default_constructible_v<int[]>);
    static_assert(!ix_is_default_constructible_v<void>);
    static_assert(!ix_is_default_constructible_v<int &>);
    static_assert(!ix_is_default_constructible_v<int &&>);
    static_assert(!ix_is_default_constructible_v<int()>);
}

ix_TEST_CASE("ix_is_copy_constructible_v")
{

    struct s
    {
        s(const s &) = delete;
    };

    struct u
    {
    };

    static_assert(ix_is_copy_constructible_v<int>);
    static_assert(!ix_is_copy_constructible_v<s>);
    static_assert(ix_is_copy_constructible_v<u>);
    static_assert(ix_is_copy_constructible_v<double>);
    static_assert(ix_is_copy_constructible_v<const int>);
    static_assert(ix_is_copy_constructible_v<void *>);
    static_assert(ix_is_copy_constructible_v<int &>);
    static_assert(!ix_is_copy_constructible_v<int[1]>);
    static_assert(!ix_is_copy_constructible_v<int[]>);
    static_assert(!ix_is_copy_constructible_v<void>);
    static_assert(!ix_is_copy_constructible_v<int &&>);
    static_assert(!ix_is_copy_constructible_v<int()>);
}

ix_TEST_CASE("ix_is_move_constructible_v")
{
    struct s
    {
        s(s &&) = delete;
    };

    struct u
    {
    };

    static_assert(ix_is_move_constructible_v<int>);
    static_assert(!ix_is_move_constructible_v<s>);
    static_assert(ix_is_move_constructible_v<u>);
    static_assert(ix_is_move_constructible_v<double>);
    static_assert(ix_is_move_constructible_v<const int>);
    static_assert(ix_is_move_constructible_v<void *>);
    static_assert(ix_is_move_constructible_v<int &>);
    static_assert(ix_is_move_constructible_v<int &&>);
    static_assert(!ix_is_move_constructible_v<int[1]>);
    static_assert(!ix_is_move_constructible_v<int[]>);
    static_assert(!ix_is_move_constructible_v<void>);
    static_assert(!ix_is_move_constructible_v<int()>);
}

ix_TEST_CASE("ix_is_trivially_constructible_v")
{
    struct X
    {
    };

    struct Y
    {
        Y(const Y &);
    };

    struct Z
    {
        ix_DumbString s = "test";
    };

    static_assert(ix_is_trivially_constructible_v<int>);
    static_assert(ix_is_trivially_constructible_v<X>);
    static_assert(!ix_is_trivially_constructible_v<Y>);
    static_assert(!ix_is_trivially_constructible_v<Z>);
}

ix_TEST_CASE("ix_is_trivially_copy_constructible_v")
{
    struct X
    {
    };

    struct Y
    {
        Y(const Y &);
    };

    struct Z
    {
        ix_DumbString s = "test";
    };

    static_assert(ix_is_trivially_copy_constructible_v<int>);
    static_assert(ix_is_trivially_copy_constructible_v<X>);
    static_assert(!ix_is_trivially_copy_constructible_v<Y>);
    static_assert(!ix_is_trivially_copy_constructible_v<Z>);
}

ix_TEST_CASE("ix_is_trivially_move_constructible_v")
{
    struct X
    {
    };

    struct Y
    {
        Y() = default;

        Y(Y &&) noexcept
        {
        }
    };

    Y y0;
    const Y y(ix_move(y0));
    ix_UNUSED(y);

    struct Z
    {
        ix_DumbString s = "test";
    };

    static_assert(ix_is_trivially_move_constructible_v<int>);
    static_assert(ix_is_trivially_move_constructible_v<X>);
    static_assert(!ix_is_trivially_move_constructible_v<Y>);
    static_assert(!ix_is_trivially_move_constructible_v<Z>);
}

ix_TEST_CASE("ix_is_copy_assignable_v")
{
    struct s
    {
        s &operator=(const s &) = delete;
    };

    static_assert(ix_is_copy_assignable_v<int>);
    static_assert(!ix_is_copy_assignable_v<s>);
    static_assert(ix_is_copy_assignable_v<int &>);
    static_assert(ix_is_copy_assignable_v<int &&>);
    static_assert(ix_is_copy_assignable_v<volatile int>);
    static_assert(ix_is_copy_assignable_v<unsigned>);
    static_assert(ix_is_copy_assignable_v<void *>);
    static_assert(!ix_is_copy_assignable_v<s &>);
    static_assert(!ix_is_copy_assignable_v<s &&>);
    static_assert(!ix_is_copy_assignable_v<const int>);
    static_assert(!ix_is_copy_assignable_v<int[1]>);
    static_assert(!ix_is_copy_assignable_v<int[]>);
    static_assert(!ix_is_copy_assignable_v<void>);
}

ix_TEST_CASE("ix_is_move_assignable_v")
{
    struct s
    {
        s &operator=(s &&) = delete;
    };

    static_assert(ix_is_move_assignable_v<int>);
    static_assert(!ix_is_move_assignable_v<s>);
    static_assert(ix_is_move_assignable_v<int &>);
    static_assert(ix_is_move_assignable_v<int &&>);
    static_assert(ix_is_move_assignable_v<volatile int>);
    static_assert(ix_is_move_assignable_v<unsigned>);
    static_assert(ix_is_move_assignable_v<std::unique_ptr<int>>);
    static_assert(!ix_is_move_assignable_v<s &>);
    static_assert(!ix_is_move_assignable_v<s &&>);
    static_assert(!ix_is_move_assignable_v<const int>);
    static_assert(!ix_is_move_assignable_v<int[1]>);
    static_assert(!ix_is_move_assignable_v<int[]>);
    static_assert(!ix_is_move_assignable_v<void>);
}

ix_TEST_CASE("ix_is_trivially_copy_assignable_v")
{
    struct X
    {
    };

    struct Y
    {
        Y &operator=(const Y &);
    };

    struct Z
    {
        ix_DumbString s = "test";
    };

    static_assert(ix_is_trivially_copy_assignable_v<int>);
    static_assert(ix_is_trivially_copy_assignable_v<X>);
    static_assert(!ix_is_trivially_copy_assignable_v<Y>);
    static_assert(!ix_is_trivially_copy_assignable_v<Z>);
}

ix_TEST_CASE("ix_is_trivially_move_assignable_v")
{

    struct X
    {
    };

    struct Y
    {
        Y() = default;

        Y &operator=(Y &&) noexcept
        {
            return *this;
        }
    };

    Y y;
    y = Y();
    ix_UNUSED(y);

    struct Z
    {
        ix_DumbString s = "test";
    };

    static_assert(ix_is_trivially_move_assignable_v<int>);
    static_assert(ix_is_trivially_move_assignable_v<X>);
    static_assert(!ix_is_trivially_move_assignable_v<Y>);
    static_assert(!ix_is_trivially_move_assignable_v<Z>);
}

ix_TEST_CASE("ix_is_destructible_v")
{
    struct S
    {
        ~S() = delete;
    };

    static_assert(ix_is_destructible_v<int>);
    static_assert(!ix_is_destructible_v<S>);

    static_assert(ix_is_destructible_v<const int>);
    static_assert(ix_is_destructible_v<int *>);
    static_assert(ix_is_destructible_v<long>);
    static_assert(ix_is_destructible_v<int[1]>);

    static_assert(!ix_is_destructible_v<int[]>);
    static_assert(!ix_is_destructible_v<void>);
    static_assert(!ix_is_destructible_v<std::locale::facet>);
    static_assert(!ix_is_destructible_v<std::locale::facet[]>);
    static_assert(!ix_is_destructible_v<std::locale::facet[1]>);
    static_assert(!ix_is_destructible_v<std::ctype<char>>);
    static_assert(!ix_is_destructible_v<std::ctype<char>[]>);
    static_assert(!ix_is_destructible_v<std::ctype<char>[1]>);
}

ix_TEST_CASE("ix_is_trivially_destructible_v")
{

    struct C1
    {
        int x = 10;
    };

    struct C2
    {
        ~C2()
        {
        }
    };

    const C2 c2;
    ix_UNUSED(c2);

    static_assert(ix_is_trivially_destructible_v<int>);
    static_assert(ix_is_trivially_destructible_v<int *>);
    static_assert(ix_is_trivially_destructible_v<C1>);
    static_assert(!ix_is_trivially_destructible_v<C2>);
}

ix_TEST_CASE("ix_add_pointer")
{
    static_assert(ix_is_same_v<ix_add_pointer<int>::type, int *>);
    static_assert(ix_is_same_v<ix_add_pointer<int &>::type, int *>);
    static_assert(ix_is_same_v<ix_add_pointer<int *>::type, int **>);
    static_assert(ix_is_same_v<ix_add_pointer<int *&>::type, int **>);
}

ix_TEST_CASE("ix_is_class_v")
{

    class c
    {
    };

    struct s
    {
    };

    enum e
    {
    };

    enum class ec
    {
    };

    union u
    {
    };

    static_assert(ix_is_class_v<c> == true);
    static_assert(ix_is_class_v<int> == false);

    static_assert(ix_is_class_v<const c> == true);
    static_assert(ix_is_class_v<volatile c> == true);
    static_assert(ix_is_class_v<const volatile c> == true);
    static_assert(ix_is_class_v<s> == true);

    static_assert(ix_is_class_v<u *> == false);
    static_assert(ix_is_class_v<e> == false);
    static_assert(ix_is_class_v<ec> == false);
    static_assert(ix_is_class_v<u> == false);
}

ix_TEST_CASE("ix_is_union_v")
{

    union u
    {
    };

    static_assert(ix_is_union_v<u> == true);
    static_assert(ix_is_union_v<int> == false);

    static_assert(ix_is_union_v<const u> == true);
    static_assert(ix_is_union_v<volatile u> == true);
    static_assert(ix_is_union_v<const volatile u> == true);

    static_assert(ix_is_union_v<u *> == false);
    static_assert(ix_is_union_v<u &> == false);
    static_assert(ix_is_union_v<u()> == false);
}

// NOLINTEND(cert-oop54-cpp, modernize-use-equals-default)
// NOLINTEND(cppcoreguidelines-special-member-functions,hicpp-special-member-functions,hicpp-use-equals-default)
