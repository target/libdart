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
