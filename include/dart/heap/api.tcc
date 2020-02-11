#ifndef DART_HEAP_API_H
#define DART_HEAP_API_H

/*----- Project Includes -----*/

#include "../common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <template <class> class RefCount>
  basic_heap<RefCount>::basic_heap(basic_heap&& other) noexcept : data(std::move(other.data)) {
    other.data = shim::monostate {};
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>& basic_heap<RefCount>::operator =(basic_heap&& other) & noexcept {
    if (this == &other) return *this;

    // Defer to destructor and move constructor to keep from copying stateful, easy to screw up,
    // code all over the place.
    this->~basic_heap();
    new(this) basic_heap(std::move(other));
    return *this;
  }

  template <template <class> class RefCount>
  template <class T, class EnableIf>
  basic_heap<RefCount>& basic_heap<RefCount>::operator =(T&& other) & {
    *this = convert::cast<basic_heap>(std::forward<T>(other));
    return *this;
  }

  template <template <class> class RefCount>
  template <class KeyType, class EnableIf>
  basic_heap<RefCount> basic_heap<RefCount>::operator [](KeyType const& identifier) const {
    return get(identifier);
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::operator ->() noexcept -> basic_heap* {
    return this;
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::operator ->() const noexcept -> basic_heap const* {
    return this;
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>::operator bool() const noexcept {
    if (!is_boolean()) return !is_null();
    else return boolean();
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>::operator view() const& noexcept {
    return view {detail::view_tag {}, data};
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>::operator std::string() const {
    return std::string {strv()};
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>::operator shim::string_view() const {
    return strv();
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>::operator int64_t() const {
    return integer();
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>::operator double() const {
    return decimal();
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::make_null() noexcept {
    return basic_heap(detail::null_tag {});
  }

  template <template <class> class RefCount>
  template <class KeyType, class ValueType, class EnableIf>
  auto basic_heap<RefCount>::insert(KeyType&& key, ValueType&& value) -> iterator {
    // Perform our copy on write if our heap is shared.
    copy_on_write();

    // Cast and forward the key/value into something we can insert.
    auto&& tmp_key = convert::cast<basic_heap>(std::forward<KeyType>(key));
    auto&& tmp_value = convert::cast<basic_heap>(std::forward<ValueType>(value));

    // Perform the insertion.
    if (tmp_key.is_str()) {
      // Check to make sure we'll be able to encode the given string.
      if (tmp_key.size() > std::numeric_limits<uint16_t>::max()) {
        throw std::invalid_argument("dart::heap keys cannot be longer than UINT16_MAX");
      }

      // Attempt the insertion.
      // Sucks that we have to copy the value here, but insert might fail,
      // and the standard doesn't guarantee that tmp_value wouldn't be consumed if so.
      auto val = std::make_pair(std::forward<decltype(tmp_key)>(tmp_key), tmp_value);
      auto res = get_fields().insert(std::move(val));

      // If the key was already present, insert will fail without consuming our value.
      // Overwrite it.
      if (!res.second) res.first->second = std::move(tmp_value);
      return detail::dynamic_iterator<RefCount> {res.first, [] (auto& it) -> auto const& { return it->second; }};
    } else if (tmp_key.is_integer()) {
      auto& elements = get_elements();
      auto pos = static_cast<size_type>(tmp_key.integer());
      if (pos > elements.size()) throw std::out_of_range("dart::heap cannot insert at out of range index");
      auto new_it = elements.insert(elements.begin() + pos, std::forward<decltype(tmp_value)>(tmp_value));
      return detail::dynamic_iterator<RefCount> {new_it, [] (auto& it) -> auto const& { return *it; }};
    } else {
      throw type_error("dart::heap cannot insert keys with non string/integer types");
    }
  }

  template <template <class> class RefCount>
  template <class ValueType, class EnableIf>
  auto basic_heap<RefCount>::insert(iterator pos, ValueType&& value) -> iterator {
    // Dig all the way down and get the underlying iterator layout.
    using elements_layout = typename detail::dynamic_iterator<RefCount>::elements_layout;

    // Make sure our iterator can be used.
    if (!pos) throw std::invalid_argument("dart::heap cannot insert from a valueless iterator");

    // Map the interator's index, and perform the insertion.
    if (is_object()) return insert(iterator_key(pos), std::forward<ValueType>(value));
    else return insert(iterator_index(pos), std::forward<ValueType>(value));
  }

  template <template <class> class RefCount>
  template <class KeyType, class ValueType, class EnableIf>
  auto basic_heap<RefCount>::set(KeyType&& key, ValueType&& value) -> iterator {
    // Perform our copy on write if our heap is shared.
    copy_on_write();

    // Cast and forward the key/value into something we can use.
    auto&& tmp_key = convert::cast<basic_heap>(std::forward<KeyType>(key));
    auto&& tmp_val = convert::cast<basic_heap>(std::forward<ValueType>(value));

    // Update the given value IF present.
    if (tmp_key.is_str()) {
      if (tmp_key.size() > std::numeric_limits<uint16_t>::max()) {
        throw std::invalid_argument("dart::heap keys cannot be longer than UINT16_MAX");
      }

      // Update the mapping IF it exists.
      auto& fields = get_fields();
      auto it = fields.find(tmp_key);
      if (it == fields.end()) throw std::out_of_range("dart::heap cannot set a non-existent key");
      it->second = std::forward<decltype(tmp_val)>(tmp_val);
      return detail::dynamic_iterator<RefCount>{it, [] (auto& it) -> auto const& { return it->second; }};
    } else if (tmp_key.is_integer()) {
      auto& elements = get_elements();
      auto pos = static_cast<size_type>(tmp_key.integer());
      if (pos >= elements.size()) throw std::out_of_range("dart::heap cannot set a value at out of range index");
      auto it = elements.begin() + pos;
      *it = std::forward<decltype(tmp_val)>(tmp_val);
      return detail::dynamic_iterator<RefCount> {it, [] (auto& it) -> auto const& { return *it; }};
    } else {
      throw type_error("dart::heap cannot set keys with non string/integer types");
    }
  }

  template <template <class> class RefCount>
  template <class ValueType, class EnableIf>
  auto basic_heap<RefCount>::set(iterator pos, ValueType&& value) -> iterator {
    // Dig all the way down and get the underlying iterator layout.
    using elements_layout = typename detail::dynamic_iterator<RefCount>::elements_layout;

    // Make sure our iterator can be used.
    if (!pos) throw std::invalid_argument("dart::heap cannot insert from a valueless iterator");

    // Map the interator's index, and perform the insertion.
    if (is_object()) return set(iterator_key(pos), std::forward<ValueType>(value));
    else return set(iterator_index(pos), std::forward<ValueType>(value));
  }

  template <template <class> class RefCount>
  template <class KeyType, class EnableIf>
  auto basic_heap<RefCount>::erase(KeyType const& identifier) -> iterator {
    switch (identifier.get_type()) {
      case type::string:
        return erase(identifier.strv());
      case type::integer:
        return erase(identifier.integer());
      default:
        throw type_error("dart::heap cannot erase values with non-string/integer type.");
    }
  }

  template <template <class> class RefCount>
  template <bool enabled, class EnableIf>
  auto basic_heap<RefCount>::erase(iterator pos) -> iterator {
    // Dig all the way down and get the underlying iterator layout.
    using fields_layout = typename detail::dynamic_iterator<RefCount>::fields_layout;

    // Make sure our iterator can be used.
    if (!pos) throw std::invalid_argument("dart::heap cannot erase from a valueless iterator");

    // Map the interator's key/index, and perform the insertion.
    // I wish we could do better than this, but copy-on-write semantics make it extraordinarily
    // difficult to work with STL iterators like this.
    if (is_object()) {
      try {
        return erase_key_impl(iterator_key(pos),
            shim::get<fields_layout>(pos.impl->impl).deref, shim::get<fields_type>(data));
      } catch (...) {
        throw type_error("dart::heap cannot erase with iterator of wrong type");
      }
    } else {
      return erase(iterator_index(pos));
    }
  }

  template <template <class> class RefCount>
  template <bool enabled, class EnableIf>
  void basic_heap<RefCount>::clear() {
    if (is_object()) get_fields().clear();
    else if (is_array()) get_elements().clear();
    else throw type_error("dart::heap is not an aggregate and cannot be cleared");
  }

  template <template <class> class RefCount>
  template <bool enabled, class EnableIf>
  basic_heap<RefCount>& basic_heap<RefCount>::definalize() & {
    return *this;
  }

  template <template <class> class RefCount>
  template <bool enabled, class EnableIf>
  basic_heap<RefCount> const& basic_heap<RefCount>::definalize() const& {
    return *this;
  }

  template <template <class> class RefCount>
  template <bool enabled, class EnableIf>
  basic_heap<RefCount>&& basic_heap<RefCount>::definalize() && {
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <bool enabled, class EnableIf>
  basic_heap<RefCount> const&& basic_heap<RefCount>::definalize() const&& {
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <bool enabled, class EnableIf>
  basic_heap<RefCount>& basic_heap<RefCount>::lift() & {
    return definalize();
  }

  template <template <class> class RefCount>
  template <bool enabled, class EnableIf>
  basic_heap<RefCount> const& basic_heap<RefCount>::lift() const& {
    return definalize();
  }

  template <template <class> class RefCount>
  template <bool enabled, class EnableIf>
  basic_heap<RefCount>&& basic_heap<RefCount>::lift() && {
    return std::move(*this).definalize();
  }

  template <template <class> class RefCount>
  template <bool enabled, class EnableIf>
  basic_heap<RefCount> const&& basic_heap<RefCount>::lift() const&& {
    return std::move(*this).definalize();
  }

  template <template <class> class RefCount>
  template <bool enabled, class EnableIf>
  basic_buffer<RefCount> basic_heap<RefCount>::finalize() const {
    return basic_buffer<RefCount> {*this};
  }

  template <template <class> class RefCount>
  template <bool enabled, class EnableIf>
  basic_buffer<RefCount> basic_heap<RefCount>::lower() const {
    return finalize();
  }

  template <template <class> class RefCount>
  template <template <class> class NewCount>
  basic_heap<NewCount> basic_heap<RefCount>::transmogrify(basic_heap const& heap) {
    switch (heap.get_type()) {
      case type::object:
        {
          basic_heap::iterator k, v;
          std::tie(k, v) = heap.kvbegin();
          auto obj = basic_heap<NewCount>::make_object();
          while (v != heap.end()) {
            obj.add_field(transmogrify<NewCount>(*k), transmogrify<NewCount>(*v));
            ++k, ++v;
          }
          return obj;
        }
      case type::array:
        {
          auto arr = basic_heap<NewCount>::make_array();
          for (auto elem : heap) arr.push_back(transmogrify<NewCount>(std::move(elem)));
          return arr;
        }
      case type::string:
        return basic_heap<NewCount>::make_string(heap.strv());
      case type::integer:
        return basic_heap<NewCount>::make_integer(heap.integer());
      case type::decimal:
        return basic_heap<NewCount>::make_decimal(heap.decimal());
      case type::boolean:
        return basic_heap<NewCount>::make_boolean(heap.boolean());
      default:
        DART_ASSERT(heap.get_type() == type::null);
        return basic_heap<NewCount>::make_null();
    }
  }

  template <template <class> class RefCount>
  template <class KeyType, class EnableIf>
  basic_heap<RefCount> basic_heap<RefCount>::get(KeyType const& identifier) const {
    switch (identifier.get_type()) {
      case type::string:
        return get(identifier.strv());
      case type::integer:
        return get(identifier.integer());
      default:
        throw type_error("dart::heap cannot retrieve values with non-string/integer type.");
    }
  }

  template <template <class> class RefCount>
  template <class KeyType, class T, class EnableIf>
  basic_heap<RefCount> basic_heap<RefCount>::get_or(KeyType const& identifier, T&& opt) const {
    // Need an extra check for objects because objects return null if the key is missing.
    // Array access with throw if the identifier isn't within or range, or isn't an integer.
    if (is_object() && has_key(identifier)) {
      return get(identifier);
    } else if (is_array() && size() > static_cast<size_t>(identifier.integer())) {
      return get(identifier);
    } else {
      return convert::cast<basic_heap>(std::forward<T>(opt));
    }
  }

  template <template <class> class RefCount>
  template <class KeyType, class EnableIf>
  basic_heap<RefCount> basic_heap<RefCount>::at(KeyType const& identifier) const {
    switch (identifier.at_type()) {
      case type::string:
        return at(identifier.strv());
      case type::integer:
        return at(identifier.integer());
      default:
        throw type_error("dart::heap cannot retrieve values with non-string/integer type.");
    }
  }

  template <template <class> class RefCount>
  std::vector<basic_heap<RefCount>> basic_heap<RefCount>::values() const {
    return detail::values_impl(*this);
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::size() const -> size_type {
    // Validate sanity.
    if (!is_aggregate() && !is_str()) {
      throw type_error("dart::heap is a primitive, and has no size.");
    }

    // Get our size.
    if (is_object()) {
      return try_get_fields()->size();
    } else if (is_array()) {
      return try_get_elements()->size();
    } else {
      return strv().size();
    }
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::empty() const {
    return size() == 0ULL;
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::is_object() const noexcept {
    return shim::holds_alternative<fields_type>(data);
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::is_array() const noexcept {
    return shim::holds_alternative<elements_type>(data);
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::is_aggregate() const noexcept {
    return is_object() || is_array();
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::is_str() const noexcept {
    return shim::holds_alternative<dynamic_string_layout>(data)
      || shim::holds_alternative<inline_string_layout>(data);
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::is_integer() const noexcept {
    return shim::holds_alternative<int64_t>(data);
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::is_decimal() const noexcept {
    return shim::holds_alternative<double>(data);
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::is_numeric() const noexcept {
    return is_integer() || is_decimal();
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::is_boolean() const noexcept {
    return shim::holds_alternative<bool>(data);
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::is_null() const noexcept {
    return shim::holds_alternative<shim::monostate>(data);
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::is_primitive() const noexcept {
    return !is_object() && !is_array() && !is_null();
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::get_type() const noexcept -> type {
    if (is_object()) {
      return type::object;
    } else if (is_array()) {
      return type::array;
    } else if (is_str()) {
      return type::string;
    } else if (is_integer()) {
      return type::integer;
    } else if (is_decimal()) {
      return type::decimal;
    } else if (is_boolean()) {
      return type::boolean;
    } else {
      DART_ASSERT(is_null());
      return type::null;
    }
  }

  template <template <class> class RefCount>
  constexpr bool basic_heap<RefCount>::is_finalized() const noexcept {
    return false;
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::refcount() const noexcept -> size_type {
    if (is_object()) {
      return shim::get<fields_type>(data).use_count();
    } else if (is_array()) {
      return shim::get<elements_type>(data).use_count();
    } else {
      auto* str = shim::get_if<dynamic_string_layout>(&data);
      if (str) return str->ptr.use_count();
      else if (is_null()) return 0;
      else return 1;
    }
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::begin() const -> iterator {
    if (is_object()) {
      auto deref = [] (auto& it) -> auto const& { return it->second; };
      return iterator(detail::dynamic_iterator<RefCount>(try_get_fields()->begin(), deref));
    } else if (is_array()) {
      auto deref = [] (auto& it) -> auto const& { return *it; };
      return iterator(detail::dynamic_iterator<RefCount>(try_get_elements()->begin(), deref));
    } else {
      throw type_error("dart::heap isn't an aggregate and cannot be iterated over");
    }
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::cbegin() const -> iterator {
    return begin();
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::end() const -> iterator {
    if (is_object()) {
      auto deref = [] (auto& it) -> auto const& { return it->second; };
      return iterator(detail::dynamic_iterator<RefCount>(try_get_fields()->end(), deref));
    } else if (is_array()) {
      auto deref = [] (auto& it) -> auto const& { return *it; };
      return iterator(detail::dynamic_iterator<RefCount>(try_get_elements()->end(), deref));
    } else {
      throw type_error("dart::heap isn't an aggregate and cannot be iterated over");
    }
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::cend() const -> iterator {
    return end();
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::rbegin() const -> reverse_iterator {
    return reverse_iterator {end()};
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::rend() const -> reverse_iterator {
    return reverse_iterator {begin()};
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::key_begin() const -> iterator {
    if (is_object()) {
      auto deref = [] (auto& it) -> auto const& { return it->first; };
      return iterator(detail::dynamic_iterator<RefCount>(try_get_fields()->begin(), deref));
    } else {
      throw type_error("dart::heap is not an object and cannot iterate over keys");
    }
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::rkey_begin() const -> reverse_iterator {
    return reverse_iterator {key_end()};
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::key_end() const -> iterator {
    if (is_object()) {
      auto deref = [] (auto& it) -> auto const& { return it->first; };
      return iterator(detail::dynamic_iterator<RefCount>(try_get_fields()->end(), deref));
    } else {
      throw type_error("dart::heap is not an object and cannot iterate over keys");
    }
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::rkey_end() const -> reverse_iterator {
    return reverse_iterator {key_begin()};
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::kvbegin() const -> std::tuple<iterator, iterator> {
    return std::tuple<iterator, iterator> {key_begin(), begin()};
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::kvend() const -> std::tuple<iterator, iterator> {
    return std::tuple<iterator, iterator> {key_end(), end()};
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::rkvbegin() const -> std::tuple<reverse_iterator, reverse_iterator> {
    return std::tuple<reverse_iterator, reverse_iterator> {rkey_begin(), rbegin()};
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::rkvend() const -> std::tuple<reverse_iterator, reverse_iterator> {
    return std::tuple<reverse_iterator, reverse_iterator> {rkey_end(), rend()};
  }

  template <template <class> class RefCount>
  constexpr bool basic_heap<RefCount>::is_view() const noexcept {
    return !refcount::is_owner<RefCount>::value;
  }

  template <template <class> class RefCount>
  template <bool enabled, class EnableIf>
  auto basic_heap<RefCount>::as_owner() const noexcept {
    return refcount::owner_indirection_t<dart::basic_heap, RefCount> {detail::view_tag {}, data};
  }

}

#endif
