#ifndef DART_OPERATORS_H
#define DART_OPERATORS_H

/*----- Local Includes -----*/

#include "common.h"
#include "conversion_traits.h"

/*----- Function Definitions -----*/

namespace dart {

  namespace detail {
    template <class Lhs, class Rhs>
    struct dart_comparison_constraints :
      meta::conjunction<
        meta::disjunction<
          detail::is_dart_api_type<std::decay_t<Lhs>>,
          detail::is_dart_api_type<std::decay_t<Rhs>>
        >,
        convert::are_comparable<Lhs, Rhs>
      >
    {};
  }

  template <class Lhs, class Rhs, class =
    std::enable_if_t<
      detail::dart_comparison_constraints<Lhs, Rhs>::value
    >
  >
  bool operator ==(Lhs const& lhs, Rhs const& rhs) noexcept(convert::are_nothrow_comparable<Lhs, Rhs>::value) {
    return convert::compare(lhs, rhs);
  }

  template <class Lhs, class Rhs, class =
    std::enable_if_t<
      detail::dart_comparison_constraints<Lhs, Rhs>::value
    >
  >
  bool operator !=(Lhs const& lhs, Rhs const& rhs) noexcept(convert::are_nothrow_comparable<Lhs, Rhs>::value) {
    return !(lhs == rhs);
  }

  template <class String, class Str, class =
    std::enable_if_t<
      std::is_convertible<
        std::decay_t<Str>,
        shim::string_view
      >::value
    >
  >
  basic_string<String> operator +(basic_string<String> const& lhs, Str const& rhs) {
    shim::string_view str = rhs;
    return basic_string<String> {lhs, str};
  }

  template <class String, class Str, class =
    std::enable_if_t<
      std::is_convertible<
        std::decay_t<Str>,
        shim::string_view
      >::value
    >
  >
  basic_string<String> operator +(Str const& lhs, basic_string<String> const& rhs) {
    shim::string_view str = rhs;
    return basic_string<String> {lhs, str};
  }

#define DART_DEFINE_ARITHMETIC_OPERATORS(op)                                              \
  template <class Number, class T, class =                                                \
    std::enable_if_t<                                                                     \
      std::is_integral<T>::value                                                          \
      ||                                                                                  \
      std::is_floating_point<T>::value                                                    \
    >                                                                                     \
  >                                                                                       \
  double operator op(basic_number<Number> const& lhs, T rhs) noexcept {                   \
    return lhs.numeric() op rhs;                                                          \
  }                                                                                       \
  template <class Number, class T, class =                                                \
    std::enable_if_t<                                                                     \
      std::is_integral<T>::value                                                          \
      ||                                                                                  \
      std::is_floating_point<T>::value                                                    \
    >                                                                                     \
  >                                                                                       \
  double operator op(T lhs, basic_number<Number> const& rhs) noexcept {                   \
    return rhs op lhs;                                                                    \
  }

  DART_DEFINE_ARITHMETIC_OPERATORS(+);
  DART_DEFINE_ARITHMETIC_OPERATORS(-);
  DART_DEFINE_ARITHMETIC_OPERATORS(*);
  DART_DEFINE_ARITHMETIC_OPERATORS(/);
#undef DART_DEFINE_ARITHMETIC_OPERATORS

#if DART_HAS_RAPIDJSON
  template <class Packet, class =
    std::enable_if_t<
      detail::is_dart_api_type<Packet>::value
    >
  >
  std::ostream& operator <<(std::ostream& out, Packet const& dart) {
    out << dart.to_json();
    return out;
  }
#endif

}

#endif
