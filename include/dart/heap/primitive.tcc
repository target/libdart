#ifndef DART_HEAP_PRIMITIVE_H
#define DART_HEAP_PRIMITIVE_H

/*----- Project Includes -----*/

#include "../common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <template <class> class RefCount>
  template <bool enabled, class EnableIf>
  basic_heap<RefCount> basic_heap<RefCount>::make_integer(int64_t val) noexcept {
    return basic_heap(detail::integer_tag {}, val);
  }

  template <template <class> class RefCount>
  template <bool enabled, class EnableIf>
  basic_heap<RefCount> basic_heap<RefCount>::make_decimal(double val) noexcept {
    return basic_heap(detail::decimal_tag {}, val);
  }

  template <template <class> class RefCount>
  template <bool enabled, class EnableIf>
  basic_heap<RefCount> basic_heap<RefCount>::make_boolean(bool val) noexcept {
    return basic_heap(detail::boolean_tag {}, val);
  }

  template <template <class> class RefCount>
  int64_t basic_heap<RefCount>::integer() const {
    if (is_integer()) return shim::get<int64_t>(data);
    else throw type_error("dart::heap has no integer value");
  }

  template <template <class> class RefCount>
  int64_t basic_heap<RefCount>::integer_or(int64_t opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_heap::is_integer, &basic_heap::integer);
  }

  template <template <class> class RefCount>
  double basic_heap<RefCount>::decimal() const {
    if (is_decimal()) return shim::get<double>(data);
    else throw type_error("dart::heap has no decimal value");
  }

  template <template <class> class RefCount>
  double basic_heap<RefCount>::decimal_or(double opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_heap::is_decimal, &basic_heap::decimal);
  }

  template <template <class> class RefCount>
  double basic_heap<RefCount>::numeric() const {
    switch (get_type()) {
      case type::integer:
        return static_cast<double>(integer());
      case type::decimal:
        return decimal();
      default:
        throw type_error("dart::heap has no numeric value");
    }
  }

  template <template <class> class RefCount>
  double basic_heap<RefCount>::numeric_or(double opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_heap::is_numeric, &basic_heap::numeric);
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::boolean() const {
    if (is_boolean()) return shim::get<bool>(data);
    else throw type_error("dart::heap has no boolean value");
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::boolean_or(bool opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_heap::is_boolean, &basic_heap::boolean);
  }

}

#endif
