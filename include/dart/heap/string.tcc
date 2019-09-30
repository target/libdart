#ifndef DART_HEAP_STRING_H
#define DART_HEAP_STRING_H

/*----- Project Includes -----*/

#include "../common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <template <class> class RefCount>
  template <bool enabled, class EnableIf>
  basic_heap<RefCount> basic_heap<RefCount>::make_string(shim::string_view val) {
    return basic_heap(detail::string_tag {}, val);
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
    auto* sso = shim::get_if<inline_string_layout>(&data);
    if (sso) {
      return {sso->buffer.data(), sso_bytes - sso->left};
    } else {
      auto* str = shim::get_if<dynamic_string_layout>(&data);
      if (str) return {str->ptr.get(), str->len};
      else throw type_error("dart::heap has no string value");
    }
  }

  template <template <class> class RefCount>
  shim::string_view basic_heap<RefCount>::strv_or(shim::string_view opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_heap::is_str, &basic_heap::strv);
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>::basic_heap(detail::string_tag, shim::string_view val) {
    auto type = detail::identify_string<RefCount>(val);
    switch (type) {
      case detail::raw_type::string:
      case detail::raw_type::big_string:
        {
          // String is too large for SSO.
          // Allocate some space and copy the string data in.
          auto ptr = std::make_unique<char[]>(val.size() + 1);
          std::copy(val.begin(), val.end(), ptr.get());
          ptr[val.size()] = '\0';

          // Move the data in.
          // FIXME: Turns out there's no portable way to use std::make_shared
          // to allocate an array of characters in C++14.
          auto shared = std::shared_ptr<char> {ptr.release(), +[] (char* ptr) { delete[] ptr; }};
          data = dynamic_string_layout {std::move(shared), val.size()};
          break;
        }
      default:
        {
          DART_ASSERT(type == detail::raw_type::small_string);

          // String is small enough for SSO, copy the string contents into the in-situ buffer.
          inline_string_layout layout;
          std::copy(val.begin(), val.end(), layout.buffer.begin());

          // Terminate the string and set the number of remaining bytes.
          // These two operations may touch the same byte.
          // XXX: Have to access through std::array::data member as writing the null terminator
          // is intentionally allowed to access 1 byte out of range if the string is the max
          // SSO length, and MSVC asserts on out of bounds accesses in std::array.
          layout.buffer.data()[val.size()] = '\0';
          layout.left = static_cast<uint8_t>(sso_bytes) - static_cast<uint8_t>(val.size());
          data = layout;
        }
    }
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::dynamic_string_layout::operator ==(dynamic_string_layout const& other) const noexcept {
    if (len != other.len) return false;
    else return !strcmp(ptr.get(), other.ptr.get());
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::dynamic_string_layout::operator !=(dynamic_string_layout const& other) const noexcept {
    return !(*this == other);
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::inline_string_layout::operator ==(inline_string_layout const& other) const noexcept {
    if (left != other.left) return false;
    else return !strcmp(buffer.data(), other.buffer.data());
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::inline_string_layout::operator !=(inline_string_layout const& other) const noexcept {
    return !(*this == other);
  }

}

#endif
