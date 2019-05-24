#ifndef DART_OBJ_IMPL_H
#define DART_OBJ_IMPL_H

/*----- Local Includes -----*/

#include "dart_intern.h"

/*----- Function Implementations -----*/

namespace dart {

  namespace detail {

    template <class Packet>
    Packet get_nested_impl(Packet haystack, shim::string_view needle, char separator) {
      // Spin through the provided needle until we reach the end, or hit a leaf.
      auto start = needle.begin();
      while (start < needle.end() && haystack.is_object()) {
        // Tokenize up the needle and drag the current packet through each one.
        auto stop = std::find(start, needle.end(), separator);
        haystack = std::move(haystack)[needle.substr(start - needle.begin(), stop - start)];

        // Prepare for next iteration.
        stop == needle.end() ? start = stop : start = stop + 1;
      }

      // If we finished, find our final value, otherwise return null.
      return start >= needle.end() ? haystack : Packet::make_null();
    }

    template <class Packet>
    std::vector<Packet> keys_impl(Packet const& that) {
      std::vector<Packet> packets;
      packets.reserve(that.size());
      for (auto it = that.key_begin(); it != that.key_end(); ++it) {
        packets.push_back(*it);
      }
      return packets;
    }

  }

  template <class Object>
  template <class Arg,
    std::enable_if_t<
      !meta::is_specialization_of<
        Arg,
        dart::basic_object
      >::value
      &&
      !std::is_convertible<
        Arg,
        Object
      >::value
      &&
      std::is_constructible<
        Object,
        Arg
      >::value
    >*
  >
  basic_object<Object>::basic_object(Arg&& arg) : val(std::forward<Arg>(arg)) {
    ensure_object("dart::packet::object can only be constructed as an object");
  }

  template <class Object>
  template <class Arg,
    std::enable_if_t<
      !meta::is_specialization_of<
        Arg,
        dart::basic_object
      >::value
      &&
      std::is_convertible<
        Arg,
        Object
      >::value
    >*
  >
  basic_object<Object>::basic_object(Arg&& arg) : val(std::forward<Arg>(arg)) {
    ensure_object("dart::packet::object can only be constructed as an object");
  }

  template <template <class> class RefCount>
  template <class... Args, class>
  basic_heap<RefCount> basic_heap<RefCount>::make_object(Args&&... pairs) {
    return basic_heap(detail::object_tag {}, std::forward<Args>(pairs)...);
  }

  template <template <class> class RefCount>
  template <class... Args, class>
  basic_buffer<RefCount> basic_buffer<RefCount>::make_object(Args&&... pairs) {
    return basic_buffer {basic_heap<RefCount>::make_object(std::forward<Args>(pairs)...)};
  }

  template <template <class> class RefCount>
  template <class... Args, class>
  basic_packet<RefCount> basic_packet<RefCount>::make_object(Args&&... pairs) {
    return basic_packet(detail::object_tag {}, std::forward<Args>(pairs)...);
  }

  template <class Object>
  template <class KeyType, class ValueType, class>
  basic_object<Object>& basic_object<Object>::add_field(KeyType&& key, ValueType&& value) & {
    val.add_field(std::forward<KeyType>(key), std::forward<ValueType>(value));
    return *this;
  }

  template <template <class> class RefCount>
  template <class KeyType, class ValueType, class>
  basic_heap<RefCount>& basic_heap<RefCount>::add_field(KeyType&& key, ValueType&& value) & {
    insert(std::forward<KeyType>(key), std::forward<ValueType>(value));
    return *this;
  }

  template <template <class> class RefCount>
  template <class KeyType, class ValueType, class>
  basic_packet<RefCount>& basic_packet<RefCount>::add_field(KeyType&& key, ValueType&& value) & {
    get_heap().add_field(std::forward<KeyType>(key), std::forward<ValueType>(value));
    return *this;
  }

  template <class Object>
  template <class KeyType, class ValueType, class>
  basic_object<Object>&& basic_object<Object>::add_field(KeyType&& key, ValueType&& value) && {
    auto&& tmp = std::move(val).add_field(std::forward<KeyType>(key), std::forward<ValueType>(value));
    (void) tmp;
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <class KeyType, class ValueType, class>
  basic_heap<RefCount>&& basic_heap<RefCount>::add_field(KeyType&& key, ValueType&& value) && {
    add_field(std::forward<KeyType>(key), std::forward<ValueType>(value));
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <class KeyType, class ValueType, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::add_field(KeyType&& key, ValueType&& value) && {
    add_field(std::forward<KeyType>(key), std::forward<ValueType>(value));
    return std::move(*this);
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>& basic_heap<RefCount>::remove_field(shim::string_view key) & {
    erase(key);
    return *this;
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>& basic_packet<RefCount>::remove_field(shim::string_view key) & {
    erase(key);
    return *this;
  }

  template <template <class> class RefCount>
  basic_heap<RefCount>&& basic_heap<RefCount>::remove_field(shim::string_view key) && {
    remove_field(key);
    return std::move(*this);
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>&& basic_packet<RefCount>::remove_field(shim::string_view key) && {
    remove_field(key);
    return std::move(*this);
  }

  template <class Object>
  template <class KeyType, class>
  basic_object<Object>& basic_object<Object>::remove_field(KeyType const& key) & {
    val.remove_field(key);
    return *this;
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_heap<RefCount>& basic_heap<RefCount>::remove_field(KeyType const& key) & {
    erase(key.strv());
    return *this;
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_packet<RefCount>& basic_packet<RefCount>::remove_field(KeyType const& key) & {
    erase(key.strv());
    return *this;
  }

  template <class Object>
  template <class KeyType, class>
  basic_object<Object>&& basic_object<Object>::remove_field(KeyType const& key) && {
    auto&& tmp = std::move(val).remove_field(key);
    (void) tmp;
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_heap<RefCount>&& basic_heap<RefCount>::remove_field(KeyType const& key) && {
    remove_field(key);
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  basic_packet<RefCount>&& basic_packet<RefCount>::remove_field(KeyType const& key) && {
    remove_field(key);
    return std::move(*this);
  }

  template <class Object>
  template <class KeyType, class ValueType, class>
  auto basic_object<Object>::insert(KeyType&& key, ValueType&& value) -> iterator {
    return val.insert(std::forward<KeyType>(key), std::forward<ValueType>(value));
  }

  template <class Object>
  template <class KeyType, class ValueType, class>
  auto basic_object<Object>::set(KeyType&& key, ValueType&& value) -> iterator {
    return val.set(std::forward<KeyType>(key), std::forward<ValueType>(value));
  }

  template <class Object>
  template <class KeyType, class>
  auto basic_object<Object>::erase(KeyType const& key) -> iterator {
    return val.erase(key);
  }

  template <template <class> class RefCount>
  template <class String>
  auto basic_heap<RefCount>::erase(basic_string<String> const& key) -> iterator {
    return erase(key.strv());
  }

  template <template <class> class RefCount>
  template <class String>
  auto basic_packet<RefCount>::erase(basic_string<String> const& key) -> iterator {
    return erase(key.strv());
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::erase(shim::string_view key) -> iterator {
    return erase_key_impl(key, [] (auto& it) { return it->second; }, nullptr);
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::erase(shim::string_view key) -> iterator {
    return get_heap().erase(key);
  }

  template <class Object>
  template <class KeyType, class>
  auto basic_object<Object>::operator [](KeyType const& key) const& -> value_type {
    return val[key];
  }

  template <class Object>
  template <class KeyType, class>
  decltype(auto) basic_object<Object>::operator [](KeyType const& key) && {
    return std::move(val)[key];
  }

  template <template <class> class RefCount>
  template <class String>
  basic_heap<RefCount> basic_heap<RefCount>::operator [](basic_string<String> const& key) const {
    return (*this)[key.strv()];
  }

  template <template <class> class RefCount>
  template <class String>
  basic_buffer<RefCount> basic_buffer<RefCount>::operator [](basic_string<String> const& key) const& {
    return (*this)[key.strv()];
  }

  template <template <class> class RefCount>
  template <class String>
  basic_packet<RefCount> basic_packet<RefCount>::operator [](basic_string<String> const& key) const& {
    return (*this)[key.strv()];
  }

  template <template <class> class RefCount>
  template <class String>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::operator [](basic_string<String> const& key) && {
    return std::move(*this)[key.strv()];
  }

  template <template <class> class RefCount>
  template <class String>
  basic_packet<RefCount>&& basic_packet<RefCount>::operator [](basic_string<String> const& key) && {
    return std::move(*this)[key.strv()];
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::operator [](shim::string_view key) const {
    return get(key);
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> basic_buffer<RefCount>::operator [](shim::string_view key) const& {
    return get(key);
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::operator [](shim::string_view key) const& {
    return get(key);
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::operator [](shim::string_view key) && {
    return std::move(*this).get(key);
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>&& basic_packet<RefCount>::operator [](shim::string_view key) && {
    return std::move(*this).get(key);
  }

  template <class Object>
  template <class KeyType, class>
  auto basic_object<Object>::get(KeyType const& key) const& -> value_type {
    return val.get(key);
  }

  template <template <class> class RefCount>
  template <class String>
  basic_heap<RefCount> basic_heap<RefCount>::get(basic_string<String> const& key) const {
    return get(key.strv());
  }

  template <template <class> class RefCount>
  template <class String>
  basic_buffer<RefCount> basic_buffer<RefCount>::get(basic_string<String> const& key) const& {
    return get(key.strv());
  }

  template <class Object>
  template <class KeyType, class>
  decltype(auto) basic_object<Object>::get(KeyType const& key) && {
    return std::move(val).get(key);
  }

  template <template <class> class RefCount>
  template <class String>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::get(basic_string<String> const& key) && {
    return std::move(*this).get(key.strv());
  }

  template <template <class> class RefCount>
  template <class String>
  basic_packet<RefCount> basic_packet<RefCount>::get(basic_string<String> const& key) const& {
    return get(key.strv());
  }

  template <template <class> class RefCount>
  template <class String>
  basic_packet<RefCount>&& basic_packet<RefCount>::get(basic_string<String> const& key) && {
    return std::move(*this).get(key.strv());
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::get(shim::string_view key) const {
    // Use our transparent comparator to find the pair without constructing
    // a temporary heap.
    auto& fields = get_fields();
    auto found = fields.find(key);
    return found != fields.end() ? found->second : basic_heap::null();
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> basic_buffer<RefCount>::get(shim::string_view key) const& {
    return basic_buffer(detail::get_object<RefCount>(raw)->get_value(key), buffer_ref);
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::get(shim::string_view key) const& {
    return shim::visit([&] (auto& v) -> basic_packet { return v.get(key); }, impl);
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::get(shim::string_view key) && {
    raw = detail::get_object<RefCount>(raw)->get_value(key);
    if (is_null()) buffer_ref = nullptr;
    return std::move(*this);
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>&& basic_packet<RefCount>::get(shim::string_view key) && {
    shim::visit([&] (auto& v) { v = std::move(v).get(key); }, impl);
    return std::move(*this);
  }

  template <template <class> class RefCount>
  template <class String, class T, class>
  basic_heap<RefCount> basic_heap<RefCount>::get_or(basic_string<String> const& key, T&& opt) const {
    return get_or(key.strv(), std::forward<T>(opt));
  }

  template <template <class> class RefCount>
  template <class String, class T, class>
  basic_packet<RefCount> basic_packet<RefCount>::get_or(basic_string<String> const& key, T&& opt) const {
    return get_or(key.strv(), std::forward<T>(opt));
  }

  template <template <class> class RefCount>
  template <class T, class>
  basic_heap<RefCount> basic_heap<RefCount>::get_or(shim::string_view key, T&& opt) const {
    if (is_object() && has_key(key)) return get(key);
    else return convert::cast<basic_heap>(std::forward<T>(opt));
  }

  template <template <class> class RefCount>
  template <class T, class>
  basic_packet<RefCount> basic_packet<RefCount>::get_or(shim::string_view key, T&& opt) const {
    if (is_object() && has_key(key)) return get(key);
    else return convert::cast<basic_packet>(std::forward<T>(opt));
  }

  template <class Object>
  template <class KeyType, class T, class>
  auto basic_object<Object>::get_or(KeyType const& key, T&& opt) const -> value_type {
    return val.get_or(key, std::forward<T>(opt));
  }

  template <class Object>
  auto basic_object<Object>::get_nested(shim::string_view path, char separator) const -> value_type {
    return val.get_nested(path, separator);
  }

  template <template <class> class RefCount>
  basic_heap<RefCount> basic_heap<RefCount>::get_nested(shim::string_view path, char separator) const {
    return detail::get_nested_impl(*this, path, separator);
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount> basic_buffer<RefCount>::get_nested(shim::string_view path, char separator) const {
    return detail::get_nested_impl(*this, path, separator);
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::get_nested(shim::string_view path, char separator) const {
    return shim::visit([&] (auto& v) -> basic_packet { return v.get_nested(path, separator); }, impl);
  }

  template <class Object>
  template <class KeyType, class>
  auto basic_object<Object>::at(KeyType const& key) const& -> value_type {
    return val.at(key);
  }

  template <template <class> class RefCount>
  template <class String>
  basic_heap<RefCount> basic_heap<RefCount>::at(basic_string<String> const& key) const {
    return at(key.strv());
  }

  template <template <class> class RefCount>
  template <class String>
  basic_buffer<RefCount> basic_buffer<RefCount>::at(basic_string<String> const& key) const& {
    return at(key.strv());
  }

  template <class Object>
  template <class KeyType, class>
  decltype(auto) basic_object<Object>::at(KeyType const& key) && {
    return std::move(val).at(key);
  }

  template <template <class> class RefCount>
  template <class String>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::at(basic_string<String> const& key) && {
    return std::move(*this).at(key.strv());
  }

  template <template <class> class RefCount>
  template <class String>
  basic_packet<RefCount> basic_packet<RefCount>::at(basic_string<String> const& key) const& {
    return at(key.strv());
  }

  template <template <class> class RefCount>
  template <class String>
  basic_packet<RefCount>&& basic_packet<RefCount>::at(basic_string<String> const& key) && {
    return std::move(*this).at(key.strv());
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
  basic_buffer<RefCount> basic_buffer<RefCount>::at(shim::string_view key) const& {
    return basic_buffer(detail::get_object<RefCount>(raw)->at_value(key), buffer_ref);
  }

  template <template <class> class RefCount>
  basic_packet<RefCount> basic_packet<RefCount>::at(shim::string_view key) const& {
    return shim::visit([&] (auto& v) -> basic_packet { return v.at(key); }, impl);
  }

  template <template <class> class RefCount>
  basic_buffer<RefCount>&& basic_buffer<RefCount>::at(shim::string_view key) && {
    raw = detail::get_object<RefCount>(raw)->at_value(key);
    if (is_null()) buffer_ref = nullptr;
    return std::move(*this);
  }

  template <template <class> class RefCount>
  basic_packet<RefCount>&& basic_packet<RefCount>::at(shim::string_view key) && {
    shim::visit([&] (auto& v) { v = std::move(v).at(key); }, impl);
    return std::move(*this);
  }

  template <class Object>
  template <class KeyType, class>
  auto basic_object<Object>::find(KeyType const& key) const -> iterator {
    return val.find(key);
  }

  template <template <class> class RefCount>
  template <class String>
  auto basic_heap<RefCount>::find(basic_string<String> const& key) const -> iterator {
    return find(key.strv());
  }

  template <template <class> class RefCount>
  template <class String>
  auto basic_buffer<RefCount>::find(basic_string<String> const& key) const -> iterator {
    return find(key.strv());
  }

  template <template <class> class RefCount>
  template <class String>
  auto basic_packet<RefCount>::find(basic_string<String> const& key) const -> iterator {
    return find(key.strv());
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::find(shim::string_view key) const -> iterator {
    if (is_object()) {
      auto deref = [] (auto& it) { return it->second; };
      return iterator(detail::dn_iterator<RefCount>(try_get_fields()->find(key), deref));
    } else {
      throw type_error("dart::heap isn't an object and cannot find key-value mappings");
    }
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::find(shim::string_view key) const -> iterator {
    return iterator(*this, detail::get_object<RefCount>(raw)->get_it(key));
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::find(shim::string_view key) const -> iterator {
    return shim::visit([&] (auto& v) -> iterator { return v.find(key); }, impl);
  }

  template <class Object>
  template <class KeyType, class>
  auto basic_object<Object>::find_key(KeyType const& key) const -> iterator {
    return val.find_key(key);
  }

  template <template <class> class RefCount>
  template <class String>
  auto basic_heap<RefCount>::find_key(basic_string<String> const& key) const -> iterator {
    return find_key(key.strv());
  }

  template <template <class> class RefCount>
  template <class String>
  auto basic_buffer<RefCount>::find_key(basic_string<String> const& key) const -> iterator {
    return find_key(key.strv());
  }

  template <template <class> class RefCount>
  template <class String>
  auto basic_packet<RefCount>::find_key(basic_string<String> const& key) const -> iterator {
    return find_key(key.strv());
  }

  template <template <class> class RefCount>
  auto basic_heap<RefCount>::find_key(shim::string_view key) const -> iterator {
    if (is_object()) {
      auto deref = [] (auto& it) { return it->first; };
      return iterator(detail::dn_iterator<RefCount>(try_get_fields()->find(key), deref));
    } else {
      throw type_error("dart::heap isn't an object and cannot find key-value mappings");
    }
  }

  template <template <class> class RefCount>
  auto basic_buffer<RefCount>::find_key(shim::string_view key) const -> iterator {
    return iterator(*this, detail::get_object<RefCount>(raw)->get_key_it(key));
  }

  template <template <class> class RefCount>
  auto basic_packet<RefCount>::find_key(shim::string_view key) const -> iterator {
    return shim::visit([&] (auto& v) -> iterator { return v.find_key(key); }, impl);
  }

  template <class Object>
  auto basic_object<Object>::keys() const -> std::vector<value_type> {
    return val.keys();
  }

  template <template <class> class RefCount>
  std::vector<basic_heap<RefCount>> basic_heap<RefCount>::keys() const {
    return detail::keys_impl(*this);
  }

  template <template <class> class RefCount>
  std::vector<basic_buffer<RefCount>> basic_buffer<RefCount>::keys() const {
    return detail::keys_impl(*this);
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

  template <class Object>
  template <class KeyType, class>
  bool basic_object<Object>::has_key(KeyType const& key) const noexcept {
    return val.has_key(key);
  }

  template <template <class> class RefCount>
  template <class String>
  bool basic_heap<RefCount>::has_key(basic_string<String> const& key) const {
    return has_key(key.strv());
  }

  template <template <class> class RefCount>
  template <class String>
  bool basic_buffer<RefCount>::has_key(basic_string<String> const& key) const {
    return has_key(key.strv());
  }

  template <template <class> class RefCount>
  template <class String>
  bool basic_packet<RefCount>::has_key(basic_string<String> const& key) const {
    return has_key(key.strv());
  }

  template <template <class> class RefCount>
  bool basic_heap<RefCount>::has_key(shim::string_view key) const {
    auto& fields = get_fields();
    return fields.find(key) != fields.end();
  }

  template <template <class> class RefCount>
  bool basic_buffer<RefCount>::has_key(shim::string_view key) const {
    auto elem = detail::get_object<RefCount>(raw)->get_key(key, [] (auto) {});
    return elem.buffer != nullptr;
  }

  template <template <class> class RefCount>
  bool basic_packet<RefCount>::has_key(shim::string_view key) const {
    return shim::visit([&] (auto& v) { return v.has_key(key); }, impl);
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  bool basic_heap<RefCount>::has_key(KeyType const& key) const {
    if (key.get_type() == type::string) return has_key(key.strv());
    else return false;
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  bool basic_buffer<RefCount>::has_key(KeyType const& key) const {
    if (key.get_type() == type::string) return has_key(key.strv());
    else return false;
  }

  template <template <class> class RefCount>
  template <class KeyType, class>
  bool basic_packet<RefCount>::has_key(KeyType const& key) const {
    if (key.get_type() == type::string) return has_key(key.strv());
    else return false;
  }

  template <class Object>
  void basic_object<Object>::ensure_object(shim::string_view msg) const {
    if (!val.is_object()) throw type_error(msg.data());
  }

  template <template <class> class RefCount>
  template <class... Args>
  basic_heap<RefCount>::basic_heap(detail::object_tag, Args&&... the_args) :
    data(make_shareable<RefCount<packet_fields>>())
  {
    inject_pairs(std::forward<Args>(the_args)...);
  }

  template <template <class> class RefCount>
  template <class KeyType, class ValueType, class... Pairs>
  void basic_heap<RefCount>::inject_pairs(KeyType&& key, ValueType&& value, Pairs&&... pairs) {
    add_field(std::forward<KeyType>(key), std::forward<ValueType>(value));
    inject_pairs(std::forward<Pairs>(pairs)...);
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
    return detail::dn_iterator<RefCount> {new_it, std::forward<Deref>(deref)};
  }

  template <template <class> class RefCount>
  shim::string_view basic_heap<RefCount>::iterator_key(iterator pos) const {
    using fields_layout = typename detail::dn_iterator<RefCount>::fields_layout;
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

  namespace detail {

#if DART_HAS_RAPIDJSON
    template <template <class> class RefCount>
    object<RefCount>::object(rapidjson::Value const& fields) noexcept : elems(fields.MemberCount()) {
      using sorter = std::vector<rapidjson::Value::ConstMemberIterator>;

      // Sort our fields real quick.
      // Ugly that this is necessary, but key lookup assumes things
      // are lexicographically sorted.
      sorter sorted;
      sorted.reserve(elems);
      for (auto it = fields.MemberBegin(); it != fields.MemberEnd(); ++it) {
        sorted.push_back(it);
      }
      std::sort(sorted.begin(), sorted.end(), [] (auto& lhs, auto& rhs) {
        shim::string_view l {lhs->name.GetString(), lhs->name.GetStringLength()};
        shim::string_view r {rhs->name.GetString(), rhs->name.GetStringLength()};
        return l < r;
      });

      // Iterate over our elements and write each one into the buffer.
      object_entry* entry = vtable();
      size_t offset = reinterpret_cast<gsl::byte*>(&vtable()[elems]) - DART_FROM_THIS_MUT;
      for (auto& it : sorted) {
        // Using the current offset, align a pointer for the key (string type).
        auto* unaligned = DART_FROM_THIS_MUT + offset;
        auto* aligned = detail::align_pointer<RefCount>(unaligned, detail::raw_type::string);
        offset += aligned - unaligned;

        // Add an entry to the vtable.
        auto val_type = json_identify<RefCount>(it->value);
        new(entry++) object_entry(val_type, offset, it->name.GetString());

        // Layout our key.
        offset += json_lower<RefCount>(aligned, it->name);

        // Realign our pointer for our value type.
        unaligned = DART_FROM_THIS_MUT + offset;
        aligned = detail::align_pointer<RefCount>(unaligned, val_type);
        offset += aligned - unaligned;

        // Layout our value (or copy it in if it's already been finalized).
        offset += json_lower<RefCount>(aligned, it->value);
      }

      // This is necessary to ensure packets can be naively stored in
      // contiguous buffers without ruining their alignment.
      offset = detail::pad_bytes<RefCount>(offset, detail::raw_type::object);

      // object is laid out, write in our final size.
      bytes = offset;
    }
#endif

    // FIXME: Audit this function. A LOT has changed since it was written.
    template <template <class> class RefCount>
    object<RefCount>::object(packet_fields<RefCount> const* fields) noexcept : elems(fields->size()) {
      // Iterate over our elements and write each one into the buffer.
      object_entry* entry = vtable();
      size_t offset = reinterpret_cast<gsl::byte*>(&vtable()[elems]) - DART_FROM_THIS_MUT;
      for (auto const& field : *fields) {
        // Using the current offset, align a pointer for the key (string type).
        auto* unaligned = DART_FROM_THIS_MUT + offset;
        auto* aligned = detail::align_pointer<RefCount>(unaligned, detail::raw_type::string);
        offset += aligned - unaligned;

        // Add an entry to the vtable.
        new(entry++) object_entry(field.second.get_raw_type(), offset, field.first.str());

        // Layout our key.
        offset += field.first.layout(aligned);

        // Realign our pointer for our value type.
        unaligned = DART_FROM_THIS_MUT + offset;
        aligned = detail::align_pointer<RefCount>(unaligned, field.second.get_raw_type());
        offset += aligned - unaligned;

        // Layout our value (or copy it in if it's already been finalized).
        offset += field.second.layout(aligned);
      }

      // This is necessary to ensure packets can be naively stored in
      // contiguous buffers without ruining their alignment.
      offset = detail::pad_bytes<RefCount>(offset, detail::raw_type::object);

      // object is laid out, write in our final size.
      bytes = offset;
    }

    template <template <class> class RefCount>
    size_t object<RefCount>::size() const noexcept {
      return elems;
    }

    template <template <class> class RefCount>
    size_t object<RefCount>::get_sizeof() const noexcept {
      return bytes;
    }

    template <template <class> class RefCount>
    auto object<RefCount>::begin() const noexcept -> ll_iterator<RefCount> {
      return ll_iterator<RefCount>(0, DART_FROM_THIS, load_value);
    }

    template <template <class> class RefCount>
    auto object<RefCount>::end() const noexcept -> ll_iterator<RefCount> {
      return ll_iterator<RefCount>(size(), DART_FROM_THIS, load_value);
    }

    template <template <class> class RefCount>
    auto object<RefCount>::key_begin() const noexcept -> ll_iterator<RefCount> {
      return ll_iterator<RefCount>(0, DART_FROM_THIS, load_key);
    }

    template <template <class> class RefCount>
    auto object<RefCount>::key_end() const noexcept -> ll_iterator<RefCount> {
      return ll_iterator<RefCount>(size(), DART_FROM_THIS, load_key);
    }

    template <template <class> class RefCount>
    template <class Callback>
    auto object<RefCount>::get_key(shim::string_view const key, Callback&& cb) const noexcept -> raw_element {
      // Get the size of the vtable.
      size_t const num_keys = size();

      // Run binary search to find the key.
      gsl::byte const* target = nullptr;
      auto type = detail::raw_type::null;
      int32_t low = 0, high = num_keys - 1;
      gsl::byte const* const base = DART_FROM_THIS;
      while (high >= low) {
        // Calculate the location of the next guess.
        auto const mid = (low + high) / 2;

        // Run the comparison.
        auto const& entry = vtable()[mid];
        auto comparison = -entry.prefix_compare(key);
        if (!comparison) {
          auto const* curr_str = detail::get_string({detail::raw_type::string, base + entry.get_offset()});
          comparison = key.compare(curr_str->get_strv());
        }

        // Update.
        if (comparison == 0) {
          // We've found it!
          // The lambda is passed through here specifically so that get_it and get_key_it can return
          // iterators without having to duplicate the logic in this function.
          // I originally had a more straightforward approach where we always returned an integer along
          // with the raw_element, but it caused like a 15% performance regression, so now we're
          // taking this approach.
          cb(mid);
          type = entry.get_type();
          target = base + entry.get_offset();
          break;
        } else if (comparison > 0) {
          low = mid + 1;
        } else {
          high = mid - 1;
        }
      }
      return {type, target};
    }

    template <template <class> class RefCount>
    auto object<RefCount>::get_it(shim::string_view const key) const noexcept -> ll_iterator<RefCount> {
      size_t idx;
      get_key(key, [&] (auto target) { idx = target; });
      return ll_iterator<RefCount>(idx, DART_FROM_THIS, load_value);
    }

    template <template <class> class RefCount>
    auto object<RefCount>::get_key_it(shim::string_view const key) const noexcept -> ll_iterator<RefCount> {
      size_t idx;
      get_key(key, [&] (auto target) { idx = target; });
      return ll_iterator<RefCount>(idx, DART_FROM_THIS, load_key);
    }

    template <template <class> class RefCount>
    auto object<RefCount>::get_value(shim::string_view const key) const noexcept -> raw_element {
      return get_value_impl(key, [] (auto) {});
    }

    template <template <class> class RefCount>
    auto object<RefCount>::at_value(shim::string_view const key) const -> raw_element {
      auto& ex_msg = "dart::buffer does not contain the requested mapping";
      return get_value_impl(key, [&] (auto& elem) { if (!elem.buffer) throw std::out_of_range(ex_msg); });
    }

    template <template <class> class RefCount>
    auto object<RefCount>::load_key(gsl::byte const* base, size_t idx) noexcept
      -> typename ll_iterator<RefCount>::value_type
    {
      // Get our vtable entry.
      auto const& entry = detail::get_object<RefCount>({raw_type::object, base})->vtable()[idx];
      return {detail::raw_type::string, base + entry.get_offset()};
    }

    template <template <class> class RefCount>
    auto object<RefCount>::load_value(gsl::byte const* base, size_t idx) noexcept
      -> typename ll_iterator<RefCount>::value_type
    {
      // Get our vtable entry.
      auto const& entry = detail::get_object<RefCount>({raw_type::object, base})->vtable()[idx];

      // Jump over the key and align to the given type.
      auto const* val_ptr = base + entry.get_offset();
      auto const* key_ptr = detail::get_string({raw_type::string, val_ptr});
      return {entry.get_type(), detail::align_pointer<RefCount>(val_ptr + key_ptr->get_sizeof(), entry.get_type())};
    }

    template <template <class> class RefCount>
    template <class Callback>
    auto object<RefCount>::get_value_impl(shim::string_view const key, Callback&& cb) const -> raw_element {
      // Propagate through to get_key to grab the pointer to our key and the type of our value.
      auto const field = get_key(key, [] (auto) {});

      // If the pointer is null, the key didn't exist, and we're done.
      // Callback function is passed through here specifically so that at_value can throw without having
      // to duplicate logic from get_value.
      // Originally had a simple if here, with a boolean argument to control throwing behavior, but it
      // caused a performance regression.
      // The approach with the lambda appears to allow the compiler to completely remove the code when
      // called from get_value but still allow the throwing behavior for at_value.
      cb(field);
      if (!field.buffer) return field;

      // If the type is null, we need to ignore the pointer (nulls are identifiers and
      // do not hold memory, so the pointer is worthless).
      if (field.type == detail::raw_type::null) return {field.type, nullptr};

      // Otherwise, jump over the key and align to the given type.
      auto const* key_ptr = detail::get_string({detail::raw_type::string, field.buffer});
      return {field.type, detail::align_pointer<RefCount>(field.buffer + key_ptr->get_sizeof(), field.type)};
    }

    template <template <class> class RefCount>
    object_entry* object<RefCount>::vtable() noexcept {
      auto* base = DART_FROM_THIS_MUT + sizeof(bytes) + sizeof(elems);
      return shim::launder(reinterpret_cast<object_entry*>(base));
    }

    template <template <class> class RefCount>
    object_entry const* object<RefCount>::vtable() const noexcept {
      auto* base = DART_FROM_THIS + sizeof(bytes) + sizeof(elems);
      return shim::launder(reinterpret_cast<object_entry const*>(base));
    }

  }

}

#endif
