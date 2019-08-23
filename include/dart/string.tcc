#ifndef DART_STRING_H
#define DART_STRING_H

/*----- Local Includes -----*/

#include "common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <class String>
  template <class Str, class>
  basic_string<String>::basic_string(Str&& val) {
    if (!val.is_str()) throw type_error("dart::packet::string can only be constructed from a string");
    this->val = std::forward<Str>(val);
  }

  template <class String>
  char const* basic_string<String>::str() const noexcept {
    return val.str();
  }

  template <class String>
  shim::string_view basic_string<String>::strv() const noexcept {
    return val.strv();
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
