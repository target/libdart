#ifndef DART_STR_IMPL_H
#define DART_STR_IMPL_H

/*----- Local Includes -----*/

#include "dart_intern.h"

/*----- Function Implementations -----*/

namespace dart {

  template <class String>
  template <class Str, class>
  basic_string<String>::basic_string(Str&& val) {
    if (!val.is_str()) throw type_error("dart::packet::string can only be constructed from a string");
    this->val = std::forward<Str>(val);
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::make_string(shim::string_view val) {
    return basic_heap(detail::string_tag {}, val);
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::make_string(shim::string_view val) {
    return basic_packet(detail::string_tag {}, val);
  }

  template <class String>
  char const* basic_string<String>::str() const noexcept {
    return val.str();
  }

  template <template <class> class RefCount>
  char const* basic_heap<RefCount>::str() const {
    return strv().data();
  }

  template <template <class> class RefCount>
  char const* basic_buffer<RefCount>::str() const {
    return strv().data();
  }

  template <template <class> class RefCount>
  char const* basic_packet<RefCount>::str() const {
    return strv().data();
  }

  template <template <class> class RefCount>
  char const* basic_heap<RefCount>::str_or(char const* opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_heap::is_str, &basic_heap::str);
  }

  template <template <class> class RefCount>
  char const* basic_packet<RefCount>::str_or(char const* opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_packet::is_str, &basic_packet::str);
  }

  template <class String>
  shim::string_view basic_string<String>::strv() const noexcept {
    return val.strv();
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
  shim::string_view basic_buffer<RefCount>::strv() const {
    return detail::string_deref([] (auto& val) { return val.get_strv(); }, raw);
  }

  template <template <class> class RefCount>
  shim::string_view basic_packet<RefCount>::strv() const {
    return shim::visit([] (auto& v) { return v.strv(); }, impl);
  }

  template <template <class> class RefCount>
  shim::string_view basic_heap<RefCount>::strv_or(shim::string_view opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_heap::is_str, &basic_heap::strv);
  }

  template <template <class> class RefCount>
  shim::string_view basic_packet<RefCount>::strv_or(shim::string_view opt) const noexcept {
    return detail::safe_optional_access(*this, opt, &basic_packet::is_str, &basic_packet::strv);
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
          layout.buffer[val.size()] = '\0';
          layout.left = sso_bytes - val.size();
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

  namespace detail {

    template <class SizeType>
    basic_string<SizeType>::basic_string(shim::string_view strv) noexcept : len(strv.size()) {
      // Copy the string into the buffer.
      // Can't use constructor delegation here because our destructor is deleted.
      std::copy_n(strv.data(), len, data());
      data()[len] = '\0';
    }

    template <class SizeType>
    basic_string<SizeType>::basic_string(char const* str, size_t len) noexcept : len(len) {
      // Copy the string into the buffer.
      std::copy_n(str, len, data());
      data()[len] = '\0';
    }

    template <class SizeType>
    size_t basic_string<SizeType>::size() const noexcept {
      return len;
    }

    template <class SizeType>
    size_t basic_string<SizeType>::get_sizeof() const noexcept {
      return static_sizeof(size());
    }

    template <class SizeType>
    shim::string_view basic_string<SizeType>::get_strv() const noexcept {
      return {data(), size()};
    }

    template <class SizeType>
    size_t basic_string<SizeType>::static_sizeof(size_type len) noexcept {
      return sizeof(size_type) + len + 1;
    }

    template <class SizeType>
    char* basic_string<SizeType>::data() noexcept {
      return shim::launder(reinterpret_cast<char*>(this) + sizeof(size_type));
    }

    template <class SizeType>
    char const* basic_string<SizeType>::data() const noexcept {
      return shim::launder(reinterpret_cast<char const*>(this) + sizeof(size_type));
    }

  }

}

#endif
