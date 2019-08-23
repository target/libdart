#ifndef DART_PACKET_ARRAY_H
#define DART_PACKET_ARRAY_H

/*----- Project Includes -----*/

#include "../common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <template <class> class RefCount>
  template <class... Args, class>
  basic_packet<RefCount> basic_packet<RefCount>::make_array(Args&&... elems) {
    return basic_heap<RefCount>::make_array(std::forward<Args>(elems)...);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount> basic_packet<RefCount>::make_array(gsl::span<basic_heap<RefCount> const> elems) {
    return basic_heap<RefCount>::make_array(elems);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount> basic_packet<RefCount>::make_array(gsl::span<basic_buffer<RefCount> const> elems) {
    return basic_heap<RefCount>::make_array(elems);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount> basic_packet<RefCount>::make_array(gsl::span<basic_packet<RefCount> const> elems) {
    return basic_heap<RefCount>::make_array(elems);
  }

  template <template <class> class RefCount>
  template <class ValueType, class>
  basic_packet<RefCount>& basic_packet<RefCount>::push_front(ValueType&& value) & {
    get_heap().push_front(std::forward<ValueType>(value));
    return *this;
  }

  template <template <class> class RefCount>
  template <class ValueType, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::push_front(ValueType&& value) && {
    push_front(std::forward<ValueType>(value));
    return std::move(*this);
  }
  
  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>& basic_packet<RefCount>::pop_front() & {
    get_heap().pop_front();
    return *this;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::pop_front() && {
    pop_front();
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <class ValueType, class>
  basic_packet<RefCount>& basic_packet<RefCount>::push_back(ValueType&& value) & {
    get_heap().push_back(std::forward<ValueType>(value));
    return *this;
  }

  template <template <class> class RefCount>
  template <class ValueType, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::push_back(ValueType&& value) && {
    push_back(std::forward<ValueType>(value));
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>& basic_packet<RefCount>::pop_back() & {
    get_heap().pop_back();
    return *this;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::pop_back() && {
    pop_back();
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <class Number, template <class> class, class>
  auto basic_packet<RefCount>::erase(basic_number<Number> const& idx) -> iterator {
    return erase(idx.integer());
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  auto basic_packet<RefCount>::erase(size_type pos) -> iterator {
    return get_heap().erase(pos);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  void basic_packet<RefCount>::reserve(size_type count) {
    get_heap().reserve(count);
  }

  template <template <class> class RefCount>
  template <class T, class>
  void basic_packet<RefCount>::resize(size_type count, T const& def) {
    get_heap().resize(count, def);
  }

  template <template <class> class RefCount>
  template <class Number>
  basic_packet<RefCount> basic_packet<RefCount>::operator [](basic_number<Number> const& idx) const& {
    return (*this)[idx.integer()];
  }

  template <template <class> class RefCount>
  template <class Number, template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::operator [](basic_number<Number> const& idx) && {
    return std::move(*this)[idx.integer()];
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::operator [](size_type index) const& {
    return get(index);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::operator [](size_type index) && {
    return std::move(*this).get(index);
  }

  template <template <class> class RefCount>
  template <class Number>
  basic_packet<RefCount> basic_packet<RefCount>::get(basic_number<Number> const& idx) const& {
    return get(idx.integer());
  }

  template <template <class> class RefCount>
  template <class Number, template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::get(basic_number<Number> const& idx) && {
    return std::move(*this).get(idx.integer());
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::get(size_type index) const& {
    return shim::visit([&] (auto& v) -> basic_packet { return v.get(index); }, impl);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::get(size_type index) && {
    shim::visit([&] (auto& v) { v = std::move(v).get(index); }, impl);
    return std::move(*this);
  }
  
  template <template <class> class RefCount>
  template <class Number, class T, class>
  basic_packet<RefCount> basic_packet<RefCount>::get_or(basic_number<Number> const& idx, T&& opt) const {
    return get_or(idx.integer(), std::forward<T>(opt));
  }

  template <template <class> class RefCount>
  template <class T, class>
  basic_packet<RefCount> basic_packet<RefCount>::get_or(size_type index, T&& opt) const {
    if (is_array() && size() > static_cast<size_t>(index)) return get(index);
    else return convert::cast<basic_packet>(std::forward<T>(opt));
  }

  template <template <class> class RefCount>
  template <class Number>
  basic_packet<RefCount> basic_packet<RefCount>::at(basic_number<Number> const& idx) const& {
    return at(idx.integer());
  }

  template <template <class> class RefCount>
  template <class Number, template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::at(basic_number<Number> const& idx) && {
    return std::move(*this).at(idx.integer());
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::at(size_type index) const& {
    return shim::visit([&] (auto& v) -> basic_packet { return v.at(index); }, impl);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::at(size_type index) && {
    shim::visit([&] (auto& v) { v = std::move(v).at(index); }, impl);
    return std::move(*this);
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::at_front() const& {
    return shim::visit([] (auto& v) -> basic_packet { return v.at_front(); }, impl);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::at_front() && {
    shim::visit([] (auto& v) -> basic_packet { v = std::move(v).at_front(); }, impl);
    return std::move(*this);
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::at_back() const& {
    return shim::visit([] (auto& v) -> basic_packet { return v.at_back(); }, impl);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::at_back() && {
    shim::visit([] (auto& v) -> basic_packet { v = std::move(v).at_back(); }, impl);
    return std::move(*this);
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::front() const& {
    return shim::visit([] (auto& v) -> basic_packet { return v.front(); }, impl);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::front() && {
    shim::visit([] (auto& v) { v = std::move(v).front(); }, impl);
    return std::move(*this);
  }
  
  template <template <class> class RefCount>
  template <class T, class>
  basic_packet<RefCount> basic_packet<RefCount>::front_or(T&& opt) const {
    return get_heap().front_or(std::forward<T>(opt));
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::back() const& {
    return shim::visit([] (auto& v) -> basic_packet { return v.back(); }, impl);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::back() && {
    shim::visit([] (auto& v) { v = std::move(v).back(); }, impl);
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <class T, class>
  basic_packet<RefCount> basic_packet<RefCount>::back_or(T&& opt) const {
    return get_heap().back_or(std::forward<T>(opt));
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::capacity() const -> size_type {
    return shim::visit([] (auto& v) { return v.capacity(); }, impl);
  }

}

#endif
