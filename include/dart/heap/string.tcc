#ifndef DART_HEAP_STRING_H
#define DART_HEAP_STRING_H

/*----- Project Includes -----*/

#include "../common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <template <class> class RefCount>
  template <bool enabled, class EnableIf>
  basic_heap<RefCount> basic_heap<RefCount>::make_string(shim::string_view base, shim::string_view app) {
    return basic_heap(detail::string_tag {}, base, app);
  }

  template <template <class> class RefCount>
  char const* basic_heap<RefCount>::str() const {
    return strv().data();
  }

  template <template <class> class RefCount>
  char const* basic_heap<RefCount>::str_or(char const* opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_heap::is_str, &basic_heap::str);
  }

  template <template <class> class RefCount>
  shim::string_view basic_heap<RefCount>::strv() const {
    auto* sso = shim::get_if<detail::inline_string_layout>(&data);
    if (sso) {
      return {sso->buffer.data(), sso_bytes - sso->left};
    } else {
      auto* str = shim::get_if<detail::dynamic_string_layout>(&data);
      if (str) return {str->ptr.get(), str->len};
      else throw type_error("dart::heap has no string value");
    }
  }

  template <template <class> class RefCount>
  shim::string_view basic_heap<RefCount>::strv_or(shim::string_view opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_heap::is_str, &basic_heap::strv);
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>::basic_heap(detail::string_tag, shim::string_view base, shim::string_view app) {
    auto len = base.size() + app.size();
    auto type = detail::identify_string<RefCount>(base, app);
    switch (type) {
      case detail::raw_type::string:
      case detail::raw_type::big_string:
        {
          // String is too large for SSO.
          // Allocate some space and copy the string data in.
          auto ptr = std::make_unique<char[]>(len + 1);
          std::copy(base.begin(), base.end(), ptr.get());
          std::copy(app.begin(), app.end(), ptr.get() + base.size());
          ptr[len] = '\0';

          // Move the data in.
          // FIXME: Turns out there's no portable way to use std::make_shared
          // to allocate an array of characters in C++14.
          auto shared = std::shared_ptr<char> {ptr.release(), +[] (char* ptr) { delete[] ptr; }};
          data = detail::dynamic_string_layout {std::move(shared), len};
          break;
        }
      default:
        {
          DART_ASSERT(type == detail::raw_type::small_string);

          // String is small enough for SSO, copy the string contents into the in-situ buffer.
          detail::inline_string_layout layout;
          std::copy(base.begin(), base.end(), layout.buffer.begin());
          std::copy(app.begin(), app.end(), layout.buffer.begin() + base.size());

          // Terminate the string and set the number of remaining bytes.
          // These two operations may touch the same byte.
          // XXX: Have to access through std::array::data member as writing the null terminator
          // is intentionally allowed to access 1 byte out of range if the string is the max
          // SSO length, and MSVC asserts on out of bounds accesses in std::array.
          layout.buffer.data()[len] = '\0';
          layout.left = static_cast<uint8_t>(sso_bytes) - static_cast<uint8_t>(len);
          data = layout;
        }
    }
  }

}

#endif
