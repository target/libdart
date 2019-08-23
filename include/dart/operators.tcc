#ifndef DART_OPERATORS_H
#define DART_OPERATORS_H

/*----- Local Includes -----*/

#include "common.h"

/*----- Function Definitions -----*/

// I apologize for this whole file.
// I'm not usually a macro kind of guy, but this is one of those rare
// situations where the code would actually be WORSE without the macros.
// It doesn't help that the Dart API surface is so large
namespace dart {

  /*----- Wrapper Equality Operations -----*/

  // Macro defines symmetric equality comparison operators (across implementation types)
  // for a given pair of wrapper templates.
#define DART_DEFINE_CROSS_WRAPPER_EQUALITY_OPERATORS(lhs_wrapper, rhs_wrapper)                                  \
  template <template <class> class LhsRC,                                                                       \
            template <class> class RhsRC,                                                                       \
            template <template <class> class> class LhsPacket,                                                  \
            template <template <class> class> class RhsPacket>                                                  \
  constexpr bool                                                                                                \
  operator ==(lhs_wrapper<LhsPacket<LhsRC>> const&, rhs_wrapper<RhsPacket<RhsRC>> const&) {                     \
    return false;                                                                                               \
  }                                                                                                             \
  template <template <class> class LhsRC,                                                                       \
            template <class> class RhsRC,                                                                       \
            template <template <class> class> class LhsPacket,                                                  \
            template <template <class> class> class RhsPacket>                                                  \
  constexpr bool                                                                                                \
  operator !=(lhs_wrapper<LhsPacket<LhsRC>> const& lhs, rhs_wrapper<RhsPacket<RhsRC>> const& rhs) {             \
    return !(lhs == rhs);                                                                                       \
  }                                                                                                             \
  template <template <class> class LhsRC,                                                                       \
            template <class> class RhsRC,                                                                       \
            template <template <class> class> class LhsPacket,                                                  \
            template <template <class> class> class RhsPacket>                                                  \
  constexpr bool                                                                                                \
  operator ==(rhs_wrapper<LhsPacket<LhsRC>> const&, lhs_wrapper<RhsPacket<RhsRC>> const&) {                     \
    return false;                                                                                               \
  }                                                                                                             \
  template <template <class> class LhsRC,                                                                       \
            template <class> class RhsRC,                                                                       \
            template <template <class> class> class LhsPacket,                                                  \
            template <template <class> class> class RhsPacket>                                                  \
  constexpr bool                                                                                                \
  operator !=(rhs_wrapper<LhsPacket<LhsRC>> const& rhs, lhs_wrapper<RhsPacket<RhsRC>> const& lhs) {             \
    return !(rhs == lhs);                                                                                       \
  }

  DART_DEFINE_CROSS_WRAPPER_EQUALITY_OPERATORS(basic_object, basic_array);
  DART_DEFINE_CROSS_WRAPPER_EQUALITY_OPERATORS(basic_object, basic_string);
  DART_DEFINE_CROSS_WRAPPER_EQUALITY_OPERATORS(basic_object, basic_number);
  DART_DEFINE_CROSS_WRAPPER_EQUALITY_OPERATORS(basic_object, basic_flag);
  DART_DEFINE_CROSS_WRAPPER_EQUALITY_OPERATORS(basic_object, basic_null);

  DART_DEFINE_CROSS_WRAPPER_EQUALITY_OPERATORS(basic_array, basic_string);
  DART_DEFINE_CROSS_WRAPPER_EQUALITY_OPERATORS(basic_array, basic_number);
  DART_DEFINE_CROSS_WRAPPER_EQUALITY_OPERATORS(basic_array, basic_flag);
  DART_DEFINE_CROSS_WRAPPER_EQUALITY_OPERATORS(basic_array, basic_null);

  DART_DEFINE_CROSS_WRAPPER_EQUALITY_OPERATORS(basic_string, basic_number);
  DART_DEFINE_CROSS_WRAPPER_EQUALITY_OPERATORS(basic_string, basic_flag);
  DART_DEFINE_CROSS_WRAPPER_EQUALITY_OPERATORS(basic_string, basic_null);

  DART_DEFINE_CROSS_WRAPPER_EQUALITY_OPERATORS(basic_number, basic_flag);
  DART_DEFINE_CROSS_WRAPPER_EQUALITY_OPERATORS(basic_number, basic_null);

  DART_DEFINE_CROSS_WRAPPER_EQUALITY_OPERATORS(basic_flag, basic_null);
#undef DART_DEFINE_CROSS_WRAPPER_EQUALITY_OPERATORS

  // Macro defines symmetric equality comparison operators for a given wrapper type and its
  // implementation type.
  // XXX: Would be nice to be able to compare across packet types here, but these templates
  // aren't otherwise constrained, and I'm afraid it would be too wide open.
#define DART_DEFINE_WRAPPER_PACKET_EQUALITY_OPERATORS(wrapper)                                                  \
  template <template <class> class LhsRC,                                                                       \
            template <class> class RhsRC, template <template <class> class> class Packet>                       \
  bool operator ==(wrapper<Packet<LhsRC>> const& lhs, Packet<RhsRC> const& rhs) {                               \
    return lhs.dynamic() == rhs;                                                                                \
  }                                                                                                             \
  template <template <class> class LhsRC,                                                                       \
            template <class> class RhsRC, template <template <class> class> class Packet>                       \
  bool operator !=(wrapper<Packet<LhsRC>> const& lhs, Packet<RhsRC> const& rhs) {                               \
    return !(lhs == rhs);                                                                                       \
  }                                                                                                             \
  template <template <class> class LhsRC,                                                                       \
            template <class> class RhsRC, template <template <class> class> class Packet>                       \
  bool operator ==(Packet<LhsRC> const& rhs, wrapper<Packet<RhsRC>> const& lhs) {                               \
    return rhs == lhs.dynamic();                                                                                \
  }                                                                                                             \
  template <template <class> class LhsRC,                                                                       \
            template <class> class RhsRC, template <template <class> class> class Packet>                       \
  bool operator !=(Packet<LhsRC> const& rhs, wrapper<Packet<RhsRC>> const& lhs) {                               \
    return !(rhs == lhs);                                                                                       \
  }

  DART_DEFINE_WRAPPER_PACKET_EQUALITY_OPERATORS(basic_object);
  DART_DEFINE_WRAPPER_PACKET_EQUALITY_OPERATORS(basic_array);
  DART_DEFINE_WRAPPER_PACKET_EQUALITY_OPERATORS(basic_string);
  DART_DEFINE_WRAPPER_PACKET_EQUALITY_OPERATORS(basic_number);
  DART_DEFINE_WRAPPER_PACKET_EQUALITY_OPERATORS(basic_flag);
  DART_DEFINE_WRAPPER_PACKET_EQUALITY_OPERATORS(basic_null);
#undef DART_DEFINE_WRAPPER_PACKET_EQUALITY_OPERATORS

  // Macro defines symmetric equality comparison operators for a given pair of wrapper and
  // machine type.
#define DART_DEFINE_WRAPPER_MACHINE_EQUALITY_OPERATORS(wrapper, machine_type)                                   \
  template <template <class> class RefCount, template <template <class> class> class Packet>                    \
  bool operator ==(wrapper<Packet<RefCount>> const& lhs, machine_type rhs) {                                    \
    return lhs.dynamic() == rhs;                                                                                \
  }                                                                                                             \
  template <template <class> class RefCount, template <template <class> class> class Packet>                    \
  bool operator !=(wrapper<Packet<RefCount>> const& lhs, machine_type rhs) {                                    \
    return !(lhs == rhs);                                                                                       \
  }                                                                                                             \
  template <template <class> class RefCount, template <template <class> class> class Packet>                    \
  bool operator ==(machine_type rhs, wrapper<Packet<RefCount>> const& lhs) {                                    \
    return rhs == lhs.dynamic();                                                                                \
  }                                                                                                             \
  template <template <class> class RefCount, template <template <class> class> class Packet>                    \
  bool operator !=(machine_type rhs, wrapper<Packet<RefCount>> const& lhs) {                                    \
    return !(rhs == lhs);                                                                                       \
  }

  DART_DEFINE_WRAPPER_MACHINE_EQUALITY_OPERATORS(basic_string, shim::string_view);
  DART_DEFINE_WRAPPER_MACHINE_EQUALITY_OPERATORS(basic_number, int64_t);
  DART_DEFINE_WRAPPER_MACHINE_EQUALITY_OPERATORS(basic_number, double);
  DART_DEFINE_WRAPPER_MACHINE_EQUALITY_OPERATORS(basic_flag, bool);
  DART_DEFINE_WRAPPER_MACHINE_EQUALITY_OPERATORS(basic_null, std::nullptr_t);
#undef DART_DEFINE_WRAPPER_MACHINE_EQUALITY_OPERATORS

  // Macro defines an std::ostream redirection operator for a wrapper type.
#if DART_HAS_RAPIDJSON
#define DART_DEFINE_WRAPPER_OSTREAM_OPERATOR(wrapper)                                                           \
  template <class Packet>                                                                                       \
  std::ostream& operator <<(std::ostream& out, wrapper<Packet> const& dart) {                                   \
    out << dart.to_json();                                                                                      \
    return out;                                                                                                 \
  }

  DART_DEFINE_WRAPPER_OSTREAM_OPERATOR(basic_object);
  DART_DEFINE_WRAPPER_OSTREAM_OPERATOR(basic_array);
  DART_DEFINE_WRAPPER_OSTREAM_OPERATOR(basic_string);
  DART_DEFINE_WRAPPER_OSTREAM_OPERATOR(basic_number);
  DART_DEFINE_WRAPPER_OSTREAM_OPERATOR(basic_flag);
  DART_DEFINE_WRAPPER_OSTREAM_OPERATOR(basic_null);
#undef DART_DEFINE_WRAPPER_OSTREAM_OPERATOR
#endif

  /*----- Cross-type Equality Operators -----*/

  template <template <class> class LhsRC, template <class> class RhsRC>
  bool operator ==(basic_buffer<LhsRC> const& lhs, basic_heap<RhsRC> const& rhs) {
    // Make sure they're at least of the same type.
    if (lhs.get_type() != rhs.get_type()) return false;

    // Perform type specific comparisons.
    switch (lhs.get_type()) {
      case detail::type::object:
        {
          // Bail early if we can.
          if (lhs.size() != rhs.size()) return false;

          // Ouch.
          // Iterates over rhs and looks up into lhs because lhs is the finalized
          // object and lookups should be significantly faster on it.
          typename basic_heap<RhsRC>::iterator k, v;
          std::tie(k, v) = rhs.kvbegin();
          while (v != rhs.end()) {
            if (*v != lhs[*k]) return false;
            ++k, ++v;
          }
          return true;
        }
      case detail::type::array:
        {
          // Bail early if we can.
          if (lhs.size() != rhs.size()) return false;

          // Ouch.
          for (auto i = 0U; i < lhs.size(); ++i) {
            if (lhs[i] != rhs[i]) return false;
          }
          return true;
        }
      case detail::type::string:
        return lhs.strv() == rhs.strv();
      case detail::type::integer:
        return lhs.integer() == rhs.integer();
      case detail::type::decimal:
        return lhs.decimal() == rhs.decimal();
      case detail::type::boolean:
        return lhs.boolean() == rhs.boolean();
      default:
        DART_ASSERT(lhs.is_null());
        return true;
    }
  }

  template <template <class> class LhsRC, template <class> class RhsRC>
  bool operator ==(basic_heap<LhsRC> const& lhs, basic_buffer<RhsRC> const& rhs) {
    return rhs == lhs;
  }

  template <template <class> class LhsRC, template <class> class RhsRC>
  bool operator ==(basic_buffer<LhsRC> const& lhs, basic_packet<RhsRC> const& rhs) {
    return shim::visit([&] (auto& v) { return lhs == v; }, rhs.impl);
  }

  template <template <class> class LhsRC, template <class> class RhsRC>
  bool operator ==(basic_heap<LhsRC> const& lhs, basic_packet<RhsRC> const& rhs) {
    return shim::visit([&] (auto& v) { return lhs == v; }, rhs.impl);
  }

  template <template <class> class LhsRC, template <class> class RhsRC>
  bool operator !=(basic_buffer<LhsRC> const& lhs, basic_heap<RhsRC> const& rhs) {
    return !(lhs == rhs);
  }

  template <template <class> class LhsRC, template <class> class RhsRC>
  bool operator !=(basic_heap<LhsRC> const& lhs, basic_buffer<RhsRC> const& rhs) {
    return !(lhs == rhs);
  }

  template <template <class> class LhsRC, template <class> class RhsRC>
  bool operator !=(basic_buffer<LhsRC> const& lhs, basic_packet<RhsRC> const& rhs) {
    return !(lhs == rhs);
  }

  template <template <class> class LhsRC, template <class> class RhsRC>
  bool operator !=(basic_heap<LhsRC> const& lhs, basic_packet<RhsRC> const& rhs) {
    return !(lhs == rhs);
  }

  /*----- String Comparison -----*/

  template <template <template <class> class> class Packet, template <class> class RefCount>
  bool operator ==(Packet<RefCount> const& dart, shim::string_view str) noexcept {
    if (!dart.is_str() || dart.size() != static_cast<size_t>(str.size())) return false;
    return str == dart.str();
  }

  template <template <template <class> class> class Packet, template <class> class RefCount>
  bool operator ==(shim::string_view str, Packet<RefCount> const& dart) noexcept {
    return dart == str;
  }

  template <template <template <class> class> class Packet, template <class> class RefCount>
  bool operator ==(Packet<RefCount> const& dart, char const* str) noexcept {
    return dart == shim::string_view(str);
  }

  template <template <template <class> class> class Packet, template <class> class RefCount>
  bool operator ==(char const* str, Packet<RefCount> const& dart) noexcept {
    return shim::string_view(str) == dart;
  }

  template <template <template <class> class> class Packet, template <class> class RefCount>
  bool operator !=(Packet<RefCount> const& dart, shim::string_view str) noexcept {
    return !(dart == str);
  }

  template <template <template <class> class> class Packet, template <class> class RefCount>
  bool operator !=(shim::string_view str, Packet<RefCount> const& dart) noexcept {
    return !(str == dart);
  }
  
  template <template <template <class> class> class Packet, template <class> class RefCount>
  bool operator !=(Packet<RefCount> const& dart, char const* str) noexcept {
    return !(dart == shim::string_view(str));
  }

  template <template <template <class> class> class Packet, template <class> class RefCount>
  bool operator !=(char const* str, Packet<RefCount> const& dart) noexcept {
    return !(shim::string_view(str) == dart);
  }

  /*----- Decimal Comparison -----*/

  template <template <template <class> class> class Packet, template <class> class RefCount, class T, class =
    std::enable_if_t<
      std::is_floating_point<T>::value
    >
  >
  bool operator ==(Packet<RefCount> const& dart, T val) noexcept {
    return dart.is_decimal() ? dart.decimal() == val : false;
  }

  template <template <template <class> class> class Packet, template <class> class RefCount, class T, class =
    std::enable_if_t<
      std::is_floating_point<T>::value
    >
  >
  bool operator ==(T val, Packet<RefCount> const& dart) noexcept {
    return dart == val;
  }

  template <template <template <class> class> class Packet, template <class> class RefCount, class T, class =
    std::enable_if_t<
      std::is_floating_point<T>::value
    >
  >
  bool operator !=(Packet<RefCount> const& dart, T val) noexcept {
    return !(dart == val);
  }

  template <template <template <class> class> class Packet, template <class> class RefCount, class T, class =
    std::enable_if_t<
      std::is_floating_point<T>::value
    >
  >
  bool operator !=(T val, Packet<RefCount> const& dart) noexcept {
    return !(val == dart);
  }

  /*----- Integer Comparison -----*/

  template <template <template <class> class> class Packet, template <class> class RefCount, class T, class =
    std::enable_if_t<
      std::is_integral<T>::value
      &&
      !std::is_same<
        std::decay_t<T>,
        bool
      >::value
    >
  >
  bool operator ==(Packet<RefCount> const& dart, T const& val) noexcept {
    // Oh C++, the things you make me do to avoid a single warning.
    return dart::shim::compose_together(
      [] (auto& d, auto v, std::true_type) {
        return d.is_integer() ? static_cast<uint64_t>(d.integer()) == v : false;
      },
      [] (auto& d, auto v, std::false_type) {
        return d.is_integer() ? d.integer() == v : false;
      }
    )(dart, val, std::is_unsigned<T> {});
  }

  // Some unfortunate template machinery to allow integers and booleans
  // to coexist in peace.
  template <template <template <class> class> class Packet, template <class> class RefCount, class T, class =
    std::enable_if_t<
      std::is_integral<T>::value
      &&
      !std::is_same<
        std::decay_t<T>,
        bool
      >::value
    >
  >
  bool operator ==(T const& val, Packet<RefCount> const& dart) noexcept {
    return dart == val;
  }

  template <template <template <class> class> class Packet, template <class> class RefCount, class T, class =
    std::enable_if_t<
      std::is_integral<T>::value
      &&
      !std::is_same<
        std::decay_t<T>,
        bool
      >::value
    >
  >
  bool operator !=(Packet<RefCount> const& dart, T const& val) noexcept {
    return !(dart == val);
  }

  template <template <template <class> class> class Packet, template <class> class RefCount, class T, class =
    std::enable_if_t<
      std::is_integral<T>::value
      &&
      !std::is_same<
        std::decay_t<T>,
        bool
      >::value
    >
  >
  bool operator !=(T const& val, Packet<RefCount> const& dart) noexcept {
    return !(val == dart);
  }

  /*----- Boolean Comparison -----*/

  template <template <template <class> class> class Packet, template <class> class RefCount>
  bool operator ==(Packet<RefCount> const& dart, bool val) noexcept {
    return dart.is_boolean() ? dart.boolean() == val : false;
  }

  template <template <template <class> class> class Packet, template <class> class RefCount>
  bool operator ==(bool val, Packet<RefCount> const& dart) noexcept {
    return dart == val;
  }

  template <template <template <class> class> class Packet, template <class> class RefCount>
  bool operator !=(Packet<RefCount> const& dart, bool val) noexcept {
    return !(dart == val);
  }

  template <template <template <class> class> class Packet, template <class> class RefCount>
  bool operator !=(bool val, Packet<RefCount> const& dart) noexcept {
    return !(val == dart);
  }

  /*----- Null Comparison -----*/

  template <template <template <class> class> class Packet, template <class> class RefCount>
  bool operator ==(Packet<RefCount> const& dart, std::nullptr_t) noexcept {
    return dart.is_null();
  }

  template <template <template <class> class> class Packet, template <class> class RefCount>
  bool operator ==(std::nullptr_t, Packet<RefCount> const& dart) noexcept {
    return dart.is_null();
  }

  template <template <template <class> class> class Packet, template <class> class RefCount>
  bool operator !=(Packet<RefCount> const& dart, std::nullptr_t) noexcept {
    return !dart.is_null();
  }

  template <template <template <class> class> class Packet, template <class> class RefCount>
  bool operator !=(std::nullptr_t, Packet<RefCount> const& dart) noexcept {
    return !dart.is_null();
  }

  // Lazy, but effective.
#if DART_HAS_RAPIDJSON
  template <template <template <class> class> class Packet, template <class> class RefCount>
  std::ostream& operator <<(std::ostream& out, Packet<RefCount> const& dart) {
    out << dart.to_json();
    return out;
  }
#endif

}

#endif
