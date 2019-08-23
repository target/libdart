#ifndef DART_PACKET_OBJECT_H
#define DART_PACKET_OBJECT_H

/*----- Project Includes -----*/

#include "../common.h"

/*----- Function Implementations -----*/

namespace dart {

  template <template <class> class RefCount>
  template <class... Args, class>
  basic_packet<RefCount> basic_packet<RefCount>::make_object(Args&&... pairs) {
    return basic_heap<RefCount>::make_object(std::forward<Args>(pairs)...);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount> basic_packet<RefCount>::make_object(gsl::span<basic_heap<RefCount> const> pairs) {
    return basic_heap<RefCount>::make_object(pairs);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount> basic_packet<RefCount>::make_object(gsl::span<basic_buffer<RefCount> const> pairs) {
    return basic_heap<RefCount>::make_object(pairs);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount> basic_packet<RefCount>::make_object(gsl::span<basic_packet const> pairs) {
    return basic_heap<RefCount>::make_object(pairs);
  }

  template <template <class> class RefCount>
  template <class KeyType, class ValueType, class>
  basic_packet<RefCount>& basic_packet<RefCount>::add_field(KeyType&& key, ValueType&& value) & {
    get_heap().add_field(std::forward<KeyType>(key), std::forward<ValueType>(value));
    return *this;
  }

  template <template <class> class RefCount>
  template <class KeyType, class ValueType, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::add_field(KeyType&& key, ValueType&& value) && {
    add_field(std::forward<KeyType>(key), std::forward<ValueType>(value));
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>& basic_packet<RefCount>::remove_field(shim::string_view key) & {
    erase(key);
    return *this;
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::remove_field(shim::string_view key) && {
    remove_field(key);
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_packet<RefCount>& basic_packet<RefCount>::remove_field(KeyType const& key) & {
    erase(key.strv());
    return *this;
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::remove_field(KeyType const& key) && {
    remove_field(key);
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <class String, template <class> class, class>
  auto basic_packet<RefCount>::erase(basic_string<String> const& key) -> iterator {
    return erase(key.strv());
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  auto basic_packet<RefCount>::erase(shim::string_view key) -> iterator {
    return get_heap().erase(key);
  }

  template <template <class> class RefCount>
  template <class... Args, class>
  basic_packet<RefCount> basic_packet<RefCount>::inject(Args&&... pairs) const {
    return shim::visit([&] (auto& v) -> basic_packet { return v.inject(std::forward<Args>(pairs)...); }, impl);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount> basic_packet<RefCount>::inject(gsl::span<basic_heap<RefCount> const> pairs) const {
    return shim::visit([&] (auto& v) -> basic_packet { return v.inject(pairs); }, impl);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount> basic_packet<RefCount>::inject(gsl::span<basic_buffer<RefCount> const> pairs) const {
    return shim::visit([&] (auto& v) -> basic_packet { return v.inject(pairs); }, impl);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount> basic_packet<RefCount>::inject(gsl::span<basic_packet const> pairs) const {
    return shim::visit([&] (auto& v) -> basic_packet { return v.inject(pairs); }, impl);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount> basic_packet<RefCount>::project(std::initializer_list<shim::string_view> keys) const {
    return shim::visit([&] (auto& v) -> basic_packet { return v.project(keys); }, impl);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount> basic_packet<RefCount>::project(gsl::span<std::string const> keys) const {
    return shim::visit([&] (auto& v) -> basic_packet { return v.project(keys); }, impl);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount> basic_packet<RefCount>::project(gsl::span<shim::string_view const> keys) const {
    return shim::visit([&] (auto& v) -> basic_packet { return v.project(keys); }, impl);
  }

  template <template <class> class RefCount>
  template <class String>
  basic_packet<RefCount> basic_packet<RefCount>::operator [](basic_string<String> const& key) const& {
    return (*this)[key.strv()];
  }

  template <template <class> class RefCount>
  template <class String, template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::operator [](basic_string<String> const& key) && {
    return std::move(*this)[key.strv()];
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::operator [](shim::string_view key) const& {
    return get(key);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::operator [](shim::string_view key) && {
    return std::move(*this).get(key);
  }

  template <template <class> class RefCount>
  template <class String>
  basic_packet<RefCount> basic_packet<RefCount>::get(basic_string<String> const& key) const& {
    return get(key.strv());
  }

  template <template <class> class RefCount>
  template <class String, template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::get(basic_string<String> const& key) && {
    return std::move(*this).get(key.strv());
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::get(shim::string_view key) const& {
    return shim::visit([&] (auto& v) -> basic_packet { return v.get(key); }, impl);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::get(shim::string_view key) && {
    shim::visit([&] (auto& v) { v = std::move(v).get(key); }, impl);
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <class String, class T, class>
  basic_packet<RefCount> basic_packet<RefCount>::get_or(basic_string<String> const& key, T&& opt) const {
    return get_or(key.strv(), std::forward<T>(opt));
  }

  template <template <class> class RefCount>
  template <class T, class>
  basic_packet<RefCount> basic_packet<RefCount>::get_or(shim::string_view key, T&& opt) const {
    if (is_object() && has_key(key)) return get(key);
    else return convert::cast<basic_packet>(std::forward<T>(opt));
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::get_nested(shim::string_view path, char separator) const {
    return shim::visit([&] (auto& v) -> basic_packet { return v.get_nested(path, separator); }, impl);
  }

  template <template <class> class RefCount>
  template <class String>
  basic_packet<RefCount> basic_packet<RefCount>::at(basic_string<String> const& key) const& {
    return at(key.strv());
  }

  template <template <class> class RefCount>
  template <class String, template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::at(basic_string<String> const& key) && {
    return std::move(*this).at(key.strv());
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::at(shim::string_view key) const& {
    return shim::visit([&] (auto& v) -> basic_packet { return v.at(key); }, impl);
  }

  template <template <class> class RefCount>
  template <template <class> class, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::at(shim::string_view key) && {
    shim::visit([&] (auto& v) { v = std::move(v).at(key); }, impl);
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <class String>
  auto basic_packet<RefCount>::find(basic_string<String> const& key) const -> iterator {
    return find(key.strv());
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::find(shim::string_view key) const -> iterator {
    return shim::visit([&] (auto& v) -> iterator { return v.find(key); }, impl);
  }

  template <template <class> class RefCount>
  template <class String>
  auto basic_packet<RefCount>::find_key(basic_string<String> const& key) const -> iterator {
    return find_key(key.strv());
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::find_key(shim::string_view key) const -> iterator {
    return shim::visit([&] (auto& v) -> iterator { return v.find_key(key); }, impl);
  }

  template <template <class> class RefCount>
  std::vector<basic_packet<RefCount>> basic_packet<RefCount>::keys() const {
    std::vector<basic_packet> packets;
    packets.reserve(size());
    shim::visit([&] (auto& v) {
      for (auto& k : v.keys()) packets.push_back(std::move(k));
    }, impl);
    return packets;
  }

  template <template <class> class RefCount>
  template <class String>
  bool basic_packet<RefCount>::has_key(basic_string<String> const& key) const {
    return has_key(key.strv());
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::has_key(shim::string_view key) const {
    return shim::visit([&] (auto& v) { return v.has_key(key); }, impl);
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  bool basic_packet<RefCount>::has_key(KeyType const& key) const {
    if (key.get_type() == type::string) return has_key(key.strv());
    else return false;
  }

}

#endif
