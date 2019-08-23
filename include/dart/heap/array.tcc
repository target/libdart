#ifndef DART_HEAP_ARRAY_H
#define DART_HEAP_ARRAY_H

/*----- Project Includes -----*/

#include "../common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <template <class> class RefCount>
  template <class... Args, class>
  basic_heap<RefCount> basic_heap<RefCount>::make_array(Args&&... elems) {
    auto arr = basic_heap(detail::array_tag {});
    convert::as_span<basic_heap>([&] (auto args) {
      // I don't know why this needs to be qualified,
      // but some older versions of gcc get weird without it
      basic_heap::push_elems<true>(arr, args);
    }, std::forward<Args>(elems)...);
    return arr;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount> basic_heap<RefCount>::make_array(gsl::span<basic_heap const> elems) {
    auto arr = basic_heap(detail::array_tag {});
    push_elems<false>(arr, elems);
    return arr;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount> basic_heap<RefCount>::make_array(gsl::span<basic_buffer<RefCount> const> elems) {
    auto arr = basic_heap(detail::array_tag {});
    push_elems<false>(arr, elems);
    return arr;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount> basic_heap<RefCount>::make_array(gsl::span<basic_packet<RefCount> const> elems) {
    auto arr = basic_heap(detail::array_tag {});
    push_elems<false>(arr, elems);
    return arr;
  }

  template <template <class> class RefCount>
  template <class ValueType, class>
  basic_heap<RefCount>& basic_heap<RefCount>::push_front(ValueType&& value) & {
    insert(0, std::forward<ValueType>(value));
    return *this;
  }

  template <template <class> class RefCount>
  template <class ValueType, class>
  basic_heap<RefCount>&& basic_heap<RefCount>::push_front(ValueType&& value) && {
    push_front(std::forward<ValueType>(value));
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount>& basic_heap<RefCount>::pop_front() & {
    erase(0);
    return *this;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount>&& basic_heap<RefCount>::pop_front() && {
    pop_front();
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <class ValueType, class>
  basic_heap<RefCount>& basic_heap<RefCount>::push_back(ValueType&& value) & {
    insert(size(), std::forward<ValueType>(value));
    return *this;
  }

  template <template <class> class RefCount>
  template <class ValueType, class>
  basic_heap<RefCount>&& basic_heap<RefCount>::push_back(ValueType&& value) && {
    push_back(std::forward<ValueType>(value));
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount>& basic_heap<RefCount>::pop_back() & {
    erase(size() - 1);
    return *this;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount>&& basic_heap<RefCount>::pop_back() && {
    pop_back();
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <class Number, template <class> class, class>
  auto basic_heap<RefCount>::erase(basic_number<Number> const& idx) -> iterator {
    return erase(idx.integer());
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  auto basic_heap<RefCount>::erase(size_type pos) -> iterator {
    // Make sure we copy out if our heap is shared.
    copy_on_write();

    // Attempt to perform the erase.
    auto& elements = get_elements();
    if (pos >= elements.size()) return end();
    auto new_it = elements.erase(elements.begin() + pos);
    return detail::dynamic_iterator<RefCount> {new_it, [] (auto& it) -> auto const& { return *it; }};
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  void basic_heap<RefCount>::reserve(size_type count) {
    get_elements().reserve(count);
  }

  template <template <class> class RefCount>
  template <class T, class>
  void basic_heap<RefCount>::resize(size_type count, T const& def) {
    get_elements().resize(count, convert::cast<basic_heap>(def));
  }

  template <template <class> class RefCount>
  template <class Number>
  basic_heap<RefCount> basic_heap<RefCount>::operator [](basic_number<Number> const& idx) const {
    return (*this)[idx.integer()];
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::operator [](size_type index) const {
    return get(index);
  }

  template <template <class> class RefCount>
  template <class Number>
  basic_heap<RefCount> basic_heap<RefCount>::get(basic_number<Number> const& idx) const {
    return get(idx.integer());
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::get(size_type index) const {
    auto& elems = get_elements();
    return (index < size()) ? elems[index] : basic_heap::make_null();
  }

  template <template <class> class RefCount>
  template <class Number, class T, class>
  basic_heap<RefCount> basic_heap<RefCount>::get_or(basic_number<Number> const& idx, T&& opt) const {
    return get_or(idx.integer(), std::forward<T>(opt));
  }

  template <template <class> class RefCount>
  template <class T, class>
  basic_heap<RefCount> basic_heap<RefCount>::get_or(size_type index, T&& opt) const {
    if (is_array() && size() > static_cast<size_t>(index)) return get(index);
    else return convert::cast<basic_heap>(std::forward<T>(opt));
  }

  template <template <class> class RefCount>
  template <class Number>
  basic_heap<RefCount> basic_heap<RefCount>::at(basic_number<Number> const& idx) const {
    return at(idx.integer());
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::at(size_type index) const {
    if (index < size()) return get_elements()[index];
    else throw std::out_of_range("dart::heap does not contain requested index");
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::at_front() const {
    if (empty()) throw std::out_of_range("dart::heap is empty and has no value at front");
    else return front();
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::at_back() const {
    if (empty()) throw std::out_of_range("dart::heap is empty and has no value at back");
    else return back();
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::front() const {
    auto& elements = get_elements();
    if (elements.empty()) return basic_heap::make_null();
    else return elements.front();
  }

  template <template <class> class RefCount>
  template <class T, class>
  basic_heap<RefCount> basic_heap<RefCount>::front_or(T&& opt) const {
    if (is_array() && !empty()) return front();
    else return convert::cast<basic_heap>(std::forward<T>(opt));
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::back() const {
    auto& elements = get_elements();
    if (elements.empty()) return basic_heap::make_null();
    else return elements.back();
  }

  template <template <class> class RefCount>
  template <class T, class>
  basic_heap<RefCount> basic_heap<RefCount>::back_or(T&& opt) const {
    if (is_array() && !empty()) return back();
    else return convert::cast<basic_heap>(std::forward<T>(opt));
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::capacity() const -> size_type {
    return get_elements().capacity();
  }

  template <template <class> class RefCount>
  template <bool consume, class Span>
  void basic_heap<RefCount>::push_elems(basic_heap& arr, Span elems) {
    // Validate that what we're being asked to do makes sense.
    if (!arr.is_array()) throw type_error("dart::heap is not an array and cannot push elements");
    
    // Push each of the requested elements.
    arr.reserve(elems.size());
    for (auto& elem : elems) {
      if (consume) arr.push_back(std::move(elem));
      else arr.push_back(elem);
    }
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::iterator_index(iterator pos) const -> size_type {
    // Dig all the way down and get the underlying iterator layout.
    using elements_layout = typename detail::dynamic_iterator<RefCount>::elements_layout;
    return shim::visit(
      shim::compose_together(
        [] (elements_type const& elems, elements_layout& layout) -> size_type {
          return layout.it - elems->begin();
        },
        [] (fields_type const&, auto&) -> size_type {
          throw type_error("dart::heap is an object, and cannot perform array operations");
        },
        [] (auto&, auto&) -> size_type {
          throw type_error("dart::heap is not an array, "
            "or was provided an invalid iterator, and cannot perform array operations");
        }
      ),
      data,
      pos.impl->impl
    );
  }

}

#endif
