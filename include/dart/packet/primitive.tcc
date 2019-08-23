#ifndef DART_PACKET_PRIMITIVE_H
#define DART_PACKET_PRIMITIVE_H

/*----- Project Includes -----*/

#include "../common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount> basic_packet<RefCount>::make_integer(int64_t val) noexcept {
    return basic_heap<RefCount>::make_integer(val);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount> basic_packet<RefCount>::make_decimal(double val) noexcept {
    return basic_heap<RefCount>::make_decimal(val);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount> basic_packet<RefCount>::make_boolean(bool val) noexcept {
    return basic_heap<RefCount>::make_boolean(val);
  }

  template <template <class> class RefCount>
  int64_t basic_packet<RefCount>::integer() const {
    return shim::visit([] (auto& v) { return v.integer(); }, impl);
  }

  template <template <class> class RefCount>
  int64_t basic_packet<RefCount>::integer_or(int64_t opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_packet::is_integer, &basic_packet::integer);
  }

  template <template <class> class RefCount>
  double basic_packet<RefCount>::decimal() const {
    return shim::visit([] (auto& v) { return v.decimal(); }, impl);
  }

  template <template <class> class RefCount>
  double basic_packet<RefCount>::decimal_or(double opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_packet::is_decimal, &basic_packet::decimal);
  }

  template <template <class> class RefCount>
  double basic_packet<RefCount>::numeric() const {
    return shim::visit([] (auto& v) { return v.numeric(); }, impl);
  }

  template <template <class> class RefCount>
  double basic_packet<RefCount>::numeric_or(double opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_packet::is_numeric, &basic_packet::numeric);
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::boolean() const {
    return shim::visit([] (auto& v) { return v.boolean(); }, impl);
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::boolean_or(bool opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_packet::is_boolean, &basic_packet::boolean);
  }

}

#endif
