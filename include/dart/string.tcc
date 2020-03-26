#ifndef DART_STRING_H
#define DART_STRING_H

/*----- Local Includes -----*/

#include "common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <class String>
  template <class Str, class EnableIf>
  basic_string<String>::basic_string(Str&& val) {
    if (!val.is_str()) throw type_error("dart::packet::string can only be constructed from a string");
    this->val = std::forward<Str>(val);
  }

  template <class String>
  template <class Str, class EnableIf>
  basic_string<String>& basic_string<String>::operator +=(shim::string_view str) {
    // Concat and re-assign
    val = String::make_string(strv(),  str);
    return *this;
  }

  template <class String>
  char basic_string<String>::operator [](size_type idx) const noexcept {
    if (idx >= size()) return '\0';
    else return str()[idx];
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
    basic_string<SizeType>::basic_string(char const* str, size_t len) noexcept :
      len(static_cast<size_type>(len))
    {
      // Copy the string into the buffer.
      std::copy_n(str, len, data());
      data()[len] = '\0';
    }

// Unfortunately some versions of GCC and MSVC aren't smart enough to figure
// out that if this function is declared noexcept the throwing cases are dead code
#if DART_USING_GCC
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpragmas"
#pragma GCC diagnostic ignored "-Wterminate"
#elif DART_USING_MSVC
#pragma warning(push)
#pragma warning(disable: 4297)
#endif

    template <class SizeType>
    template <bool silent>
    bool basic_string<SizeType>::is_valid(size_t bytes) const noexcept(silent) {
      // Check if we even have enough space left for the string header
      if (bytes < header_len) {
        if (silent) return false;
        else throw validation_error("Serialized string is truncated");
      }

      // We now know it's safe to access the string length, but it could still be garbage,
      // so check if the string claims to be larger than the total buffer.
      auto total_size = get_sizeof();
      if (total_size > bytes) {
        if (silent) return false;
        else throw validation_error("Serialized string length is out of bounds");
      }

      // We now know that the total string is within bounds, but it could still be garbage,
      // so, since we can't generically calculate if the string contents are "correct",
      // use the existence of the null terminator as a proxy for lack of corruption.
      if (data()[size()] != '\0') {
        if (silent) return false;
        else throw validation_error("Serialized string is corrupted, internal consistency checks failed");
      }
      return true;
    }

#if DART_USING_GCC
#pragma GCC diagnostic pop
#elif DART_USING_MSVC
#pragma warning(pop)
#endif

    template <class SizeType>
    size_t basic_string<SizeType>::size() const noexcept {
      return len;
    }

    template <class SizeType>
    size_t basic_string<SizeType>::get_sizeof() const noexcept {
      return static_sizeof(static_cast<size_type>(size()));
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
