#ifndef DART_BUFFER_OBJECT_H
#define DART_BUFFER_OBJECT_H

/*----- Project Includes -----*/

#include "../common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <template <class> class RefCount>
  template <class... Args, class>
  basic_buffer<RefCount> basic_buffer<RefCount>::make_object(Args&&... pairs) {
    // XXX: This HAS to check the parameter pack size using Args instead of pairs
    // because using pairs segfaults gcc 5.1.0 for some reason
    using pair_storage = std::array<detail::basic_pair<basic_packet<RefCount>>, sizeof...(Args) / 2>;
    return convert::as_span<basic_packet<RefCount>>([] (auto args) {
      // Group each key-value pair together, and create more temporary storage so that
      // all other logic can work with pointers to pairs.
      pair_storage storage;
      for (auto i = 0U; i < static_cast<size_t>(args.size()); i += 2) {
        storage[i / 2] = {std::move(args[i]), std::move(args[i + 1])};
      }

      // Sort the pairs and hand off to the low level code.
      return detail::buffer_builder<RefCount>::build_buffer(gsl::make_span(storage));
    }, std::forward<Args>(pairs)...);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_buffer<RefCount> basic_buffer<RefCount>::make_object(gsl::span<basic_heap<RefCount> const> pairs) {
    return dynamic_make_object(pairs);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_buffer<RefCount> basic_buffer<RefCount>::make_object(gsl::span<basic_buffer const> pairs) {
    return dynamic_make_object(pairs);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_buffer<RefCount> basic_buffer<RefCount>::make_object(gsl::span<basic_packet<RefCount> const> pairs) {
    return dynamic_make_object(pairs);
  }

  template <template <class> class RefCount>
  template <class... Args, class>
  basic_buffer<RefCount> basic_buffer<RefCount>::inject(Args&&... pairs) const {
    // Forward the given arguments into a temporary object.
    auto tmp = make_object(std::forward<Args>(pairs)...);

    // Hand off to our implementation.
    return detail::buffer_builder<RefCount>::merge_buffers(*this, tmp);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_buffer<RefCount> basic_buffer<RefCount>::inject(gsl::span<basic_heap<RefCount> const> pairs) const {
    return detail::buffer_builder<RefCount>::merge_buffers(*this, make_object(pairs));
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_buffer<RefCount> basic_buffer<RefCount>::inject(gsl::span<basic_buffer const> pairs) const {
    return detail::buffer_builder<RefCount>::merge_buffers(*this, make_object(pairs));
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_buffer<RefCount> basic_buffer<RefCount>::inject(gsl::span<basic_packet<RefCount> const> pairs) const {
    return detail::buffer_builder<RefCount>::merge_buffers(*this, make_object(pairs));
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_buffer<RefCount> basic_buffer<RefCount>::project(std::initializer_list<shim::string_view> keys) const {
    return detail::buffer_builder<RefCount>::project_keys(*this, keys);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_buffer<RefCount> basic_buffer<RefCount>::project(gsl::span<std::string const> keys) const {
    return detail::buffer_builder<RefCount>::project_keys(*this, keys);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_buffer<RefCount> basic_buffer<RefCount>::project(gsl::span<shim::string_view const> keys) const {
    return detail::buffer_builder<RefCount>::project_keys(*this, keys);
  }

  template <template <class> class RefCount>
  template <class String>
  basic_buffer<RefCount> basic_buffer<RefCount>::operator [](basic_string<String> const& key) const& {
    return (*this)[key.strv()];
  }

  template <template <class> class RefCount>
  template <class String, template <class> class, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::operator [](basic_string<String> const& key) && {
    return std::move(*this)[key.strv()];
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> basic_buffer<RefCount>::operator [](shim::string_view key) const& {
    return get(key);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::operator [](shim::string_view key) && {
    return std::move(*this).get(key);
  }

  template <template <class> class RefCount>
  template <class String>
  basic_buffer<RefCount> basic_buffer<RefCount>::get(basic_string<String> const& key) const& {
    return get(key.strv());
  }

  template <template <class> class RefCount>
  template <class String, template <class> class, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::get(basic_string<String> const& key) && {
    return std::move(*this).get(key.strv());
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> basic_buffer<RefCount>::get(shim::string_view key) const& {
    return basic_buffer(detail::get_object<RefCount>(raw)->get_value(key), buffer_ref);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::get(shim::string_view key) && {
    raw = detail::get_object<RefCount>(raw)->get_value(key);
    if (is_null()) buffer_ref = nullptr;
    return std::move(*this);
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> basic_buffer<RefCount>::get_nested(shim::string_view path, char separator) const {
    return detail::get_nested_impl(*this, path, separator);
  }

  template <template <class> class RefCount>
  template <class String>
  basic_buffer<RefCount> basic_buffer<RefCount>::at(basic_string<String> const& key) const& {
    return at(key.strv());
  }

  template <template <class> class RefCount>
  template <class String, template <class> class, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::at(basic_string<String> const& key) && {
    return std::move(*this).at(key.strv());
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> basic_buffer<RefCount>::at(shim::string_view key) const& {
    return basic_buffer(detail::get_object<RefCount>(raw)->at_value(key), buffer_ref);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::at(shim::string_view key) && {
    raw = detail::get_object<RefCount>(raw)->at_value(key);
    if (is_null()) buffer_ref = nullptr;
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <class String>
  auto basic_buffer<RefCount>::find(basic_string<String> const& key) const -> iterator {
    return find(key.strv());
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::find(shim::string_view key) const -> iterator {
    return iterator(*this, detail::get_object<RefCount>(raw)->get_it(key));
  }

  template <template <class> class RefCount>
  template <class String>
  auto basic_buffer<RefCount>::find_key(basic_string<String> const& key) const -> iterator {
    return find_key(key.strv());
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::find_key(shim::string_view key) const -> iterator {
    return iterator(*this, detail::get_object<RefCount>(raw)->get_key_it(key));
  }

  template <template <class> class RefCount>
  std::vector<basic_buffer<RefCount>> basic_buffer<RefCount>::keys() const {
    return detail::keys_impl(*this);
  }

  template <template <class> class RefCount>
  template <class String>
  bool basic_buffer<RefCount>::has_key(basic_string<String> const& key) const {
    return has_key(key.strv());
  }

  template <template <class> class RefCount>
  bool basic_buffer<RefCount>::has_key(shim::string_view key) const {
    auto elem = detail::get_object<RefCount>(raw)->get_key(key, [] (auto) {});
    return elem.buffer != nullptr;
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  bool basic_buffer<RefCount>::has_key(KeyType const& key) const {
    if (key.get_type() == type::string) return has_key(key.strv());
    else return false;
  }

}

#endif
