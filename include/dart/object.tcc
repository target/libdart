#ifndef DART_OBJECT_H
#define DART_OBJECT_H

/*----- Local Includes -----*/

#include "common.h"

/*----- Function Implementations -----*/

namespace dart {

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
    >* EnableIf
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
    >* EnableIf
  >
  basic_object<Object>::basic_object(Arg&& arg) : val(std::forward<Arg>(arg)) {
    ensure_object("dart::packet::object can only be constructed as an object");
  }

  template <class Object>
  template <class KeyType, class ValueType, class EnableIf>
  basic_object<Object>& basic_object<Object>::add_field(KeyType&& key, ValueType&& value) & {
    val.add_field(std::forward<KeyType>(key), std::forward<ValueType>(value));
    return *this;
  }

  template <class Object>
  template <class KeyType, class ValueType, class EnableIf>
  basic_object<Object>&& basic_object<Object>::add_field(KeyType&& key, ValueType&& value) && {
    auto&& tmp = std::move(val).add_field(std::forward<KeyType>(key), std::forward<ValueType>(value));
    (void) tmp;
    return std::move(*this);
  }

  template <class Object>
  template <class KeyType, class EnableIf>
  basic_object<Object>& basic_object<Object>::remove_field(KeyType const& key) & {
    val.remove_field(key);
    return *this;
  }

  template <class Object>
  template <class KeyType, class EnableIf>
  basic_object<Object>&& basic_object<Object>::remove_field(KeyType const& key) && {
    auto&& tmp = std::move(val).remove_field(key);
    (void) tmp;
    return std::move(*this);
  }

  template <class Object>
  template <class KeyType, class ValueType, class EnableIf>
  auto basic_object<Object>::insert(KeyType&& key, ValueType&& value) -> iterator {
    return val.insert(std::forward<KeyType>(key), std::forward<ValueType>(value));
  }

  template <class Object>
  template <class KeyType, class ValueType, class EnableIf>
  auto basic_object<Object>::set(KeyType&& key, ValueType&& value) -> iterator {
    return val.set(std::forward<KeyType>(key), std::forward<ValueType>(value));
  }

  template <class Object>
  template <class KeyType, class EnableIf>
  auto basic_object<Object>::erase(KeyType const& key) -> iterator {
    return val.erase(key);
  }

  template <class Object>
  template <class Obj, class EnableIf>
  void basic_object<Object>::clear() {
    val.clear();
  }

  template <class Object>
  template <class... Args, class EnableIf>
  basic_object<Object> basic_object<Object>::inject(Args&&... the_args) const {
    return basic_object {val.inject(std::forward<Args>(the_args)...)};
  }

  template <class Object>
  basic_object<Object> basic_object<Object>::project(std::initializer_list<shim::string_view> keys) const {
    return val.project(keys);
  }

  template <class Object>
  template <class StringSpan, class EnableIf>
  basic_object<Object> basic_object<Object>::project(StringSpan const& keys) const {
    return val.project(keys);
  }

  template <class Object>
  template <class KeyType, class EnableIf>
  auto basic_object<Object>::operator [](KeyType const& key) const& -> value_type {
    return val[key];
  }

  template <class Object>
  template <class KeyType, class EnableIf>
  decltype(auto) basic_object<Object>::operator [](KeyType const& key) && {
    return std::move(val)[key];
  }

  template <class Object>
  template <class KeyType, class EnableIf>
  auto basic_object<Object>::get(KeyType const& key) const& -> value_type {
    return val.get(key);
  }

  template <class Object>
  template <class KeyType, class EnableIf>
  decltype(auto) basic_object<Object>::get(KeyType const& key) && {
    return std::move(val).get(key);
  }

  template <class Object>
  template <class KeyType, class T, class EnableIf>
  auto basic_object<Object>::get_or(KeyType const& key, T&& opt) const -> value_type {
    return val.get_or(key, std::forward<T>(opt));
  }

  template <class Object>
  auto basic_object<Object>::get_nested(shim::string_view path, char separator) const -> value_type {
    return val.get_nested(path, separator);
  }

  template <class Object>
  template <class KeyType, class EnableIf>
  auto basic_object<Object>::at(KeyType const& key) const& -> value_type {
    return val.at(key);
  }

  template <class Object>
  template <class KeyType, class EnableIf>
  decltype(auto) basic_object<Object>::at(KeyType const& key) && {
    return std::move(val).at(key);
  }

  template <class Object>
  template <class KeyType, class EnableIf>
  auto basic_object<Object>::find(KeyType const& key) const -> iterator {
    return val.find(key);
  }

  template <class Object>
  template <class KeyType, class EnableIf>
  auto basic_object<Object>::find_key(KeyType const& key) const -> iterator {
    return val.find_key(key);
  }

  template <class Object>
  auto basic_object<Object>::keys() const -> std::vector<value_type> {
    return val.keys();
  }

  template <class Object>
  template <class KeyType, class EnableIf>
  bool basic_object<Object>::has_key(KeyType const& key) const noexcept {
    return val.has_key(key);
  }

  template <class Object>
  void basic_object<Object>::ensure_object(shim::string_view msg) const {
    if (!val.is_object()) throw type_error(msg.data());
  }

  namespace detail {

#ifdef DART_USE_SAJSON
    template <template <class> class RefCount>
    object<RefCount>::object(sajson::value fields) noexcept :
      elems(static_cast<uint32_t>(fields.get_length()))
    {
      // Iterate over our elements and write each one into the buffer.
      object_entry* entry = vtable();
      size_t offset = reinterpret_cast<gsl::byte*>(&vtable()[elems]) - DART_FROM_THIS_MUT;
      for (auto idx = 0U; idx < elems; ++idx) {
        // Using the current offset, align a pointer for the key (string type).
        auto* unaligned = DART_FROM_THIS_MUT + offset;
        auto* aligned = align_pointer<RefCount>(unaligned, detail::raw_type::string);
        offset += aligned - unaligned;

        // Get our key/value from sajson.
        auto key = fields.get_object_key(idx);
        auto val = fields.get_object_value(idx);

        // Construct the vtable entry.
        shim::string_view keyv {key.data(), key.length()};
        auto val_type = json_identify<RefCount>(val);
        new(entry++) object_entry(val_type, static_cast<uint32_t>(offset), keyv);

        // Layout our key.
        new(aligned) string(keyv);
        offset += find_sizeof<RefCount>({raw_type::string, aligned});

        // Realign our pointer for our value type.
        unaligned = DART_FROM_THIS_MUT + offset;
        aligned = align_pointer<RefCount>(unaligned, val_type);
        offset += aligned - unaligned;

        // Layout our value (or copy it in if it's already been finalized).
        offset += json_lower<RefCount>(aligned, val);
      }

      // This is necessary to ensure packets can be naively stored in
      // contiguous buffers without ruining their alignment.
      offset = pad_bytes<RefCount>(offset, detail::raw_type::object);

      // object is laid out, write in our final size.
      bytes = static_cast<uint32_t>(offset);
    }
#endif

#if DART_HAS_RAPIDJSON
    template <template <class> class RefCount>
    object<RefCount>::object(rapidjson::Value const& fields) noexcept :
      elems(static_cast<uint32_t>(fields.MemberCount()))
    {
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
        auto lhs_len = lhs->name.GetStringLength();
        auto rhs_len = rhs->name.GetStringLength();
        if (lhs_len == rhs_len) {
          shim::string_view l {lhs->name.GetString(), lhs_len};
          shim::string_view r {rhs->name.GetString(), rhs_len};
          return l < r;
        } else {
          return lhs_len < rhs_len;
        }
      });

      // Iterate over our elements and write each one into the buffer.
      object_entry* entry = vtable();
      size_t offset = reinterpret_cast<gsl::byte*>(&vtable()[elems]) - DART_FROM_THIS_MUT;
      for (auto& it : sorted) {
        // Using the current offset, align a pointer for the key (string type).
        auto* unaligned = DART_FROM_THIS_MUT + offset;
        auto* aligned = align_pointer<RefCount>(unaligned, detail::raw_type::string);
        offset += aligned - unaligned;

        // Add an entry to the vtable.
        auto val_type = json_identify<RefCount>(it->value);
        new(entry++) object_entry(val_type, static_cast<uint32_t>(offset), it->name.GetString());

        // Layout our key.
        offset += json_lower<RefCount>(aligned, it->name);

        // Realign our pointer for our value type.
        unaligned = DART_FROM_THIS_MUT + offset;
        aligned = align_pointer<RefCount>(unaligned, val_type);
        offset += aligned - unaligned;

        // Layout our value (or copy it in if it's already been finalized).
        offset += json_lower<RefCount>(aligned, it->value);
      }

      // This is necessary to ensure packets can be naively stored in
      // contiguous buffers without ruining their alignment.
      offset = pad_bytes<RefCount>(offset, detail::raw_type::object);

      // object is laid out, write in our final size.
      bytes = static_cast<uint32_t>(offset);
    }
#endif

    template <template <class> class RefCount>
    object<RefCount>::object(gsl::span<packet_pair<RefCount>> pairs) noexcept :
      elems(static_cast<uint32_t>(pairs.size()))
    {
      // Iterate over our elements and write each one into the buffer.
      object_entry* entry = vtable();
      size_t offset = reinterpret_cast<gsl::byte*>(&vtable()[elems]) - DART_FROM_THIS_MUT;
      for (auto& pair : pairs) {
        // Using the current offset, align a pointer for the key (string type).
        auto* unaligned = DART_FROM_THIS_MUT + offset;
        auto* aligned = align_pointer<RefCount>(unaligned, detail::raw_type::string);
        offset += aligned - unaligned;

        // Add an entry to the vtable.
        new(entry++)
          object_entry(pair.value.get_raw_type(),
            static_cast<uint32_t>(offset), pair.key.str());

        // Layout our key.
        offset += pair.key.layout(aligned);

        // Realign our pointer for our value type.
        unaligned = DART_FROM_THIS_MUT + offset;
        aligned = align_pointer<RefCount>(unaligned, pair.value.get_raw_type());
        offset += aligned - unaligned;

        // Layout our value (or copy it in if it's already been finalized).
        offset += pair.value.layout(aligned);
      }

      // This is necessary to ensure packets can be naively stored in
      // contiguous buffers without ruining their alignment.
      offset = pad_bytes<RefCount>(offset, detail::raw_type::object);

      // object is laid out, write in our final size.
      bytes = static_cast<uint32_t>(offset);
    }

    // FIXME: Audit this function. A LOT has changed since it was written.
    template <template <class> class RefCount>
    object<RefCount>::object(packet_fields<RefCount> const* fields) noexcept :
      elems(static_cast<uint32_t>(fields->size()))
    {
      // Iterate over our elements and write each one into the buffer.
      object_entry* entry = vtable();
      size_t offset = reinterpret_cast<gsl::byte*>(&vtable()[elems]) - DART_FROM_THIS_MUT;
      for (auto const& field : *fields) {
        // Using the current offset, align a pointer for the key (string type).
        auto* unaligned = DART_FROM_THIS_MUT + offset;
        auto* aligned = align_pointer<RefCount>(unaligned, detail::raw_type::string);
        offset += aligned - unaligned;

        // Add an entry to the vtable.
        new(entry++)
          object_entry(field.second.get_raw_type(),
              static_cast<uint32_t>(offset), field.first.str());

        // Layout our key.
        offset += field.first.layout(aligned);

        // Realign our pointer for our value type.
        unaligned = DART_FROM_THIS_MUT + offset;
        aligned = align_pointer<RefCount>(unaligned, field.second.get_raw_type());
        offset += aligned - unaligned;

        // Layout our value (or copy it in if it's already been finalized).
        offset += field.second.layout(aligned);
      }

      // This is necessary to ensure packets can be naively stored in
      // contiguous buffers without ruining their alignment.
      offset = pad_bytes<RefCount>(offset, detail::raw_type::object);

      // object is laid out, write in our final size.
      bytes = static_cast<uint32_t>(offset);
    }

    template <template <class> class RefCount>
    object<RefCount>::object(object const* base, object const* incoming) noexcept : elems(0) {
      // This is unfortunate.
      // We need to merge together two objects that may contain duplicate keys,
      // but before we can start laying out the object we have to know how many
      // keys will be present so we can calculate the address of the end of the vtable.
      // Unfortunately we can't know how many keys there will be until we've already
      // walked across the objects and de-duped things, which is expensive, and so
      // for now we'll just assume that all keys are unique (conservative option, maximum
      // memory usage by the vtable), and memmove things after the fact if that assumption
      // turns out to be inaccurate.
      // Note that, of course this whole problem could be solved by creating a
      // temporary std::unordered_set to dedupe the keys for us, but the cost of doing so
      // would completely defeat the point of this whole pathway.
      auto guess = base->size() + incoming->size();

      // Iterate across both object simultaneously, uniquely visiting each key-value pair,
      // giving precedence to the incoming packet for collisions.
      // Write each pair into our buffer.
      object_entry* entry = vtable();
      size_t offset = reinterpret_cast<gsl::byte*>(&vtable()[guess]) - DART_FROM_THIS_MUT;
      buffer_builder<RefCount>::each_unique_pair(base, incoming, [&] (auto raw_key, auto raw_val) {
        // Using the current offset, align a pointer for the key (string type).
        auto* unaligned = DART_FROM_THIS_MUT + offset;
        auto* aligned = align_pointer<RefCount>(unaligned, detail::raw_type::string);
        offset += aligned - unaligned;

        // Add an entry to the vtable.
        auto* key = get_string(raw_key);
        new(entry++) object_entry(raw_val.type,
            static_cast<uint32_t>(offset), key->get_strv().data());

        // Copy in our key.
        auto key_len = find_sizeof<RefCount>(raw_key);
        std::copy_n(raw_key.buffer, key_len, aligned);
        offset += key_len;

        // Realign our pointer for our value type.
        unaligned = DART_FROM_THIS_MUT + offset;
        aligned = align_pointer<RefCount>(unaligned, raw_val.type);
        offset += aligned - unaligned;

        // Copy in our value
        auto val_len = find_sizeof<RefCount>(raw_val);
        std::copy_n(raw_val.buffer, val_len, aligned);
        offset += val_len;
        ++elems;
      });

      // Alright, since we guessed at the total size of the object, we now have to handle
      // the case where we were wrong.
      if (elems != guess) offset = realign(guess, offset);

      // This is necessary to ensure packets can be naively stored in
      // contiguous buffers without ruining their alignment.
      offset = pad_bytes<RefCount>(offset, detail::raw_type::object);

      // object is laid out, write in our final size.
      bytes = static_cast<uint32_t>(offset);
    }

    template <template <class> class RefCount>
    template <class Key>
    object<RefCount>::object(object const* base, gsl::span<Key const*> key_ptrs) noexcept : elems(0) {
      // Need to guess again. To see the reasoning for this, check the object merge constructor.
      auto guess = key_ptrs.size();

      // Iterate over our elements and write each one into the buffer.
      object_entry* entry = vtable();
      size_t offset = reinterpret_cast<gsl::byte*>(&vtable()[guess]) - DART_FROM_THIS_MUT;
      buffer_builder<RefCount>::project_each_pair(base, key_ptrs, [&] (auto raw_key, auto raw_val) {
        // Using the current offset, align a pointer for the key (string type).
        auto* unaligned = DART_FROM_THIS_MUT + offset;
        auto* aligned = align_pointer<RefCount>(unaligned, detail::raw_type::string);
        offset += aligned - unaligned;

        // Add an entry to the vtable.
        auto* key = get_string(raw_key);
        new(entry++) object_entry(raw_val.type,
            static_cast<uint32_t>(offset), key->get_strv().data());

        // Copy in our key.
        auto key_len = find_sizeof<RefCount>(raw_key);
        std::copy_n(raw_key.buffer, key_len, aligned);
        offset += key_len;

        // Realign our pointer for our value type.
        unaligned = DART_FROM_THIS_MUT + offset;
        aligned = align_pointer<RefCount>(unaligned, raw_val.type);
        offset += aligned - unaligned;

        // Copy in our value
        auto val_len = find_sizeof<RefCount>(raw_val);
        std::copy_n(raw_val.buffer, val_len, aligned);
        offset += val_len;
        ++elems;
      });

      // Alright, since we guessed at the total size of the object, we now have to handle
      // the case where we were wrong.
      if (elems != guess) offset = realign(guess, offset);

      // This is necessary to ensure packets can be naively stored in
      // contiguous buffers without ruining their alignment.
      offset = pad_bytes<RefCount>(offset, detail::raw_type::object);

      // object is laid out, write in our final size.
      bytes = static_cast<uint32_t>(offset);
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
      ssize_t const key_size = key.size();
      gsl::byte const* const base = DART_FROM_THIS;
      int32_t low = 0, high = static_cast<int32_t>(num_keys) - 1;
      while (high >= low) {
        // Calculate the location of the next guess.
        auto const mid = (low + high) / 2;

        // Run the comparison.
        auto const& entry = vtable()[mid];
        ssize_t comparison = -entry.prefix_compare(key);
        if (!comparison) {
          auto const* curr_str = detail::get_string({detail::raw_type::string, base + entry.get_offset()});
          auto const curr_view = curr_str->get_strv();
          ssize_t const curr_size = curr_view.size();
          comparison = (curr_size == key_size) ? key.compare(curr_view) : key_size - curr_size;
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
      return {entry.get_type(), align_pointer<RefCount>(val_ptr + key_ptr->get_sizeof(), entry.get_type())};
    }

    template <template <class> class RefCount>
    size_t object<RefCount>::realign(size_t guess, size_t offset) noexcept {
      // Get a pointer to where the packet data we wrote starts
      assert(elems < guess);
      auto* src = reinterpret_cast<gsl::byte const*>(&vtable()[guess]);

      // Get a pointer to the next alignment boundary after where the vtable ACTUALLY ends.
      auto* unaligned = reinterpret_cast<gsl::byte*>(&vtable()[elems]);
      auto* dst = align_pointer<RefCount>(unaligned, vtable()[0].get_type());

      // Re-align things.
      auto diff = src - dst;
      std::copy(src, DART_FROM_THIS + offset, dst);
      offset -= diff;

      // We just realigned things, which means all of our vtable offsets are now wrong.
      for (auto idx = 0U; idx < elems; ++idx) vtable()[idx].adjust_offset(-diff);

      // Lastly, we need to zero the trailing bytes to ensure buffers
      // can still be compared via memcmp.
      std::fill_n(DART_FROM_THIS_MUT + offset, diff, gsl::byte {});
      return offset;
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
      return {field.type, align_pointer<RefCount>(field.buffer + key_ptr->get_sizeof(), field.type)};
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
