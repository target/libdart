#ifndef DART_BUFFER_PRIMITIVE_H
#define DART_BUFFER_PRIMITIVE_H

/*----- Project Includes -----*/

#include "../common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <template <class> class RefCount>
  int64_t basic_buffer<RefCount>::integer() const {
    return detail::integer_deref([] (auto& val) { return val.get_data(); }, raw);
  }

  template <template <class> class RefCount>
  double basic_buffer<RefCount>::decimal() const {
    return detail::decimal_deref([] (auto& val) { return val.get_data(); }, raw);
  }

  template <template <class> class RefCount>
  double basic_buffer<RefCount>::numeric() const {
    switch (get_type()) {
      case type::integer:
        return static_cast<double>(integer());
      case type::decimal:
        return decimal();
      default:
        throw type_error("dart::buffer has no numeric value");
    }
  }

  template <template <class> class RefCount>
  bool basic_buffer<RefCount>::boolean() const {
    return detail::get_primitive<bool>(raw)->get_data();
  }

}

#endif
