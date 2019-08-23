#ifndef DART_ARRAY_H
#define DART_ARRAY_H

/*----- Project Includes -----*/

#include "common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <class Array>
  template <class Arr, class>
  basic_array<Array>::basic_array(Arr&& arr) : val(std::forward<Arr>(arr)) {
    if (!val.is_array()) {
      throw type_error("dart::packet::array can only be constructed as an array.");
    }
  }

  template <class Array>
  template <class ValueType, class>
  basic_array<Array>& basic_array<Array>::push_front(ValueType&& value) & {
    val.push_front(std::forward<ValueType>(value));
    return *this;
  }

  template <class Array>
  template <class ValueType, class>
  basic_array<Array>&& basic_array<Array>::push_front(ValueType&& value) && {
    auto&& tmp = std::move(val).push_front(std::forward<ValueType>(value));
    (void) tmp;
    return std::move(*this);
  }

  template <class Array>
  template <class, class>
  basic_array<Array>& basic_array<Array>::pop_front() & {
    val.pop_front();
    return *this;
  }

  template <class Array>
  template <class, class>
  basic_array<Array>&& basic_array<Array>::pop_front() && {
    auto&& tmp = std::move(val).pop_front();
    (void) tmp;
    return std::move(*this);
  }

  template <class Array>
  template <class ValueType, class>
  basic_array<Array>& basic_array<Array>::push_back(ValueType&& value) & {
    val.push_back(std::forward<ValueType>(value));
    return *this;
  }

  template <class Array>
  template <class ValueType, class>
  basic_array<Array>&& basic_array<Array>::push_back(ValueType&& value) && {
    auto&& tmp = std::move(val).push_back(std::forward<ValueType>(value));
    (void) tmp;
    return std::move(*this);
  }

  template <class Array>
  template <class, class>
  basic_array<Array>& basic_array<Array>::pop_back() & {
    val.pop_back();
    return *this;
  }

  template <class Array>
  template <class, class>
  basic_array<Array>&& basic_array<Array>::pop_back() && {
    auto&& tmp = std::move(val).pop_back();
    (void) tmp;
    return std::move(*this);
  }

  template <class Array>
  template <class Index, class ValueType, class>
  auto basic_array<Array>::insert(Index&& idx, ValueType&& value) -> iterator {
    return val.insert(std::forward<Index>(idx), std::forward<ValueType>(value));
  }

  template <class Array>
  template <class Index, class ValueType, class>
  auto basic_array<Array>::set(Index&& idx, ValueType&& value) -> iterator {
    return val.set(std::forward<Index>(idx), std::forward<ValueType>(value));
  }

  template <class Array>
  template <class Index, class>
  auto basic_array<Array>::erase(Index const& idx) -> iterator {
    return val.erase(idx);
  }

  template <class Array>
  template <class, class>
  void basic_array<Array>::clear() {
    val.clear();
  }

  template <class Array>
  template <class, class>
  void basic_array<Array>::reserve(size_type count) {
    val.reserve(count);
  }

  template <class Array>
  template <class T, class>
  void basic_array<Array>::resize(size_type count, T const& def) {
    val.resize(count, def);
  }

  template <class Array>
  template <class Index, class>
  auto basic_array<Array>::operator [](Index const& idx) const& -> value_type {
    return val[idx];
  }

  template <class Array>
  template <class Index, class>
  decltype(auto) basic_array<Array>::operator [](Index const& idx) && {
    return std::move(val)[idx];
  }

  template <class Array>
  template <class Index, class>
  auto basic_array<Array>::get(Index const& idx) const& -> value_type {
    return val.get(idx);
  }

  template <class Array>
  template <class Index, class>
  decltype(auto) basic_array<Array>::get(Index const& idx) && {
    return std::move(val).get(idx);
  }

  template <class Array>
  template <class KeyType, class T, class>
  auto basic_array<Array>::get_or(KeyType const& idx, T&& opt) const -> value_type {
    return val.get_or(idx, std::forward<T>(opt));
  }

  template <class Array>
  template <class Index, class>
  auto basic_array<Array>::at(Index const& idx) const& -> value_type {
    return val.at(idx);
  }

  template <class Array>
  template <class Index, class>
  decltype(auto) basic_array<Array>::at(Index const& idx) && {
    return std::move(val).at(idx);
  }

  template <class Array>
  auto basic_array<Array>::at_front() const& -> value_type {
    return val.at_front();
  }

  template <class Array>
  decltype(auto) basic_array<Array>::at_front() && {
    return std::move(val).at_front();
  }

  template <class Array>
  auto basic_array<Array>::at_back() const& -> value_type {
    return val.at_back();
  }

  template <class Array>
  decltype(auto) basic_array<Array>::at_back() && {
    return std::move(val).at_back();
  }

  template <class Array>
  auto basic_array<Array>::front() const& -> value_type {
    return val.front();
  }

  template <class Array>
  decltype(auto) basic_array<Array>::front() && {
    return std::move(val).front();
  }

  template <class Array>
  template <class T, class>
  auto basic_array<Array>::front_or(T&& opt) const -> value_type {
    return val.front_or(std::forward<T>(opt));
  }

  template <class Array>
  auto basic_array<Array>::back() const& -> value_type {
    return val.back();
  }

  template <class Array>
  decltype(auto) basic_array<Array>::back() && {
    return std::move(val).back();
  }

  template <class Array>
  template <class T, class>
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
    array<RefCount>::array(sajson::value vals) noexcept : elems(vals.get_length()) {
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
        new(entry++) array_entry(val_type, offset);

        // Recurse.
        offset += json_lower<RefCount>(aligned, curr_val);
      }

      // array is laid out, write in our final size.
      bytes = offset;
    }
#endif

#if DART_HAS_RAPIDJSON
    template <template <class> class RefCount>
    array<RefCount>::array(rapidjson::Value const& vals) noexcept : elems(vals.Size()) {
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
        new(entry++) array_entry(val_type, offset);

        // Recurse.
        offset += json_lower<RefCount>(aligned, curr_val);
      }

      // array is laid out, write in our final size.
      bytes = offset;
    }
#endif

    // FIXME: Audit this function. A LOT has changed since it was written.
    template <template <class> class RefCount>
    array<RefCount>::array(packet_elements<RefCount> const* vals) noexcept : elems(vals->size()) {
      // Iterate over our elements and write each one into the buffer.
      array_entry* entry = vtable();
      size_t offset = reinterpret_cast<gsl::byte*>(&vtable()[elems]) - DART_FROM_THIS_MUT;
      for (auto const& elem : *vals) {
        // Using the current offset, align a pointer for the next element type.
        auto* unaligned = DART_FROM_THIS_MUT + offset;
        auto* aligned = detail::align_pointer<RefCount>(unaligned, elem.get_raw_type());
        offset += aligned - unaligned;

        // Add an entry to the vtable.
        new(entry++) array_entry(elem.get_raw_type(), offset);

        // Recurse.
        offset += elem.layout(aligned);
      }

      // array is laid out, write in our final size.
      bytes = offset;
    }

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

  }

}

#endif
