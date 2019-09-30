#ifndef DART_COMMON_IMPL_H
#define DART_COMMON_IMPL_H

/*----- Local Includes -----*/

#include "common.h"

/*----- Function Implementations -----*/

namespace dart {

  namespace detail {

    template <size_t bytes>
    int prefix_compare_impl(char const* prefix, char const* str, size_t const len) noexcept {
      if (len && *prefix != *str) return *prefix - *str;
      else if (len) return prefix_compare_impl<bytes - 1>(prefix + 1, str + 1, len - 1);
      else return *prefix;
    }
    template <>
    inline int prefix_compare_impl<0>(char const*, char const* str, size_t const len) noexcept {
      if (len) return -*str;
      else return 0;
    }

    template <template <class> class RefCount>
    bool dart_comparator<RefCount>::operator ()(shim::string_view lhs, shim::string_view rhs) const {
      auto const lhs_size = lhs.size();
      auto const rhs_size = rhs.size();
      if (lhs_size != rhs_size) return lhs_size < rhs_size;
      else return lhs < rhs;
    }

    template <template <class> class RefCount>
    template <template <template <class> class> class Packet>
    bool dart_comparator<RefCount>::operator ()(Packet<RefCount> const& lhs, shim::string_view rhs) const {
      auto const lhs_size = lhs.size();
      auto const rhs_size = rhs.size();
      if (lhs_size != rhs_size) return lhs_size < rhs_size;
      else return lhs.strv() < rhs;
    }

    template <template <class> class RefCount>
    template <template <template <class> class> class Packet>
    bool dart_comparator<RefCount>::operator ()(shim::string_view lhs, Packet<RefCount> const& rhs) const {
      auto const lhs_size = lhs.size();
      auto const rhs_size = rhs.size();
      if (lhs_size != rhs_size) return lhs_size < rhs_size;
      else return lhs < rhs.strv();
    }

    template <template <class> class RefCount>
    template <template <template <class> class> class Packet>
    bool dart_comparator<RefCount>::operator ()(basic_pair<Packet<RefCount>> const& lhs, shim::string_view rhs) const {
      auto const lhs_size = lhs.key.size();
      auto const rhs_size = rhs.size();
      if (lhs_size != rhs_size) return lhs_size < rhs_size;
      else return lhs.key.strv() < rhs;
    }

    template <template <class> class RefCount>
    template <template <template <class> class> class Packet>
    bool dart_comparator<RefCount>::operator ()(shim::string_view lhs, basic_pair<Packet<RefCount>> const& rhs) const {
      auto const lhs_size = lhs.size();
      auto const rhs_size = rhs.key.size();
      if (lhs_size != rhs_size) return lhs_size < rhs_size;
      else return lhs < rhs.key.strv();
    }

    template <template <class> class RefCount>
    template <
      template <template <class> class> class LhsPacket,
      template <template <class> class> class RhsPacket
    >
    bool dart_comparator<RefCount>::operator ()(LhsPacket<RefCount> const& lhs, RhsPacket<RefCount> const& rhs) const {
      auto const lhs_size = lhs.size();
      auto const rhs_size = rhs.size();
      if (lhs_size != rhs_size) return lhs_size < rhs_size;
      else return lhs.strv() < rhs.strv();
    }

    template <template <class> class RefCount>
    template <
      template <template <class> class> class LhsPacket,
      template <template <class> class> class RhsPacket
    >
    bool dart_comparator<RefCount>::operator
        ()(basic_pair<LhsPacket<RefCount>> const& lhs, RhsPacket<RefCount> const& rhs) const {
      auto const lhs_size = lhs.key.size();
      auto const rhs_size = rhs.size();
      if (lhs_size != rhs_size) return lhs_size < rhs_size;
      else return lhs.key.strv() < rhs.strv();
    }

    template <template <class> class RefCount>
    template <
      template <template <class> class> class LhsPacket,
      template <template <class> class> class RhsPacket
    >
    bool dart_comparator<RefCount>::operator
        ()(LhsPacket<RefCount> const& lhs, basic_pair<RhsPacket<RefCount>> const& rhs) const {
      auto const lhs_size = lhs.size();
      auto const rhs_size = rhs.key.size();
      if (lhs_size != rhs_size) return lhs_size < rhs_size;
      else return lhs.strv() < rhs.key.strv();
    }

    template <template <class> class RefCount>
    template <
      template <template <class> class> class LhsPacket,
      template <template <class> class> class RhsPacket
    >
    bool dart_comparator<RefCount>::operator
        ()(basic_pair<LhsPacket<RefCount>> const& lhs, basic_pair<RhsPacket<RefCount>> const& rhs) const {
      auto const lhs_size = lhs.key.size();
      auto const rhs_size = rhs.key.size();
      if (lhs_size != rhs_size) return lhs_size < rhs_size;
      else return lhs.key.strv() < rhs.key.strv();
    }

#if DART_USING_MSVC
#pragma warning(push)
#pragma warning(disable: 4805)
#endif

    template <class Lhs, class Rhs>
    bool typeless_comparator::operator ()(Lhs&& lhs, Rhs&& rhs) const noexcept {
      // if constexpr!
      // my kingdom for if constexpr!
      return shim::compose_together(
        [] (auto&& l, auto&& r, std::true_type) {
          return std::forward<decltype(l)>(l) == std::forward<decltype(r)>(r);
        },
        [] (auto&&, auto&&, std::false_type) {
          return false;
        }
      )(std::forward<Lhs>(lhs), std::forward<Rhs>(rhs), meta::are_comparable<Lhs, Rhs> {});
    }

#if DART_USING_MSVC
#pragma warning(pop)
#endif

    template <class T>
    vtable_entry<T>::vtable_entry(detail::raw_type type, uint32_t offset) {
      // Truncate dynamic type information.
      if (type == detail::raw_type::small_string) type = detail::raw_type::string;

      // Create our combined entry for the vtable.
      layout.offset = offset;
      layout.type = static_cast<uint8_t>(type);
    }

    template <class T>
    raw_type vtable_entry<T>::get_type() const noexcept {
      // Apparently this CAN'T use brace-initialization for... REASONS???
      // Put it down as _yet another_ painful edge case for "uniform" initialization.
      // I'm asking for an explicit, non-narrowing, conversion either way,
      // don't really understand the issue.
      return raw_type(layout.type.get());
    }

    template <class T>
    uint32_t vtable_entry<T>::get_offset() const noexcept {
      return layout.offset;
    }

    template <class T>
    void vtable_entry<T>::adjust_offset(std::ptrdiff_t diff) noexcept {
      layout.offset += diff;
    }

    inline prefix_entry::prefix_entry(detail::raw_type type, uint32_t offset, shim::string_view prefix) noexcept :
      vtable_entry<prefix_entry>(type, offset)
    {
      // Decide how many bytes we're going to copy out of the key.
      auto bytes = prefix.size();
      if (bytes > sizeof(this->layout.prefix)) bytes = sizeof(this->layout.prefix);

      // Set the length, truncating down to 256.
      auto max_len = std::numeric_limits<uint8_t>::max();
      if (prefix.size() < max_len) this->layout.len = static_cast<uint8_t>(prefix.size());
      else this->layout.len = max_len;

      // Try SO HARD not to violate strict aliasing rules, while copying those characters into an integer.
      // This is probably all still undefined behavior.
      // FIXME: It is because, _officially speaking_, we're reading an unitialized integer.
      storage_t raw {};
      std::copy_n(prefix.data(), bytes, reinterpret_cast<char*>(&raw));
      new(&raw) prefix_type;
      this->layout.prefix = *shim::launder(reinterpret_cast<prefix_type const*>(&raw));
    }

    inline int prefix_entry::prefix_compare(shim::string_view str) const noexcept {
      // Cache all of our lengths and stuff.
      auto const their_len = str.size();
      auto const our_len = this->layout.len;
      constexpr auto max_len = std::numeric_limits<uint8_t>::max();

      // Compare first by string lengths, then by lexical ordering.
      // If they are longer than us, but we're capped at the max value,
      // return equality to force key lookup to fall back on the general case.
      if (our_len < their_len) return (our_len == max_len) ? 0 : -1;
      else if (our_len == their_len) return compare_impl(str.data(), their_len);
      else return 1;
    }

    inline int prefix_entry::compare_impl(char const* const str, size_t const len) const noexcept {
      // Fast path where we attempt to perform a direct integer comparison.
      if (len >= sizeof(prefix_type)) {
        // Despite all my hard work, this is probably still undefined behavior.
        // FIXME: It is because, _officially speaking_, we're reading an unitialized integer.
        storage_t raw {};
        std::copy_n(str, sizeof(this->layout.prefix), reinterpret_cast<char*>(&raw));
        new(&raw) prefix_type;
        if (*shim::launder(reinterpret_cast<prefix_type const*>(&raw)) == this->layout.prefix) {
          return 0;
        }
      }

      // Fallback path where we actually compare the prefixes.
      auto* bytes = reinterpret_cast<char const*>(&this->layout.prefix);
      return detail::prefix_compare_impl<sizeof(prefix_type)>(bytes, str, len);
    }

    template <template <class> class RefCount>
    template <class Span>
    auto buffer_builder<RefCount>::build_buffer(Span pairs) -> buffer {
      using owner = buffer_refcount_type<RefCount>;

      // Low level object code assumes keys are sorted, so validate that assumption.
      std::sort(std::begin(pairs), std::end(pairs), dart_comparator<RefCount> {});

      // Calculate how much space we'll need.
      auto bytes = max_bytes(pairs);

      // Build it.
      auto ref = aligned_alloc<RefCount>(max_bytes(pairs), raw_type::object, [&] (auto* ptr) {
        // XXX: std::fill_n is REQUIRED here so that we can perform memcmps for finalized packets.
        std::fill_n(ptr, bytes, gsl::byte {});
        new(ptr) detail::object<RefCount>(pairs);
      });
      return basic_buffer<RefCount> {std::move(ref)};
    }

    template <template <class> class RefCount>
    auto buffer_builder<RefCount>::merge_buffers(buffer const& base, buffer const& incoming) -> buffer {
      // Unwrap our buffers to get the underlying machine representation.
      auto* raw_base = get_object<RefCount>(base.raw);
      auto* raw_incoming = get_object<RefCount>(incoming.raw);
      
      // Figure out the maximum amount of space we could need for the merged object.
      auto total_size = raw_base->get_sizeof() + raw_incoming->get_sizeof();

      // Merge it.
      auto ref = aligned_alloc<RefCount>(total_size, raw_type::object, [&] (auto* ptr) {
        std::fill_n(ptr, total_size, gsl::byte {});
        new(ptr) detail::object<RefCount>(raw_base, raw_incoming);
      });
      return basic_buffer<RefCount> {std::move(ref)};
    }

    template <template <class> class RefCount>
    template <class Spannable>
    auto buffer_builder<RefCount>::project_keys(buffer const& base, Spannable const& keys) -> buffer {
      return detail::sort_spannable<RefCount>(keys, [&] (auto key_ptrs) {
        // Unwrap our buffers to get the underlying machine representation.
        auto* raw_base = get_object<RefCount>(base.raw);

        // Maximum required size is that of the current object, as the new one must be smaller.
        auto total_size = raw_base->get_sizeof();
        auto ref = aligned_alloc<RefCount>(total_size, raw_type::object, [&] (auto* ptr) {
          std::fill_n(ptr, total_size, gsl::byte {});
          new(ptr) detail::object<RefCount>(raw_base, key_ptrs);
        });
        return basic_buffer<RefCount> {std::move(ref)};
      });
    }

    template <template <class> class RefCount>
    template <class Span>
    size_t buffer_builder<RefCount>::max_bytes(Span pairs) {
      // Walk across the span of pairs and calculate the total required memory.
      size_t bytes = 0;
      shim::optional<shim::string_view> prev_key;
      for (auto& pair : pairs) {
        // Keys are sorted, so check if we ever run into the same key twice
        // in a row to avoid duplicates.
        auto curr_key = pair.key.strv();
        if (curr_key == prev_key) {
          throw std::invalid_argument("dart::buffer cannot make an object with duplicate keys");
        } else if (curr_key.size() > std::numeric_limits<uint16_t>::max()) {
          throw std::invalid_argument("dart::buffer keys cannot be longer than UINT16_MAX");
        }
        prev_key = curr_key;

        // Accumulate the total number of bytes.
        bytes += pair.key.upper_bound() + alignment_of<RefCount>(pair.key.get_raw_type()) - 1;
        bytes += pair.value.upper_bound() + alignment_of<RefCount>(pair.value.get_raw_type()) - 1;
      }
      bytes += sizeof(detail::object<RefCount>) + ((sizeof(detail::object_entry) * (pairs.size() + 1)));
      return bytes + detail::pad_bytes<RefCount>(bytes, detail::raw_type::object);
    }

    template <template <class> class RefCount>
    template <class Callback>
    void buffer_builder<
      RefCount
    >::each_unique_pair(object<RefCount> const* base, object<RefCount> const* incoming, Callback&& cb) {
      // BOY this sucks
      dart_comparator<RefCount> comp;
      auto in_vals = incoming->begin(), base_vals = base->begin();
      auto in_keys = incoming->key_begin(), base_keys = base->key_begin();
      auto in_key_end = incoming->key_end(), base_key_end = base->key_end();

      // Spin across both keyspaces,
      // identifying unique keys, giving precedence to the incoming object.
      auto unwrap = [] (auto& it) { return get_string(*it); };
      while (in_keys != in_key_end) {
        // Walk the base key iterator forward until
        // we find a pair of keys that compare greater or equal.
        while (base_keys != base_key_end) {
          // If the current base key is less than the current
          // incoming key, pass it through as we know the incoming object
          // can't contain it
          // Otherwise it could be equal or greater, so break to allow
          // the incoming object to overtake us again.
          if (comp(unwrap(base_keys)->get_strv(), unwrap(in_keys)->get_strv())) {
            // Current pair is unique, and there are more to find
            cb(*base_keys++, *base_vals++);
          } else if (!comp(unwrap(in_keys)->get_strv(), unwrap(base_keys)->get_strv())) {
            // The base key is not less than the incoming key,
            // and the incoming key is not less than the base key
            // Current pair is a duplicate, skip it and yield control to the incoming
            ++base_keys, ++base_vals;
            break;
          } else {
            // The base key is not less than the incoming key,
            // but the incoming key is greater than the base key, so key is unique,
            // but we've overtaken the incoming iterator, so yield control to it.
            break;
          }
        }

        // At this point we're either out of base keys,
        // or the base key iterator has overtaken us.
        while (in_keys != in_key_end) {
          if (base_keys == base_key_end
              || !comp(unwrap(base_keys)->get_strv(), unwrap(in_keys)->get_strv())) {
            // Incoming key is less than or equal to base key
            cb(*in_keys, *in_vals);
            if (base_keys != base_key_end && !comp(unwrap(in_keys)->get_strv(), unwrap(base_keys)->get_strv())) {
              // Incoming key is equal to base key.
              // Increment the base iterators to ensure this duplicate pair
              // isn't considered when we make it back around the loop.
              ++base_keys, ++base_vals;
            }
          } else {
            // Incoming key is greater than the base key
            // We've overtaken the base iterator, yield control back to it.
            break;
          }
          ++in_keys, ++in_vals;
        }
      }

      // It's possible we didn't exhaust our base iterator
      while (base_keys != base_key_end) {
        cb(*base_keys++, *base_vals++);
      }
    }

    template <template <class> class RefCount>
    template <class Key, class Callback>
    void buffer_builder<
      RefCount
    >::project_each_pair(object<RefCount> const* base, gsl::span<Key const*> key_ptrs, Callback&& cb) {
      // BOY this sucks
      dart_comparator<RefCount> comp;
      auto base_vals = base->begin();
      auto base_keys = base->key_begin();
      auto base_key_end = base->key_end();

      // Spin across both keyspaces,
      // identifying unique keys, giving precedence to the incoming object.
      for (auto in_keys = std::begin(key_ptrs); in_keys != std::end(key_ptrs); ++in_keys) {
        // Walk the base key iterator forward until
        // we find a pair of keys that compare greater or equal.
        auto& in_key = **in_keys;
        while (base_keys != base_key_end) {
          // If the current base key is less than the current
          // incoming key, pass it through as we know the incoming object
          // can't contain it
          // Otherwise it could be equal or greater, so break to allow
          // the incoming object to overtake us again.
          auto base_key = get_string(*base_keys)->get_strv();
          if (comp(base_key, in_key)) {
            // Current key is less than our current key pointer, and cannot
            // be contained within the projection. Skip it.
            ++base_keys, ++base_vals;
            continue;
          } else if (!comp(in_key, base_key)) {
            // Current key is equal to our current key pointer, and must
            // be contained within the project. Pass it along.
            cb(*base_keys++, *base_vals++);
          } else {
            // Current key is greater than our current key pointer.
            // Key may be contained within the projection, move to the
            // next key to know.
            break;
          }
        }
      }
    }

  }

}

#endif
