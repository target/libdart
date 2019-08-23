#ifndef DART_BUFFER_ARRAY_H
#define DART_BUFFER_ARRAY_H

/*----- Project Includes -----*/

#include "../common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <template <class> class RefCount>
  template <class Number>
  basic_buffer<RefCount> basic_buffer<RefCount>::operator [](basic_number<Number> const& idx) const& {
    return (*this)[idx.integer()];
  }

  template <template <class> class RefCount>
  template <class Number, template <class> class, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::operator [](basic_number<Number> const& idx) && {
    return std::move(*this)[idx.integer()];
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> basic_buffer<RefCount>::operator [](size_type index) const& {
    return get(index);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::operator [](size_type index) && {
    return std::move(*this).get(index);
  }

  template <template <class> class RefCount>
  template <class Number>
  basic_buffer<RefCount> basic_buffer<RefCount>::get(basic_number<Number> const& idx) const& {
    return get(idx.integer());
  }

  template <template <class> class RefCount>
  template <class Number, template <class> class, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::get(basic_number<Number> const& idx) && {
    return std::move(*this).get(idx.integer());
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> basic_buffer<RefCount>::get(size_type index) const& {
    return basic_buffer(detail::get_array<RefCount>(raw)->get_elem(index), buffer_ref);
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::get(size_type index) && {
    raw = detail::get_array<RefCount>(raw)->get_elem(index);
    if (is_null()) buffer_ref = nullptr;
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <class Number>
  basic_buffer<RefCount> basic_buffer<RefCount>::at(basic_number<Number> const& idx) const& {
    return at(idx.integer());
  }

  template <template <class> class RefCount>
  template <class Number, template <class> class, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::at(basic_number<Number> const& idx) && {
    return std::move(*this).at(idx.integer());
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> basic_buffer<RefCount>::at(size_type index) const& {
    return basic_buffer(detail::get_array<RefCount>(raw)->at_elem(index), buffer_ref);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::at(size_type index) && {
    raw = detail::get_array<RefCount>(raw)->at_elem(index);
    if (is_null()) buffer_ref = nullptr;
    return std::move(*this);
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> basic_buffer<RefCount>::at_front() const& {
    if (empty()) throw std::out_of_range("dart::buffer is empty and has no value at front");
    else return front();
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::at_front() && {
    if (empty()) throw std::out_of_range("dart::buffer is empty and has no value at front");
    else return std::move(*this).front();
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> basic_buffer<RefCount>::at_back() const& {
    if (empty()) throw std::out_of_range("dart::buffer is empty and has no value at back");
    else return back();
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::at_back() && {
    if (empty()) throw std::out_of_range("dart::buffer is empty and has no value at back");
    else return std::move(*this).back();
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> basic_buffer<RefCount>::front() const& {
    auto* arr = detail::get_array<RefCount>(raw);
    if (empty()) return basic_buffer::make_null();
    else return basic_buffer(arr->get_elem(0), buffer_ref);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::front() && {
    auto* arr = detail::get_array<RefCount>(raw);
    if (empty()) raw = {detail::raw_type::null, nullptr};
    else raw = arr->get_elem(0);
    if (is_null()) buffer_ref = nullptr;
    return std::move(*this);
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> basic_buffer<RefCount>::back() const& {
    auto* arr = detail::get_array<RefCount>(raw);
    if (empty()) return basic_buffer::make_null();
    else return basic_buffer(arr->get_elem(size() - 1), buffer_ref);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::back() && {
    auto* arr = detail::get_array<RefCount>(raw);
    if (empty()) raw = {detail::raw_type::null, nullptr};
    else raw = arr->get_elem(size() - 1);
    if (is_null()) buffer_ref = nullptr;
    return std::move(*this);
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::capacity() const -> size_type {
    return size();
  }

}

#endif
