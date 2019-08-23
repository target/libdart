#ifndef DART_HEAP_OBJECT_H
#define DART_HEAP_OBJECT_H

/*----- Project Includes -----*/

#include "../common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <template <class> class RefCount>
  template <class... Args, class>
  basic_heap<RefCount> basic_heap<RefCount>::make_object(Args&&... pairs) {
    auto obj = basic_heap(detail::object_tag {});
    convert::as_span<basic_heap>([&] (auto args) {
      // I don't know why this needs to be qualified,
      // but some older versions of gcc get weird without it.
      basic_heap::inject_pairs<true>(obj, args); 
    }, std::forward<Args>(pairs)...);
    return obj;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount> basic_heap<RefCount>::make_object(gsl::span<basic_heap const> pairs) {
    auto obj = basic_heap(detail::object_tag {});
    inject_pairs<false>(obj, pairs);
    return obj;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount> basic_heap<RefCount>::make_object(gsl::span<basic_buffer<RefCount> const> pairs) {
    auto obj = basic_heap(detail::object_tag {});
    inject_pairs<false>(obj, pairs);
    return obj;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount> basic_heap<RefCount>::make_object(gsl::span<basic_packet<RefCount> const> pairs) {
    auto obj = basic_heap(detail::object_tag {});
    inject_pairs<false>(obj, pairs);
    return obj;
  }

  template <template <class> class RefCount>
  template <class KeyType, class ValueType, class>
  basic_heap<RefCount>& basic_heap<RefCount>::add_field(KeyType&& key, ValueType&& value) & {
    insert(std::forward<KeyType>(key), std::forward<ValueType>(value));
    return *this;
  }

  template <template <class> class RefCount>
  template <class KeyType, class ValueType, class>
  basic_heap<RefCount>&& basic_heap<RefCount>::add_field(KeyType&& key, ValueType&& value) && {
    add_field(std::forward<KeyType>(key), std::forward<ValueType>(value));
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount>& basic_heap<RefCount>::remove_field(shim::string_view key) & {
    erase(key);
    return *this;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount>&& basic_heap<RefCount>::remove_field(shim::string_view key) && {
    remove_field(key);
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_heap<RefCount>& basic_heap<RefCount>::remove_field(KeyType const& key) & {
    erase(key.strv());
    return *this;
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_heap<RefCount>&& basic_heap<RefCount>::remove_field(KeyType const& key) && {
    remove_field(key);
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <class String, template <class> class, class>
  auto basic_heap<RefCount>::erase(basic_string<String> const& key) -> iterator {
    return erase(key.strv());
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  auto basic_heap<RefCount>::erase(shim::string_view key) -> iterator {
    return erase_key_impl(key, [] (auto& it) -> auto const& { return it->second; }, nullptr);
  }

  template <template <class> class RefCount>
  template <class... Args, class>
  basic_heap<RefCount> basic_heap<RefCount>::inject(Args&&... pairs) const {
    // Hand off to our implementation.
    auto obj {*this};
    convert::as_span<basic_heap>([&] (auto span) {
      // I don't know why this needs to be qualified,
      // but some older versions of gcc get weird without it.
      basic_heap::inject_pairs<true>(obj, span);
    }, std::forward<Args>(pairs)...);
    return obj;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount> basic_heap<RefCount>::inject(gsl::span<basic_heap const> pairs) const {
    auto obj {*this};
    inject_pairs<false>(obj, pairs);
    return obj;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount> basic_heap<RefCount>::inject(gsl::span<basic_buffer<RefCount> const> pairs) const {
    auto obj {*this};
    inject_pairs<false>(obj, pairs);
    return obj;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount> basic_heap<RefCount>::inject(gsl::span<basic_packet<RefCount> const> pairs) const {
    auto obj {*this};
    inject_pairs<false>(obj, pairs);
    return obj;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount> basic_heap<RefCount>::project(std::initializer_list<shim::string_view> keys) const {
    return project_keys(keys);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount> basic_heap<RefCount>::project(gsl::span<std::string const> keys) const {
    return project_keys(keys);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_heap<RefCount> basic_heap<RefCount>::project(gsl::span<shim::string_view const> keys) const {
    return project_keys(keys);
  }

  template <template <class> class RefCount>
  template <class String>
  basic_heap<RefCount> basic_heap<RefCount>::operator [](basic_string<String> const& key) const {
    return (*this)[key.strv()];
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::operator [](shim::string_view key) const {
    return get(key);
  }

  template <template <class> class RefCount>
  template <class String>
  basic_heap<RefCount> basic_heap<RefCount>::get(basic_string<String> const& key) const {
    return get(key.strv());
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::get(shim::string_view key) const {
    // Use our transparent comparator to find the pair without constructing
    // a temporary heap.
    auto& fields = get_fields();
    auto found = fields.find(key);
    if (found == fields.end()) return basic_heap::make_null();
    else return found->second;
  }

  template <template <class> class RefCount>
  template <class String, class T, class>
  basic_heap<RefCount> basic_heap<RefCount>::get_or(basic_string<String> const& key, T&& opt) const {
    return get_or(key.strv(), std::forward<T>(opt));
  }

  template <template <class> class RefCount>
  template <class T, class>
  basic_heap<RefCount> basic_heap<RefCount>::get_or(shim::string_view key, T&& opt) const {
    if (is_object() && has_key(key)) return get(key);
    else return convert::cast<basic_heap>(std::forward<T>(opt));
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::get_nested(shim::string_view path, char separator) const {
    return detail::get_nested_impl(*this, path, separator);
  }

  template <template <class> class RefCount>
  template <class String>
  basic_heap<RefCount> basic_heap<RefCount>::at(basic_string<String> const& key) const {
    return at(key.strv());
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::at(shim::string_view key) const {
    // Use our transparent comparator to find the pair without constructing
    // a temporary heap.
    auto& fields = get_fields();
    auto found = fields.find(key);
    if (found == fields.end()) throw std::out_of_range("dart::heap does not contain requested mapping");
    return found->second;
  }

  template <template <class> class RefCount>
  template <class String>
  auto basic_heap<RefCount>::find(basic_string<String> const& key) const -> iterator {
    return find(key.strv());
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::find(shim::string_view key) const -> iterator {
    if (is_object()) {
      auto deref = [] (auto& it) -> auto const& { return it->second; };
      return iterator(detail::dynamic_iterator<RefCount>(try_get_fields()->find(key), deref));
    } else {
      throw type_error("dart::heap isn't an object and cannot find key-value mappings");
    }
  }

  template <template <class> class RefCount>
  template <class String>
  auto basic_heap<RefCount>::find_key(basic_string<String> const& key) const -> iterator {
    return find_key(key.strv());
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::find_key(shim::string_view key) const -> iterator {
    if (is_object()) {
      auto deref = [] (auto& it) -> auto const& { return it->first; };
      return iterator(detail::dynamic_iterator<RefCount>(try_get_fields()->find(key), deref));
    } else {
      throw type_error("dart::heap isn't an object and cannot find key-value mappings");
    }
  }

  template <template <class> class RefCount>
  std::vector<basic_heap<RefCount>> basic_heap<RefCount>::keys() const {
    return detail::keys_impl(*this);
  }

  template <template <class> class RefCount>
  template <class String>
  bool basic_heap<RefCount>::has_key(basic_string<String> const& key) const {
    return has_key(key.strv());
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::has_key(shim::string_view key) const {
    auto& fields = get_fields();
    return fields.find(key) != fields.end();
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  bool basic_heap<RefCount>::has_key(KeyType const& key) const {
    if (key.get_type() == type::string) return has_key(key.strv());
    else return false;
  }

  template <template <class> class RefCount>
  template <bool consume, class Span>
  void basic_heap<RefCount>::inject_pairs(basic_heap& obj, Span pairs) {
    // Validate that what we're being asked to do makes sense.
    if (!obj.is_object()) throw type_error("dart::heap is not an object and cannot inject key-value pairs");

    // Insert each of the requested pairs.
    auto end = std::end(pairs);
    auto it = std::begin(pairs);
    while (it != end) {
      auto& k = *it++;
      auto& v = *it++;
      if (consume) obj.insert(std::move(k), std::move(v));
      else obj.insert(k, v);
    }
  }

  template <template <class> class RefCount>
  template <class Spannable>
  basic_heap<RefCount> basic_heap<RefCount>::project_keys(Spannable const& keys) const {
    // Validate that what we're being asked to do makes sense.
    if (!is_object()) throw type_error("dart::heap is not an object an cannot project keys");

    // Iterate and pull out.
    auto obj = make_object();
    for (auto& key : keys) if (has_key(key)) obj.add_field(key, get(key));
    return obj;
  }

  template <template <class> class RefCount>
  template <class Deref>
  auto basic_heap<RefCount>::erase_key_impl(shim::string_view key, Deref&& deref, fields_type safeguard)
    -> iterator
  {
    // Make sure we copy out if our heap is shared.
    copy_on_write(safeguard ? 2 : 1);

    // Try to locate the key.
    auto& fields = get_fields();
    auto new_it = fields.find(key);

    // No longer need to access the key, release the safeguard.
    safeguard.reset();

    // Erase if we found it.
    if (new_it != fields.end()) new_it = fields.erase(new_it);
    return detail::dynamic_iterator<RefCount> {new_it, std::forward<Deref>(deref)};
  }

  template <template <class> class RefCount>
  shim::string_view basic_heap<RefCount>::iterator_key(iterator pos) const {
    using fields_layout = typename detail::dynamic_iterator<RefCount>::fields_layout;
    return shim::visit(
      shim::compose_together(
        [] (fields_type const&, fields_layout& layout) -> shim::string_view {
          return layout.it->first.strv();
        },
        [] (elements_type const&, auto&) -> shim::string_view {
          throw type_error("dart::heap is an array, and cannot perform object operations");
        },
        [] (auto&, auto&) -> shim::string_view {
          throw type_error("dart::heap is not an object, "
            "or was provided an invalid iterator, and cannot perform object operations");
        }
      ),
      data,
      pos.impl->impl
    );
  }

}

#endif
