#ifndef DART_ARRAY_H
#define DART_ARRAY_H

/*----- Project Includes -----*/

#include "common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <class Array>
  template <class Arr, class EnableIf>
  basic_array<Array>::basic_array(Arr&& arr) : val(std::forward<Arr>(arr)) {
    if (!val.is_array()) {
      throw type_error("dart::packet::array can only be constructed as an array.");
    }
  }

  template <class Array>
  template <class ValueType, class EnableIf>
  basic_array<Array>& basic_array<Array>::push_front(ValueType&& value) & {
    val.push_front(std::forward<ValueType>(value));
    return *this;
  }

  template <class Array>
  template <class ValueType, class EnableIf>
  basic_array<Array>&& basic_array<Array>::push_front(ValueType&& value) && {
    auto&& tmp = std::move(val).push_front(std::forward<ValueType>(value));
    (void) tmp;
    return std::move(*this);
  }

  template <class Array>
  template <class Arr, class EnableIf>
  basic_array<Array>& basic_array<Array>::pop_front() & {
    val.pop_front();
    return *this;
  }

  template <class Array>
  template <class Arr, class EnableIf>
  basic_array<Array>&& basic_array<Array>::pop_front() && {
    auto&& tmp = std::move(val).pop_front();
    (void) tmp;
    return std::move(*this);
  }

  template <class Array>
  template <class ValueType, class EnableIf>
  basic_array<Array>& basic_array<Array>::push_back(ValueType&& value) & {
    val.push_back(std::forward<ValueType>(value));
    return *this;
  }

  template <class Array>
  template <class ValueType, class EnableIf>
  basic_array<Array>&& basic_array<Array>::push_back(ValueType&& value) && {
    auto&& tmp = std::move(val).push_back(std::forward<ValueType>(value));
    (void) tmp;
    return std::move(*this);
  }

  template <class Array>
  template <class Arr, class EnableIf>
  basic_array<Array>& basic_array<Array>::pop_back() & {
    val.pop_back();
    return *this;
  }

  template <class Array>
  template <class Arr, class EnableIf>
  basic_array<Array>&& basic_array<Array>::pop_back() && {
    auto&& tmp = std::move(val).pop_back();
    (void) tmp;
    return std::move(*this);
  }

  template <class Array>
  template <class Index, class ValueType, class EnableIf>
  auto basic_array<Array>::insert(Index&& idx, ValueType&& value) -> iterator {
    return val.insert(std::forward<Index>(idx), std::forward<ValueType>(value));
  }

  template <class Array>
  template <class Index, class ValueType, class EnableIf>
  auto basic_array<Array>::set(Index&& idx, ValueType&& value) -> iterator {
    return val.set(std::forward<Index>(idx), std::forward<ValueType>(value));
  }

  template <class Array>
  template <class Index, class EnableIf>
  auto basic_array<Array>::erase(Index const& idx) -> iterator {
    return val.erase(idx);
  }

  template <class Array>
  template <class Arr, class EnableIf>
  void basic_array<Array>::clear() {
    val.clear();
  }

  template <class Array>
  template <class Arr, class EnableIf>
  void basic_array<Array>::reserve(size_type count) {
    val.reserve(count);
  }

  template <class Array>
  template <class T, class EnableIf>
  void basic_array<Array>::resize(size_type count, T const& def) {
    val.resize(count, def);
  }

  template <class Array>
  template <class Index, class EnableIf>
  auto basic_array<Array>::operator [](Index const& idx) const -> value_type {
    return val[idx];
  }

  template <class Array>
  template <class Index, class EnableIf>
  auto basic_array<Array>::get(Index const& idx) const -> value_type {
    return val.get(idx);
  }

  template <class Array>
  template <class Index, class T, class EnableIf>
  auto basic_array<Array>::get_or(Index const& idx, T&& opt) const -> value_type {
    return val.get_or(idx, std::forward<T>(opt));
  }

  template <class Array>
  template <class Index, class EnableIf>
  auto basic_array<Array>::at(Index const& idx) const -> value_type {
    return val.at(idx);
  }

  template <class Array>
  auto basic_array<Array>::at_front() const -> value_type {
    return val.at_front();
  }

  template <class Array>
  auto basic_array<Array>::at_back() const -> value_type {
    return val.at_back();
  }

  template <class Array>
  auto basic_array<Array>::front() const -> value_type {
    return val.front();
  }

  template <class Array>
  template <class T, class EnableIf>
  auto basic_array<Array>::front_or(T&& opt) const -> value_type {
    return val.front_or(std::forward<T>(opt));
  }

  template <class Array>
  auto basic_array<Array>::back() const -> value_type {
    return val.back();
  }

  template <class Array>
  template <class T, class EnableIf>
  auto basic_array<Array>::back_or(T&& opt) const -> value_type {
    return val.back_or(std::forward<T>(opt));
  }

  template <class Array>
  auto basic_array<Array>::capacity() const -> size_type {
    return val.capacity();
  }

  namespace detail {

#ifdef DART_USE_SAJSON
    template <template <class> class RefCount>
    array<RefCount>::array(sajson::value vals) noexcept :
      elems(static_cast<uint32_t>(vals.get_length()))
    {
      // Iterate over our elements and write each one into the buffer.
      array_entry* entry = vtable();
      size_t offset = reinterpret_cast<gsl::byte*>(&vtable()[elems]) - DART_FROM_THIS_MUT;
      for (auto idx = 0U; idx < elems; ++idx) {
        // Identify the internal type of the current element.
        auto curr_val = vals.get_array_element(idx);
        auto val_type = json_identify<RefCount>(curr_val);

        // Using the current offset, align a pointer for the next element type.
        auto* unaligned = DART_FROM_THIS_MUT + offset;
        auto* aligned = detail::align_pointer<RefCount>(unaligned, val_type);
        offset += aligned - unaligned;

        // Add an entry to the vtable.
        new(entry++) array_entry(val_type, static_cast<uint32_t>(offset));

        // Recurse.
        offset += json_lower<RefCount>(aligned, curr_val);
      }

      // array is laid out, write in our final size.
      bytes = static_cast<uint32_t>(offset);
    }
#endif

#if DART_HAS_RAPIDJSON
    template <template <class> class RefCount>
    array<RefCount>::array(rapidjson::Value const& vals) noexcept :
      elems(static_cast<uint32_t>(vals.Size()))
    {
      // Iterate over our elements and write each one into the buffer.
      array_entry* entry = vtable();
      size_t offset = reinterpret_cast<gsl::byte*>(&vtable()[elems]) - DART_FROM_THIS_MUT;
      for (auto it = vals.Begin(); it != vals.End(); ++it) {
        // Identify the internal type of the current element.
        auto& curr_val = *it;
        auto val_type = json_identify<RefCount>(curr_val);

        // Using the current offset, align a pointer for the next element type.
        auto* unaligned = DART_FROM_THIS_MUT + offset;
        auto* aligned = detail::align_pointer<RefCount>(unaligned, val_type);
        offset += aligned - unaligned;

        // Add an entry to the vtable.
        new(entry++) array_entry(val_type, static_cast<uint32_t>(offset));

        // Recurse.
        offset += json_lower<RefCount>(aligned, curr_val);
      }

      // array is laid out, write in our final size.
      bytes = static_cast<uint32_t>(offset);
    }
#endif

    // FIXME: Audit this function. A LOT has changed since it was written.
    template <template <class> class RefCount>
    array<RefCount>::array(packet_elements<RefCount> const* vals) noexcept :
      elems(static_cast<uint32_t>(vals->size()))
    {
      // Iterate over our elements and write each one into the buffer.
      array_entry* entry = vtable();
      size_t offset = reinterpret_cast<gsl::byte*>(&vtable()[elems]) - DART_FROM_THIS_MUT;
      for (auto const& elem : *vals) {
        // Using the current offset, align a pointer for the next element type.
        auto* unaligned = DART_FROM_THIS_MUT + offset;
        auto* aligned = detail::align_pointer<RefCount>(unaligned, elem.get_raw_type());
        offset += aligned - unaligned;

        // Add an entry to the vtable.
        new(entry++) array_entry(elem.get_raw_type(), static_cast<uint32_t>(offset));

        // Recurse.
        offset += elem.layout(aligned);
      }

      // array is laid out, write in our final size.
      bytes = static_cast<uint32_t>(offset);
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

    template <template <class> class RefCount>
    template <bool silent>
    bool array<RefCount>::is_valid(size_t bytes) const noexcept(silent) {
      // Check if we even have enough space left for the array header.
      if (bytes < header_len) {
        if (silent) return false;
        else throw validation_error("Serialized array is truncated");
      }

      // We now know it's safe to access the array length, but it could still be garbage,
      // so check if the array claims to be larger than our total buffer.
      // After this check, all other length checks will use the length reported by the array
      // itself to validate internal consistency
      // Signed comparison warnings are the actual worst
      auto total_size = static_cast<std::ptrdiff_t>(get_sizeof());
      if (total_size > static_cast<ssize_t>(bytes)) {
        if (silent) return false;
        else throw validation_error("Serialized array length is out of bounds");
      }

      // The array reports a reasonable length,
      // so now check if the vtable is within bounds.
      auto* vtable_end = raw_vtable() + (size() * sizeof(array_entry));
      if (vtable_end - DART_FROM_THIS > total_size) {
        if (silent) return false;
        else throw validation_error("Serialized array vtable length is out of bounds");
      }

      // We now know that the vtable is fully within bounds, but it could still be full of crap
      // Check that every element in the vtable has a valid type
      for (size_t i = 0; i < size(); ++i) {
        if (!valid_type(vtable()[i].get_type())) {
          if (silent) return false;
          else throw validation_error("Serialized object value is of no known type");
        }
      }

      // We now know the entire vtable is within bounds,
      // so iterate over it and check all contained children.
      void const* prev = this;
      for (auto raw_val : *this) {
        // Load the base address of the value and verify that it's within bounds.
        auto val_offset = raw_val.buffer - DART_FROM_THIS;
        if (val_offset > total_size) {
          if (silent) return false;
          else throw validation_error("Serialized array value offset is out of bounds");
        } else if (raw_val.buffer <= prev) {
          if (silent) return false;
          else throw validation_error("Serialized array value contained a negative or cyclic offset");
        } else if (align_pointer<RefCount>(raw_val.buffer, raw_val.type) != raw_val.buffer) {
          if (silent) return false;
          else throw validation_error("Serialized array value offset does not meet alignment requirements");
        }
        prev = raw_val.buffer;

        // We now know that at least up to the base of the value is within bounds, so recurse on the value.
        // If the buffer validation routine returns false, it means we're not throwing errors.
        auto valid_val = valid_buffer<silent, RefCount>(raw_val, total_size - val_offset);
        if (!valid_val) return false;
      }
      return true;
    }

#if DART_USING_GCC
#pragma GCC diagnostic pop
#elif DART_USING_MSVC
#pragma warning(pop)
#endif

    template <template <class> class RefCount>
    size_t array<RefCount>::size() const noexcept {
      return elems;
    }

    template <template <class> class RefCount>
    size_t array<RefCount>::get_sizeof() const noexcept {
      return bytes;
    }

    template <template <class> class RefCount>
    auto array<RefCount>::begin() const noexcept -> ll_iterator<RefCount> {
      return detail::ll_iterator<RefCount>(0, DART_FROM_THIS, load_elem);
    }

    template <template <class> class RefCount>
    auto array<RefCount>::end() const noexcept -> ll_iterator<RefCount> {
      return detail::ll_iterator<RefCount>(size(), DART_FROM_THIS, load_elem);
    }

    template <template <class> class RefCount>
    auto array<RefCount>::get_elem(size_t index) const noexcept -> raw_element {
      return get_elem_impl(index, false);
    }

    template <template <class> class RefCount>
    auto array<RefCount>::at_elem(size_t index) const -> raw_element {
      return get_elem_impl(index, true);
    }

    template <template <class> class RefCount>
    auto array<RefCount>::load_elem(gsl::byte const* base, size_t idx) noexcept
      -> typename ll_iterator<RefCount>::value_type
    {
      return get_array<RefCount>({raw_type::array, base})->get_elem(idx);
    }

    template <template <class> class RefCount>
    auto array<RefCount>::get_elem_impl(size_t index, bool throw_if_absent) const -> raw_element {
      // Grab the value, or null, if the index is out of range.
      if (index < size()) {
        auto const& meta = vtable()[index];
        return {meta.get_type(), DART_FROM_THIS + meta.get_offset()};
      } else if (!throw_if_absent) {
        return {raw_type::null, nullptr};
      } else {
        throw std::out_of_range("dart::buffer does not contain requested index");
      }
    }

    template <template <class> class RefCount>
    array_entry* array<RefCount>::vtable() noexcept {
      auto* base = DART_FROM_THIS_MUT + sizeof(bytes) + sizeof(elems);
      return shim::launder(reinterpret_cast<array_entry*>(base));
    }

    template <template <class> class RefCount>
    array_entry const* array<RefCount>::vtable() const noexcept {
      auto* base = DART_FROM_THIS + sizeof(bytes) + sizeof(elems);
      return shim::launder(reinterpret_cast<array_entry const*>(base));
    }

    template <template <class> class RefCount>
    gsl::byte* array<RefCount>::raw_vtable() noexcept {
      return DART_FROM_THIS_MUT + sizeof(bytes) + sizeof(elems);
    }

    template <template <class> class RefCount>
    gsl::byte const* array<RefCount>::raw_vtable() const noexcept {
      return DART_FROM_THIS + sizeof(bytes) + sizeof(elems);
    }

  }

}

#endif
