#ifndef DART_META_H
#define DART_META_H

/*----- System Includes -----*/

#include <memory>
#include <type_traits>

#define DART_COMPARE_HELPER(name, op)                                     \
  template <class Lhs, class Rhs, class = void>                           \
  struct name : std::false_type {};                                       \
  template <class Lhs, class Rhs>                                         \
  struct name<                                                            \
    Lhs,                                                                  \
    Rhs,                                                                  \
    void_t<decltype(std::declval<Lhs>() op std::declval<Rhs>())>          \
  > : std::true_type {};

/*----- Type Declarations -----*/

namespace dart {

  namespace meta {

    template <class...>
    using void_t = void;

    template <class T>
    struct identity {
      using type = T;
    };

    // Create type traits to check for each type of comparison.
    DART_COMPARE_HELPER(are_comparable, ==);
    DART_COMPARE_HELPER(are_negated_comparable, !=);
    DART_COMPARE_HELPER(are_lt_comparable, <);
    DART_COMPARE_HELPER(are_lte_comparable, <=);
    DART_COMPARE_HELPER(are_gt_comparable, >);
    DART_COMPARE_HELPER(are_gte_comparable, >=);

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
    struct is_dartlike {
      static constexpr auto value =
        is_detected<strv_t, T>::value && is_detected<integer_t, T>::value
        && is_detected<decimal_t, T>::value && is_detected<boolean_t, T>::value && is_detected<get_type_t, T>::value;
    };

    template <class T, template <class...> class Template>
    struct is_specialization_of : std::false_type {};
    template <class... Args, template <class...> class Template>
    struct is_specialization_of<Template<Args...>, Template> : std::true_type {};

    template <class T, template <template <class...> class> class Compound>
    struct is_higher_specialization_of : std::false_type {};
    template <template <class...> class Inner, template <template <class...> class> class Outer>
    struct is_higher_specialization_of<Outer<Inner>, Outer> : std::true_type {};

    template <size_t pos>
    struct priority_tag : priority_tag<pos - 1> {};
    template <>
    struct priority_tag<0> {};

  }

}

#undef DART_COMPARE_HELPER

#endif
