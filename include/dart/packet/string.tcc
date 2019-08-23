#ifndef DART_PACKET_STRING_H
#define DART_PACKET_STRING_H

/*----- Project Includes -----*/

#include "../common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount> basic_packet<RefCount>::make_string(shim::string_view val) {
    return basic_heap<RefCount>::make_string(val);
  }

  template <template <class> class RefCount>
  char const* basic_packet<RefCount>::str() const {
    return strv().data();
  }

  template <template <class> class RefCount>
  char const* basic_packet<RefCount>::str_or(char const* opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_packet::is_str, &basic_packet::str);
  }

  template <template <class> class RefCount>
  shim::string_view basic_packet<RefCount>::strv() const {
    return shim::visit([] (auto& v) { return v.strv(); }, impl);
  }

  template <template <class> class RefCount>
  shim::string_view basic_packet<RefCount>::strv_or(shim::string_view opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_packet::is_str, &basic_packet::strv);
  }

}

#endif
