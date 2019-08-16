#ifndef DART_ARR_IMPL_H
#define DART_ARR_IMPL_H

/*----- Local Includes -----*/

#include "dart_intern.h"

/*----- Function Implementations -----*/

namespace dart {

  template <class Array>
  template <class Arr, class>
  basic_array<Array>::basic_array(Arr&& arr) : val(std::forward<Arr>(arr)) {
    if (!val.is_array()) {
      throw type_error("dart::packet::array can only be constructed as an array.");
    }
  }

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
  template <class... Args, class>
  basic_packet<RefCount> basic_packet<RefCount>::make_array(Args&&... elems) {
    return basic_heap<RefCount>::make_array(std::forward<Args>(elems)...);
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
  basic_packet<RefCount> basic_packet<RefCount>::make_array(gsl::span<basic_heap<RefCount> const> elems) {
    return basic_heap<RefCount>::make_array(elems);
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
  basic_packet<RefCount> basic_packet<RefCount>::make_array(gsl::span<basic_buffer<RefCount> const> elems) {
    return basic_heap<RefCount>::make_array(elems);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount> basic_heap<RefCount>::make_array(gsl::span<basic_packet<RefCount> const> elems) {
    auto arr = basic_heap(detail::array_tag {});
    push_elems<false>(arr, elems);
    return arr;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount> basic_packet<RefCount>::make_array(gsl::span<basic_packet<RefCount> const> elems) {
    return basic_heap<RefCount>::make_array(elems);
  }

  template <class Array>
  template <class ValueType, class>
  basic_array<Array>& basic_array<Array>::push_front(ValueType&& value) & {
    val.push_front(std::forward<ValueType>(value));
    return *this;
  }

  template <template <class> class RefCount>
  template <class ValueType, class>
  basic_heap<RefCount>& basic_heap<RefCount>::push_front(ValueType&& value) & {
    insert(0, std::forward<ValueType>(value));
    return *this;
  }

  template <template <class> class RefCount>
  template <class ValueType, class>
  basic_packet<RefCount>& basic_packet<RefCount>::push_front(ValueType&& value) & {
    get_heap().push_front(std::forward<ValueType>(value));
    return *this;
  }

  template <class Array>
  template <class ValueType, class>
  basic_array<Array>&& basic_array<Array>::push_front(ValueType&& value) && {
    auto&& tmp = std::move(val).push_front(std::forward<ValueType>(value));
    (void) tmp;
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <class ValueType, class>
  basic_heap<RefCount>&& basic_heap<RefCount>::push_front(ValueType&& value) && {
    push_front(std::forward<ValueType>(value));
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <class ValueType, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::push_front(ValueType&& value) && {
    push_front(std::forward<ValueType>(value));
    return std::move(*this);
  }
  
  template <class Array>
  template <class, class>
  basic_array<Array>& basic_array<Array>::pop_front() & {
    val.pop_front();
    return *this;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount>& basic_heap<RefCount>::pop_front() & {
    erase(0);
    return *this;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>& basic_packet<RefCount>::pop_front() & {
    get_heap().pop_front();
    return *this;
  }

  template <class Array>
  template <class, class>
  basic_array<Array>&& basic_array<Array>::pop_front() && {
    auto&& tmp = std::move(val).pop_front();
    (void) tmp;
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount>&& basic_heap<RefCount>::pop_front() && {
    pop_front();
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::pop_front() && {
    pop_front();
    return std::move(*this);
  }

  template <class Array>
  template <class ValueType, class>
  basic_array<Array>& basic_array<Array>::push_back(ValueType&& value) & {
    val.push_back(std::forward<ValueType>(value));
    return *this;
  }

  template <template <class> class RefCount>
  template <class ValueType, class>
  basic_heap<RefCount>& basic_heap<RefCount>::push_back(ValueType&& value) & {
    insert(size(), std::forward<ValueType>(value));
    return *this;
  }

  template <template <class> class RefCount>
  template <class ValueType, class>
  basic_packet<RefCount>& basic_packet<RefCount>::push_back(ValueType&& value) & {
    get_heap().push_back(std::forward<ValueType>(value));
    return *this;
  }

  template <class Array>
  template <class ValueType, class>
  basic_array<Array>&& basic_array<Array>::push_back(ValueType&& value) && {
    auto&& tmp = std::move(val).push_back(std::forward<ValueType>(value));
    (void) tmp;
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <class ValueType, class>
  basic_heap<RefCount>&& basic_heap<RefCount>::push_back(ValueType&& value) && {
    push_back(std::forward<ValueType>(value));
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <class ValueType, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::push_back(ValueType&& value) && {
    push_back(std::forward<ValueType>(value));
    return std::move(*this);
  }

  template <class Array>
  template <class, class>
  basic_array<Array>& basic_array<Array>::pop_back() & {
    val.pop_back();
    return *this;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount>& basic_heap<RefCount>::pop_back() & {
    erase(size() - 1);
    return *this;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>& basic_packet<RefCount>::pop_back() & {
    get_heap().pop_back();
    return *this;
  }

  template <class Array>
  template <class, class>
  basic_array<Array>&& basic_array<Array>::pop_back() && {
    auto&& tmp = std::move(val).pop_back();
    (void) tmp;
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount>&& basic_heap<RefCount>::pop_back() && {
    pop_back();
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::pop_back() && {
    pop_back();
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

  template <template <class> class RefCount>
  template <class Number, template <class> class, class>
  auto basic_heap<RefCount>::erase(basic_number<Number> const& idx) -> iterator {
    return erase(idx.integer());
  }

  template <template <class> class RefCount>
  template <class Number, template <class> class, class>
  auto basic_packet<RefCount>::erase(basic_number<Number> const& idx) -> iterator {
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
  auto basic_packet<RefCount>::erase(size_type pos) -> iterator {
    return get_heap().erase(pos);
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
  template <template <class> class, class>
  void basic_packet<RefCount>::reserve(size_type count) {
    get_heap().reserve(count);
  }

  template <template <class> class RefCount>
  template <class T, class>
  void basic_packet<RefCount>::resize(size_type count, T const& def) {
    get_heap().resize(count, def);
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

  template <template <class> class RefCount>
  template <class Number>
  basic_heap<RefCount> basic_heap<RefCount>::operator [](basic_number<Number> const& idx) const {
    return (*this)[idx.integer()];
  }

  template <template <class> class RefCount>
  template <class Number>
  basic_buffer<RefCount> basic_buffer<RefCount>::operator [](basic_number<Number> const& idx) const& {
    return (*this)[idx.integer()];
  }

  template <template <class> class RefCount>
  template <class Number>
  basic_packet<RefCount> basic_packet<RefCount>::operator [](basic_number<Number> const& idx) const& {
    return (*this)[idx.integer()];
  }

  template <template <class> class RefCount>
  template <class Number, template <class> class, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::operator [](basic_number<Number> const& idx) && {
    return std::move(*this)[idx.integer()];
  }

  template <template <class> class RefCount>
  template <class Number, template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::operator [](basic_number<Number> const& idx) && {
    return std::move(*this)[idx.integer()];
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::operator [](size_type index) const {
    return get(index);
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> basic_buffer<RefCount>::operator [](size_type index) const& {
    return get(index);
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::operator [](size_type index) const& {
    return get(index);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::operator [](size_type index) && {
    return std::move(*this).get(index);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::operator [](size_type index) && {
    return std::move(*this).get(index);
  }

  template <class Array>
  template <class Index, class>
  auto basic_array<Array>::get(Index const& idx) const& -> value_type {
    return val.get(idx);
  }

  template <template <class> class RefCount>
  template <class Number>
  basic_heap<RefCount> basic_heap<RefCount>::get(basic_number<Number> const& idx) const {
    return get(idx.integer());
  }

  template <template <class> class RefCount>
  template <class Number>
  basic_buffer<RefCount> basic_buffer<RefCount>::get(basic_number<Number> const& idx) const& {
    return get(idx.integer());
  }

  template <class Array>
  template <class Index, class>
  decltype(auto) basic_array<Array>::get(Index const& idx) && {
    return std::move(val).get(idx);
  }

  template <template <class> class RefCount>
  template <class Number, template <class> class, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::get(basic_number<Number> const& idx) && {
    return std::move(*this).get(idx.integer());
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
  basic_heap<RefCount> basic_heap<RefCount>::get(size_type index) const {
    auto& elems = get_elements();
    return (index < size()) ? elems[index] : basic_heap::make_null();
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> basic_buffer<RefCount>::get(size_type index) const& {
    return basic_buffer(detail::get_array<RefCount>(raw)->get_elem(index), buffer_ref);
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::get(size_type index) const& {
    return shim::visit([&] (auto& v) -> basic_packet { return v.get(index); }, impl);
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::get(size_type index) && {
    raw = detail::get_array<RefCount>(raw)->get_elem(index);
    if (is_null()) buffer_ref = nullptr;
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::get(size_type index) && {
    shim::visit([&] (auto& v) { v = std::move(v).get(index); }, impl);
    return std::move(*this);
  }
  
  template <class Array>
  template <class KeyType, class T, class>
  auto basic_array<Array>::get_or(KeyType const& idx, T&& opt) const -> value_type {
    return val.get_or(idx, std::forward<T>(opt));
  }

  template <template <class> class RefCount>
  template <class Number, class T, class>
  basic_heap<RefCount> basic_heap<RefCount>::get_or(basic_number<Number> const& idx, T&& opt) const {
    return get_or(idx.integer(), std::forward<T>(opt));
  }

  template <template <class> class RefCount>
  template <class Number, class T, class>
  basic_packet<RefCount> basic_packet<RefCount>::get_or(basic_number<Number> const& idx, T&& opt) const {
    return get_or(idx.integer(), std::forward<T>(opt));
  }

  template <template <class> class RefCount>
  template <class T, class>
  basic_heap<RefCount> basic_heap<RefCount>::get_or(size_type index, T&& opt) const {
    if (is_array() && size() > static_cast<size_t>(index)) return get(index);
    else return convert::cast<basic_heap>(std::forward<T>(opt));
  }

  template <template <class> class RefCount>
  template <class T, class>
  basic_packet<RefCount> basic_packet<RefCount>::get_or(size_type index, T&& opt) const {
    if (is_array() && size() > static_cast<size_t>(index)) return get(index);
    else return convert::cast<basic_packet>(std::forward<T>(opt));
  }

  template <class Array>
  template <class Index, class>
  auto basic_array<Array>::at(Index const& idx) const& -> value_type {
    return val.at(idx);
  }

  template <template <class> class RefCount>
  template <class Number>
  basic_heap<RefCount> basic_heap<RefCount>::at(basic_number<Number> const& idx) const {
    return at(idx.integer());
  }

  template <template <class> class RefCount>
  template <class Number>
  basic_buffer<RefCount> basic_buffer<RefCount>::at(basic_number<Number> const& idx) const& {
    return at(idx.integer());
  }

  template <class Array>
  template <class Index, class>
  decltype(auto) basic_array<Array>::at(Index const& idx) && {
    return std::move(val).at(idx);
  }

  template <template <class> class RefCount>
  template <class Number, template <class> class, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::at(basic_number<Number> const& idx) && {
    return std::move(*this).at(idx.integer());
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
  basic_heap<RefCount> basic_heap<RefCount>::at(size_type index) const {
    if (index < size()) return get_elements()[index];
    else throw std::out_of_range("dart::heap does not contain requested index");
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> basic_buffer<RefCount>::at(size_type index) const& {
    return basic_buffer(detail::get_array<RefCount>(raw)->at_elem(index), buffer_ref);
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::at(size_type index) const& {
    return shim::visit([&] (auto& v) -> basic_packet { return v.at(index); }, impl);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::at(size_type index) && {
    raw = detail::get_array<RefCount>(raw)->at_elem(index);
    if (is_null()) buffer_ref = nullptr;
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::at(size_type index) && {
    shim::visit([&] (auto& v) { v = std::move(v).at(index); }, impl);
    return std::move(*this);
  }

  template <class Array>
  auto basic_array<Array>::at_front() const& -> value_type {
    return val.at_front();
  }

  template <class Array>
  decltype(auto) basic_array<Array>::at_front() && {
    return std::move(val).at_front();
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::at_front() const {
    if (empty()) throw std::out_of_range("dart::heap is empty and has no value at front");
    else return front();
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> basic_buffer<RefCount>::at_front() const& {
    if (empty()) throw std::out_of_range("dart::buffer is empty and has no value at front");
    else return front();
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::at_front() const& {
    return shim::visit([] (auto& v) -> basic_packet { return v.at_front(); }, impl);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::at_front() && {
    if (empty()) throw std::out_of_range("dart::buffer is empty and has no value at front");
    else return std::move(*this).front();
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::at_front() && {
    shim::visit([] (auto& v) -> basic_packet { v = std::move(v).at_front(); }, impl);
    return std::move(*this);
  }

  template <class Array>
  auto basic_array<Array>::at_back() const& -> value_type {
    return val.at_back();
  }

  template <class Array>
  decltype(auto) basic_array<Array>::at_back() && {
    return std::move(val).at_back();
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::at_back() const {
    if (empty()) throw std::out_of_range("dart::heap is empty and has no value at back");
    else return back();
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> basic_buffer<RefCount>::at_back() const& {
    if (empty()) throw std::out_of_range("dart::buffer is empty and has no value at back");
    else return back();
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::at_back() const& {
    return shim::visit([] (auto& v) -> basic_packet { return v.at_back(); }, impl);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::at_back() && {
    if (empty()) throw std::out_of_range("dart::buffer is empty and has no value at back");
    else return std::move(*this).back();
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::at_back() && {
    shim::visit([] (auto& v) -> basic_packet { v = std::move(v).at_back(); }, impl);
    return std::move(*this);
  }

  template <class Array>
  auto basic_array<Array>::front() const& -> value_type {
    return val.front();
  }

  template <class Array>
  decltype(auto) basic_array<Array>::front() && {
    return std::move(val).front();
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::front() const {
    auto& elements = get_elements();
    if (elements.empty()) return basic_heap::make_null();
    else return elements.front();
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> basic_buffer<RefCount>::front() const& {
    auto* arr = detail::get_array<RefCount>(raw);
    if (empty()) return basic_buffer::make_null();
    else return basic_buffer(arr->get_elem(0), buffer_ref);
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::front() const& {
    return shim::visit([] (auto& v) -> basic_packet { return v.front(); }, impl);
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
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::front() && {
    shim::visit([] (auto& v) { v = std::move(v).front(); }, impl);
    return std::move(*this);
  }
  
  template <class Array>
  template <class T, class>
  auto basic_array<Array>::front_or(T&& opt) const -> value_type {
    return val.front_or(std::forward<T>(opt));
  }

  template <template <class> class RefCount>
  template <class T, class>
  basic_heap<RefCount> basic_heap<RefCount>::front_or(T&& opt) const {
    if (is_array() && !empty()) return front();
    else return convert::cast<basic_heap>(std::forward<T>(opt));
  }

  template <template <class> class RefCount>
  template <class T, class>
  basic_packet<RefCount> basic_packet<RefCount>::front_or(T&& opt) const {
    return get_heap().front_or(std::forward<T>(opt));
  }

  template <class Array>
  auto basic_array<Array>::back() const& -> value_type {
    return val.back();
  }

  template <class Array>
  decltype(auto) basic_array<Array>::back() && {
    return std::move(val).back();
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::back() const {
    auto& elements = get_elements();
    if (elements.empty()) return basic_heap::make_null();
    else return elements.back();
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
  basic_packet<RefCount> basic_packet<RefCount>::back() const& {
    return shim::visit([] (auto& v) -> basic_packet { return v.back(); }, impl);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::back() && {
    shim::visit([] (auto& v) { v = std::move(v).back(); }, impl);
    return std::move(*this);
  }

  template <class Array>
  template <class T, class>
  auto basic_array<Array>::back_or(T&& opt) const -> value_type {
    return val.back_or(std::forward<T>(opt));
  }

  template <template <class> class RefCount>
  template <class T, class>
  basic_heap<RefCount> basic_heap<RefCount>::back_or(T&& opt) const {
    if (is_array() && !empty()) return back();
    else return convert::cast<basic_heap>(std::forward<T>(opt));
  }

  template <template <class> class RefCount>
  template <class T, class>
  basic_packet<RefCount> basic_packet<RefCount>::back_or(T&& opt) const {
    return get_heap().back_or(std::forward<T>(opt));
  }

  template <class Array>
  auto basic_array<Array>::capacity() const -> size_type {
    return val.capacity();
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::capacity() const -> size_type {
    return get_elements().capacity();
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::capacity() const -> size_type {
    return size();
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::capacity() const -> size_type {
    return shim::visit([] (auto& v) { return v.capacity(); }, impl);
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
