#ifndef DART_META_H
#define DART_META_H

/*----- System Includes -----*/

#include <memory>
#include <gsl/span>
#include <type_traits>

/*----- Local Includes -----*/

#include "shim.h"

/*----- Type Declarations -----*/

namespace dart {

  namespace meta {

    template <class...>
    using void_t = void;

    template <class T>
    struct identity {
      using type = T;
    };

    // Removes one level of "pointer to const", "pointer to volatile"
    // "reference to const", "reference to volatile", etc,
    // to make working with strings easier.
    // Like std::decay except it doesn't decay C arrays,
    // but it otherwise much stronger.
    template <class T>
    struct canonical_type {
      using type = T;
    };
    template <class T>
    struct canonical_type<T const> {
      using type = T;
    };
    template <class T>
    struct canonical_type<T volatile> {
      using type = T;
    };
    template <class T>
    struct canonical_type<T const volatile> {
      using type = T;
    };
    template <class T>
    struct canonical_type<T&> {
      using type = T&;
    };
    template <class T>
    struct canonical_type<T const&> {
      using type = T&;
    };
    template <class T>
    struct canonical_type<T volatile&> {
      using type = T&;
    };
    template <class T>
    struct canonical_type<T const volatile&> {
      using type = T&;
    };
    template <class T>
    struct canonical_type<T*> {
      using type = T*;
    };
    template <class T>
    struct canonical_type<T const*> {
      using type = T*;
    };
    template <class T>
    struct canonical_type<T volatile*> {
      using type = T*;
    };
    template <class T>
    struct canonical_type<T const volatile*> {
      using type = T*;
    };
    template <class T, size_t len>
    struct canonical_type<T (&)[len]> {
      using type = T (&)[len];
    };
    template <class T, size_t len>
    struct canonical_type<T const (&)[len]> {
      using type = T (&)[len];
    };
    template <class T, size_t len>
    struct canonical_type<T volatile (&)[len]> {
      using type = T (&)[len];
    };
    template <class T, size_t len>
    struct canonical_type<T const volatile (&)[len]> {
      using type = T (&)[len];
    };
    template <class T, size_t len>
    struct canonical_type<T (*)[len]> {
      using type = T (*)[len];
    };
    template <class T, size_t len>
    struct canonical_type<T const (*)[len]> {
      using type = T (*)[len];
    };
    template <class T, size_t len>
    struct canonical_type<T volatile (*)[len]> {
      using type = T (*)[len];
    };
    template <class T, size_t len>
    struct canonical_type<T const volatile (*)[len]> {
      using type = T (*)[len];
    };
    template <class T>
    using canonical_type_t = typename canonical_type<T>::type;

    template <class T>
    struct is_ptr_to_const : std::false_type {};
    template <class T>
    struct is_ptr_to_const<T const*> : std::true_type {};

    // Create type traits to check for each type of comparison.
    template <class Lhs, class Rhs, class = void>
    struct are_comparable : std::false_type {};
    template <class Lhs, class Rhs>
    struct are_comparable<
      Lhs,
      Rhs,
      void_t<decltype(std::declval<Lhs>() == std::declval<Rhs>())>
    > : std::true_type {};

    template <class T, class = void>
    struct is_dereferenceable : std::false_type {};
    template <class T>
    struct is_dereferenceable<
      T,
      void_t<decltype(*std::declval<T>())>
    > : std::true_type {};

    template <class...>
    struct conjunction : std::true_type {};
    template <class B1>
    struct conjunction<B1> : B1 {};
    template <class B1, class... Bn>
    struct conjunction<B1, Bn...> :
      std::conditional_t<
        bool(B1::value),
        conjunction<Bn...>,
        B1
      >
    {};

    template <class...>
    struct disjunction : std::false_type {};
    template <class B1>
    struct disjunction<B1> : B1 {};
    template <class B1, class... Bn>
    struct disjunction<B1, Bn...> :
      std::conditional_t<
        bool(B1::value),
        B1,
        disjunction<Bn...>
      >
    {};

    template <class B>
    struct negation : std::integral_constant<bool, !bool(B::value)> {};

    template <class T, class... Ts>
    struct contained : disjunction<std::is_same<T, Ts>...> {};
    template <class T, class... Ts>
    struct not_contained : conjunction<negation<std::is_same<T, Ts>>...> {};

    template <class... Types>
    struct types {
      template <class T, class =
        std::enable_if_t<
          contained<T, Types...>::value
        >
      >
      types(T) {}
    };
    template <class... Types>
    struct not_types {
      template <class T, class =
        std::enable_if_t<
          not_contained<T, Types...>::value
        >
      >
      not_types(T) {}
    };

    template <class...>
    struct first_type {
      using type = void;
    };
    template <class T, class... Ts>
    struct first_type<T, Ts...> {
      using type = T;
    };
    template <class... Ts>
    using first_type_t = typename first_type<Ts...>::type;

    namespace detail {

      template <class Default, class AlwaysVoid, template <class...> class Op, class... Args>
      struct detector {
        using value_t = std::false_type;
        using type = Default;
      };
 
      template <class Default, template <class...> class Op, class... Args>
      struct detector<Default, void_t<Op<Args...>>, Op, Args...> {
        // Note that std::void_t is a C++17 feature
        using value_t = std::true_type;
        using type = Op<Args...>;
      };
 
    }
 
    struct nonesuch {
      nonesuch(nonesuch const&) = delete;
      nonesuch(nonesuch&&) = delete;
      ~nonesuch() = delete;
    };

    template <template <class...> class Op, class... Args>
    using is_detected = typename detail::detector<nonesuch, void, Op, Args...>::value_t;
    template <template <class...> class Op, class... Args>
    using detected_t = typename detail::detector<nonesuch, void, Op, Args...>::type;
    template <class Default, template <class...> class Op, class... Args>
    using detected_or = detail::detector<Default, void, Op, Args...>;

    template <class T>
    using strv_t = decltype(std::declval<T>().strv());
    template <class T>
    using integer_t = decltype(std::declval<T>().integer());
    template <class T>
    using decimal_t = decltype(std::declval<T>().decimal());
    template <class T>
    using boolean_t = decltype(std::declval<T>().boolean());
    template <class T>
    using get_type_t = decltype(std::declval<T>().get_type());

    template <class T>
    struct is_dartlike :
      meta::conjunction<
        is_detected<strv_t, T>,
        is_detected<integer_t, T>,
        is_detected<decimal_t, T>,
        is_detected<boolean_t, T>,
        is_detected<get_type_t, T>
      >
    {};

    template <class T, template <class> class Template>
    struct is_specialization_of : std::false_type {};
    template <class... Args, template <class> class Template>
    struct is_specialization_of<Template<Args...>, Template> : std::true_type {};

    template <class T, template <template <class> class> class Compound>
    struct is_higher_specialization_of : std::false_type {};
    template <template <class> class Inner, template <template <class> class> class Outer>
    struct is_higher_specialization_of<Outer<Inner>, Outer> : std::true_type {};

    template <class T>
    struct is_span : std::false_type {};
    template <class T, std::ptrdiff_t extent>
    struct is_span<gsl::span<T, extent>> : std::true_type {};

    template <class T>
    struct is_std_smart_ptr : std::false_type {};
    template <class T, class Del>
    struct is_std_smart_ptr<std::unique_ptr<T, Del>> : std::true_type {};
    template <class T>
    struct is_std_smart_ptr<std::shared_ptr<T>> : std::true_type {};
    template <class T>
    struct is_std_smart_ptr<std::weak_ptr<T>> : std::true_type {};

    template <size_t pos>
    struct priority_tag : priority_tag<pos - 1> {};
    template <>
    struct priority_tag<0> {};

    namespace detail {
      template <class T>
      struct is_builtin_string_impl : std::false_type {};
      template <>
      struct is_builtin_string_impl<char*> : std::true_type {};
      template <>
      struct is_builtin_string_impl<wchar_t*> : std::true_type {};
      template <>
      struct is_builtin_string_impl<char16_t*> : std::true_type {};
      template <>
      struct is_builtin_string_impl<char32_t*> : std::true_type {};
      template <size_t len>
      struct is_builtin_string_impl<char (&)[len]> : std::true_type {};
      template <size_t len>
      struct is_builtin_string_impl<wchar_t (&)[len]> : std::true_type {};
      template <size_t len>
      struct is_builtin_string_impl<char16_t (&)[len]> : std::true_type {};
      template <size_t len>
      struct is_builtin_string_impl<char32_t (&)[len]> : std::true_type {};
    }

    template <class T>
    struct is_builtin_string : detail::is_builtin_string_impl<canonical_type_t<T>> {};

    template <class T>
    struct is_std_string : std::false_type {};
    template <class CharT, class Traits, class Allocator>
    struct is_std_string<std::basic_string<CharT, Traits, Allocator>> : std::true_type {};

    template <class T>
    struct is_std_string_view : std::false_type {};
    template <class CharT, class Traits>
    struct is_std_string_view<shim::basic_string_view<CharT, Traits>> : std::true_type {};

    template <class T>
    struct is_string :
      meta::disjunction<
        is_builtin_string<T>,
        is_std_string<T>,
        is_std_string_view<T>
      >
    {};

  }

}

#undef DART_COMPARE_HELPER

#endif
