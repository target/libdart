/*----- System Includes -----*/

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

/*----- Local Includes -----*/

#include "dart_tests.h"

/*----- Type Declarations -----*/

template <class From, class To>
struct cast_is_correct :
  std::is_same<
    To,
    decltype(dart::convert::cast<To>(std::declval<From>()))
  >
{};

struct my_string {
  my_string(char const* val) : val(val) {}

  bool operator ==(my_string const& other) const {
    return val == other.val;
  }
  bool operator !=(my_string const& other) const {
    return !(*this == other);
  }
  bool operator <(my_string const& other) const {
    return val < other.val;
  }

  std::string val;
};

namespace dart {
  namespace convert {
    template <>
    struct conversion_traits<my_string> {
      template <class Packet>
      Packet to_dart(my_string const& str) {
        return Packet::make_string(str.val);
      }
      template <class Packet>
      my_string from_dart(Packet const& pkt) {
        if (!pkt.is_str()) throw dart::type_error("Expected string");
        return my_string {pkt.str()};
      }
      template <class Packet>
      bool compare(Packet const& pkt, my_string const& str) noexcept {
        return pkt.is_str() && pkt.strv() == str.val;
      }
    };
  }
}

namespace std {
  template <>
  struct hash<my_string> {
    size_t operator ()(my_string const& str) const noexcept {
      return std::hash<std::string> {}(str.val);
    }
  };
}

namespace {
  using clock = std::chrono::system_clock;
}

/*----- Static Assertions -----*/

static_assert(
  dart::convert::is_castable<my_string, dart::heap>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<my_string, dart::buffer>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<my_string, dart::packet>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap, my_string>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer, my_string>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet, my_string>::value,
  "Dart is misconfigured"
);

// Internal API casting
static_assert(
  dart::convert::is_castable<dart::heap, dart::buffer>::value
  &&
  cast_is_correct<dart::heap, dart::buffer>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer, dart::heap>::value
  &&
  cast_is_correct<dart::buffer, dart::heap>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap, dart::packet>::value
  &&
  cast_is_correct<dart::heap, dart::packet>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet, dart::heap>::value
  &&
  cast_is_correct<dart::packet, dart::heap>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet, dart::buffer>::value
  &&
  cast_is_correct<dart::packet, dart::buffer>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer, dart::packet>::value
  &&
  cast_is_correct<dart::buffer, dart::packet>::value,
  "Dart is misconfigured"
);

// dart::heap view type casting
static_assert(
  !dart::convert::is_castable<dart::heap, dart::heap::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap&, dart::heap::view>::value
  &&
  cast_is_correct<dart::heap&, dart::heap::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap::view, dart::heap>::value
  &&
  cast_is_correct<dart::heap::view, dart::heap>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<dart::heap, dart::buffer::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer::view, dart::heap>::value
  &&
  cast_is_correct<dart::buffer::view, dart::heap>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<dart::heap, dart::packet::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet::view, dart::heap>::value
  &&
  cast_is_correct<dart::packet::view, dart::heap>::value,
  "Dart is misconfigured"
);

// dart::heap external type casting
static_assert(
  dart::convert::is_castable<dart::heap, std::string>::value
  &&
  cast_is_correct<dart::heap, std::string>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<std::string, dart::heap>::value
  &&
  cast_is_correct<std::string, dart::heap>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap::view, std::string>::value
  &&
  cast_is_correct<dart::heap::view, std::string>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<std::string, dart::heap::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap, int>::value
  &&
  cast_is_correct<dart::heap, int>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<int, dart::heap>::value
  &&
  cast_is_correct<int, dart::heap>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap::view, int>::value
  &&
  cast_is_correct<dart::heap::view, int>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<int, dart::heap::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap, double>::value
  &&
  cast_is_correct<dart::heap, double>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<double, dart::heap>::value
  &&
  cast_is_correct<double, dart::heap>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap::view, double>::value
  &&
  cast_is_correct<dart::heap::view, double>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<double, dart::heap::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap, bool>::value
  &&
  cast_is_correct<dart::heap, bool>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<bool, dart::heap>::value
  &&
  cast_is_correct<bool, dart::heap>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap::view, bool>::value
  &&
  cast_is_correct<dart::heap::view, bool>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<bool, dart::heap::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap, std::nullptr_t>::value
  &&
  cast_is_correct<dart::heap, std::nullptr_t>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<std::nullptr_t, dart::heap>::value
  &&
  cast_is_correct<std::nullptr_t, dart::heap>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap::view, std::nullptr_t>::value
  &&
  cast_is_correct<dart::heap::view, std::nullptr_t>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<std::nullptr_t, dart::heap::view>::value,
  "Dart is misconfigured"
);

// dart::heap wrapper type casting
static_assert(
  dart::convert::is_castable<dart::heap, dart::heap::object>::value
  &&
  cast_is_correct<dart::heap, dart::heap::object>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap::object, dart::heap>::value
  &&
  cast_is_correct<dart::heap::object, dart::heap>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap, dart::buffer::object>::value
  &&
  cast_is_correct<dart::heap, dart::buffer::object>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer::object, dart::heap>::value
  &&
  cast_is_correct<dart::buffer::object, dart::heap>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap, dart::packet::object>::value
  &&
  cast_is_correct<dart::heap, dart::packet::object>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet::object, dart::heap>::value
  &&
  cast_is_correct<dart::packet::object, dart::heap>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap, dart::heap::array>::value
  &&
  cast_is_correct<dart::heap, dart::heap::array>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap::array, dart::heap>::value
  &&
  cast_is_correct<dart::heap::array, dart::heap>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap, dart::heap::string>::value
  &&
  cast_is_correct<dart::heap, dart::heap::string>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap::string, dart::heap>::value
  &&
  cast_is_correct<dart::heap::string, dart::heap>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap, dart::heap::number>::value
  &&
  cast_is_correct<dart::heap, dart::heap::number>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap::number, dart::heap>::value
  &&
  cast_is_correct<dart::heap::number, dart::heap>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap, dart::heap::flag>::value
  &&
  cast_is_correct<dart::heap, dart::heap::flag>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap::flag, dart::heap>::value
  &&
  cast_is_correct<dart::heap::flag, dart::heap>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap, dart::heap::null>::value
  &&
  cast_is_correct<dart::heap, dart::heap::null>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap::null, dart::heap>::value
  &&
  cast_is_correct<dart::heap::null, dart::heap>::value,
  "Dart is misconfigured"
);

// dart::buffer view type casting
static_assert(
  !dart::convert::is_castable<dart::buffer, dart::buffer::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer&, dart::buffer::view>::value
  &&
  cast_is_correct<dart::buffer&, dart::buffer::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer::view, dart::buffer>::value
  &&
  cast_is_correct<dart::buffer::view, dart::buffer>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<dart::buffer, dart::heap::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap::view, dart::buffer>::value
  &&
  cast_is_correct<dart::heap::view, dart::buffer>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<dart::buffer, dart::packet::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet::view, dart::buffer>::value
  &&
  cast_is_correct<dart::packet::view, dart::buffer>::value,
  "Dart is misconfigured"
);

// dart::buffer external type casting
static_assert(
  dart::convert::is_castable<dart::buffer, std::string>::value
  &&
  cast_is_correct<dart::buffer, std::string>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<std::string, dart::buffer>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer::view, std::string>::value
  &&
  cast_is_correct<dart::buffer::view, std::string>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<std::string, dart::buffer::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer, int>::value
  &&
  cast_is_correct<dart::buffer, int>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<int, dart::buffer>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer::view, int>::value
  &&
  cast_is_correct<dart::buffer::view, int>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<int, dart::buffer::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer, double>::value
  &&
  cast_is_correct<dart::buffer, double>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<double, dart::buffer>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer::view, double>::value
  &&
  cast_is_correct<dart::buffer::view, double>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<double, dart::buffer::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer, bool>::value
  &&
  cast_is_correct<dart::buffer, bool>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<bool, dart::buffer>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer::view, bool>::value
  &&
  cast_is_correct<dart::buffer::view, bool>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<bool, dart::buffer::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer, std::nullptr_t>::value
  &&
  cast_is_correct<dart::buffer, std::nullptr_t>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<std::nullptr_t, dart::buffer>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer::view, std::nullptr_t>::value
  &&
  cast_is_correct<dart::buffer::view, std::nullptr_t>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<std::nullptr_t, dart::buffer::view>::value,
  "Dart is misconfigured"
);

// dart::buffer wrapper type casting
static_assert(
  dart::convert::is_castable<dart::buffer, dart::buffer::object>::value
  &&
  cast_is_correct<dart::buffer, dart::buffer::object>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer::object, dart::buffer>::value
  &&
  cast_is_correct<dart::buffer::object, dart::buffer>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer, dart::heap::object>::value
  &&
  cast_is_correct<dart::buffer, dart::heap::object>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap::object, dart::buffer>::value
  &&
  cast_is_correct<dart::heap::object, dart::buffer>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer, dart::packet::object>::value
  &&
  cast_is_correct<dart::buffer, dart::packet::object>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet::object, dart::buffer>::value
  &&
  cast_is_correct<dart::packet::object, dart::buffer>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer, dart::buffer::array>::value
  &&
  cast_is_correct<dart::buffer, dart::buffer::array>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer::array, dart::buffer>::value
  &&
  cast_is_correct<dart::buffer::array, dart::buffer>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer, dart::buffer::string>::value
  &&
  cast_is_correct<dart::heap, dart::heap::string>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer::string, dart::buffer>::value
  &&
  cast_is_correct<dart::heap::string, dart::heap>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap, dart::buffer::number>::value
  &&
  cast_is_correct<dart::heap, dart::buffer::number>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer::number, dart::buffer>::value
  &&
  cast_is_correct<dart::buffer::number, dart::buffer>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer, dart::buffer::flag>::value
  &&
  cast_is_correct<dart::buffer, dart::buffer::flag>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer::flag, dart::buffer>::value
  &&
  cast_is_correct<dart::buffer::flag, dart::buffer>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer, dart::buffer::null>::value
  &&
  cast_is_correct<dart::buffer, dart::buffer::null>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer::null, dart::buffer>::value
  &&
  cast_is_correct<dart::buffer::null, dart::buffer>::value,
  "Dart is misconfigured"
);

// dart::packet view type casting
static_assert(
  !dart::convert::is_castable<dart::packet, dart::packet::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet&, dart::packet::view>::value
  &&
  cast_is_correct<dart::packet&, dart::packet::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet::view, dart::packet>::value
  &&
  cast_is_correct<dart::packet::view, dart::packet>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<dart::packet, dart::heap::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap::view, dart::packet>::value
  &&
  cast_is_correct<dart::heap::view, dart::packet>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<dart::packet, dart::buffer::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer::view, dart::packet>::value
  &&
  cast_is_correct<dart::buffer::view, dart::packet>::value,
  "Dart is misconfigured"
);

// dart::packet external type casting
static_assert(
  dart::convert::is_castable<dart::packet, std::string>::value
  &&
  cast_is_correct<dart::packet, std::string>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<std::string, dart::packet>::value
  &&
  cast_is_correct<std::string, dart::packet>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet::view, std::string>::value
  &&
  cast_is_correct<dart::packet::view, std::string>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<std::string, dart::packet::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet, int>::value
  &&
  cast_is_correct<dart::packet, int>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<int, dart::packet>::value
  &&
  cast_is_correct<int, dart::packet>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet::view, int>::value
  &&
  cast_is_correct<dart::packet::view, int>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<int, dart::packet::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet, double>::value
  &&
  cast_is_correct<dart::packet, double>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<double, dart::packet>::value
  &&
  cast_is_correct<double, dart::packet>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet::view, double>::value
  &&
  cast_is_correct<dart::packet::view, double>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<double, dart::packet::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet, bool>::value
  &&
  cast_is_correct<dart::packet, bool>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<bool, dart::packet>::value
  &&
  cast_is_correct<bool, dart::packet>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet::view, bool>::value
  &&
  cast_is_correct<dart::packet::view, bool>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<bool, dart::packet::view>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet, std::nullptr_t>::value
  &&
  cast_is_correct<dart::packet, std::nullptr_t>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<std::nullptr_t, dart::packet>::value
  &&
  cast_is_correct<std::nullptr_t, dart::packet>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet::view, std::nullptr_t>::value
  &&
  cast_is_correct<dart::packet::view, std::nullptr_t>::value,
  "Dart is misconfigured"
);
static_assert(
  !dart::convert::is_castable<std::nullptr_t, dart::packet::view>::value,
  "Dart is misconfigured"
);

// dart::packet wrapper type casting
static_assert(
  dart::convert::is_castable<dart::packet, dart::packet::object>::value
  &&
  cast_is_correct<dart::packet, dart::packet::object>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet::object, dart::packet>::value
  &&
  cast_is_correct<dart::packet::object, dart::packet>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet, dart::heap::object>::value
  &&
  cast_is_correct<dart::packet, dart::heap::object>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap::object, dart::packet>::value
  &&
  cast_is_correct<dart::heap::object, dart::packet>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet, dart::buffer::object>::value
  &&
  cast_is_correct<dart::packet, dart::buffer::object>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::buffer::object, dart::packet>::value
  &&
  cast_is_correct<dart::buffer::object, dart::packet>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet, dart::packet::array>::value
  &&
  cast_is_correct<dart::packet, dart::packet::array>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet::array, dart::packet>::value
  &&
  cast_is_correct<dart::packet::array, dart::packet>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet, dart::packet::string>::value
  &&
  cast_is_correct<dart::heap, dart::heap::string>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet::string, dart::packet>::value
  &&
  cast_is_correct<dart::heap::string, dart::heap>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::heap, dart::packet::number>::value
  &&
  cast_is_correct<dart::heap, dart::packet::number>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet::number, dart::packet>::value
  &&
  cast_is_correct<dart::packet::number, dart::packet>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet, dart::packet::flag>::value
  &&
  cast_is_correct<dart::packet, dart::packet::flag>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet::flag, dart::packet>::value
  &&
  cast_is_correct<dart::packet::flag, dart::packet>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet, dart::packet::null>::value
  &&
  cast_is_correct<dart::packet, dart::packet::null>::value,
  "Dart is misconfigured"
);
static_assert(
  dart::convert::is_castable<dart::packet::null, dart::packet>::value
  &&
  cast_is_correct<dart::packet::null, dart::packet>::value,
  "Dart is misconfigured"
);

/*----- Function Implementations -----*/

SCENARIO("mutable dart types can be assigned to from many types", "[operator unit]") {
  GIVEN("a default constructed generic type") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      using svec = std::vector<my_string>;
      using map = std::map<my_string, my_string>;
      using mmap = std::multimap<my_string, my_string>;
      using umap = std::unordered_map<std::string, my_string>;
      using ummap = std::unordered_multimap<my_string, my_string>;
      using vec = std::vector<dart::shim::optional<dart::shim::variant<int, double, my_string>>>;
      using deq = std::deque<dart::shim::optional<dart::shim::variant<int, double, my_string>>>;
      using arr = std::array<dart::shim::optional<dart::shim::variant<int, double, my_string>>, 4>;
      using lst = std::list<dart::shim::optional<dart::shim::variant<int, double, my_string>>>;
      using flst = std::forward_list<dart::shim::optional<dart::shim::variant<int, double, my_string>>>;
      using set = std::set<my_string>;
      using uset = std::unordered_set<my_string>;
      using mset = std::multiset<my_string>;
      using umset = std::unordered_multiset<my_string>;
      using pair = std::pair<my_string, my_string>;
      using span = gsl::span<my_string const>;

      // Get a default constructed instance.
      pkt val;
      REQUIRE(val.is_null());

      // Test std::vector, std::variant, and std::optional assignment/comparison
      DYNAMIC_WHEN("the value is assigned from a vector", idx) {
        auto v = vec {1337, 3.14159, "there are many like it, but this vector is mine", dart::shim::nullopt};
        val = v;
        auto rv = dart::convert::cast<vec>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(v == rv);
          REQUIRE(rv == v);
          REQUIRE(val == v);
          REQUIRE(v == val);
          REQUIRE(val == rv);
          REQUIRE(rv == val);
          static_assert(noexcept(v == val), "Dart is misconfigured");
          static_assert(noexcept(val == v), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<vec, pkt>::value, "Dart is misconfigured");
        }
      }

      // Test std::deque, std::variant, and std::optional assignment/comparison
      DYNAMIC_WHEN("the value is assigned from a deque", idx) {
        auto d = deq {1337, 3.14159, "there are many like it, but this deque is mine", dart::shim::nullopt};
        val = d;
        auto rd = dart::convert::cast<deq>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(d == rd);
          REQUIRE(rd == d);
          REQUIRE(val == d);
          REQUIRE(d == val);
          static_assert(noexcept(d == val), "Dart is misconfigured");
          static_assert(noexcept(val == d), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<deq, pkt>::value, "Dart is misconfigured");
        }
      }

      // Test std::array, std::variant, and std::optional assignment/comparison
      DYNAMIC_WHEN("the value is assigned from an array", idx) {
        auto a = arr {{1337, 3.14159, "there are many like it, but this array is mine", dart::shim::nullopt}};
        val = a;
        auto ra = dart::convert::cast<arr>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(a == ra);
          REQUIRE(ra == a);
          REQUIRE(val == a);
          REQUIRE(a == val);
          REQUIRE(val == ra);
          REQUIRE(ra == val);
          static_assert(noexcept(a == val), "Dart is misconfigured");
          static_assert(noexcept(val == a), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<arr, pkt>::value, "Dart is misconfigured");
        }
      }

      // Test std::list, std::variant, and std::optional assignment/comparison
      DYNAMIC_WHEN("the value is assigned from a list", idx) {
        auto l = lst {1337, 3.14159, "there are many like it, but this list is mine", dart::shim::nullopt};
        val = l;
        auto rl = dart::convert::cast<lst>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(l == rl);
          REQUIRE(rl == l);
          REQUIRE(val == l);
          REQUIRE(l == val);
          REQUIRE(rl == val);
          REQUIRE(val == rl);
          static_assert(noexcept(l == val), "Dart is misconfigured");
          static_assert(noexcept(val == l), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<lst, pkt>::value, "Dart is misconfigured");
        }
      }

      // Test std::forward_list, std::variant, and std::optional assignment/comparison
      DYNAMIC_WHEN("the value is assigned from a forward list", idx) {
        auto fl = flst {1337, 3.14159, "there are many like it, but this list is mine", dart::shim::nullopt};
        val = fl;
        auto rfl = dart::convert::cast<flst>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(fl == rfl);
          REQUIRE(rfl == fl);
          REQUIRE(val == fl);
          REQUIRE(fl == val);
          REQUIRE(val == rfl);
          REQUIRE(rfl == val);
          static_assert(noexcept(fl == val), "Dart is misconfigured");
          static_assert(noexcept(val == fl), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<flst, pkt>::value, "Dart is misconfigured");
        }
      }

      // Test std::map assignment/comparison
      DYNAMIC_WHEN("the value is assigned from a map", idx) {
        auto m = map {{"hello", "world"}, {"yes", "no"}};
        val = m;
        auto rm = dart::convert::cast<map>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(m == rm);
          REQUIRE(rm == m);
          REQUIRE(val == m);
          REQUIRE(m == val);
          REQUIRE(val == rm);
          REQUIRE(rm == val);
          static_assert(noexcept(m == val), "Dart is misconfigured");
          static_assert(noexcept(val == m), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<map, pkt>::value, "Dart is misconfigured");
        }
      }

      // Test std::unordered_map assignment/comparison
      DYNAMIC_WHEN("the value is assigned from an unordered map", idx) {
        auto um = umap {{"hello", "world"}, {"yes", "no"}};
        val = um;
        auto rum = dart::convert::cast<umap>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(um == rum);
          REQUIRE(rum == um);
          REQUIRE(val == um);
          REQUIRE(um == val);
          REQUIRE(val == rum);
          REQUIRE(rum == val);
          static_assert(noexcept(um == val), "Dart is misconfigured");
          static_assert(noexcept(val == um), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<umap, pkt>::value, "Dart is misconfigured");
        }
      }

      // Test std::multimap assignment/comparison
      DYNAMIC_WHEN("the value is assigned from a multimap", idx) {
        auto m = mmap {{"hello", "world"}, {"yes", "no"}};
        val = m;
        auto rm = dart::convert::cast<mmap>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(m == rm);
          REQUIRE(rm == m);
          REQUIRE(val == m);
          REQUIRE(m == val);
          REQUIRE(val == rm);
          REQUIRE(rm == val);
          static_assert(noexcept(m == val), "Dart is misconfigured");
          static_assert(noexcept(val == m), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<mmap, pkt>::value, "Dart is misconfigured");
        }
      }

      // Test std::multimap assignment/comparison
      DYNAMIC_WHEN("the value is assigned from a multimap", idx) {
        auto um = ummap {{"hello", "world"}, {"yes", "no"}};
        val = um;
        auto rum = dart::convert::cast<ummap>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(um == rum);
          REQUIRE(rum == um);
          REQUIRE(val == um);
          REQUIRE(um == val);
          REQUIRE(val == rum);
          REQUIRE(rum == val);
          static_assert(noexcept(um == val), "Dart is misconfigured");
          static_assert(noexcept(val == um), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<ummap, pkt>::value, "Dart is misconfigured");
        }
      }

      // Test std::set assignment/comparison
      DYNAMIC_WHEN("the value is assigned from a set", idx) {
        auto s = set {"dark side", "meddle", "the wall", "animals"};
        val = s;
        auto rs = dart::convert::cast<set>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(s == rs);
          REQUIRE(rs == s);
          REQUIRE(val == s);
          REQUIRE(s == val);
          REQUIRE(val == rs);
          REQUIRE(rs == val);
          static_assert(noexcept(s == val), "Dart is misconfigured");
          static_assert(noexcept(val == s), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<set, pkt>::value, "Dart is misconfigured");
        }
      }

      // Test std::unordered_set assignment/comparison
      DYNAMIC_WHEN("the value is assigned from a unordered_set", idx) {
        auto s = uset {"dark side", "meddle", "the wall", "animals"};
        val = s;
        auto rs = dart::convert::cast<uset>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(s == rs);
          REQUIRE(rs == s);
          REQUIRE(val == s);
          REQUIRE(s == val);
          REQUIRE(val == rs);
          REQUIRE(rs == val);
          static_assert(noexcept(s == val), "Dart is misconfigured");
          static_assert(noexcept(val == s), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<uset, pkt>::value, "Dart is misconfigured");
        }
      }

      // Test std::multiset assignment/comparison
      DYNAMIC_WHEN("the value is assigned from a multiset", idx) {
        auto m = mset {"dark side", "meddle", "meddle", "the wall", "animals"};
        val = m;
        auto rm = dart::convert::cast<mset>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(m == rm);
          REQUIRE(rm == m);
          REQUIRE(val == m);
          REQUIRE(m == val);
          REQUIRE(val == rm);
          REQUIRE(rm == val);
          static_assert(noexcept(m == val), "Dart is misconfigured");
          static_assert(noexcept(val == m), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<mset, pkt>::value, "Dart is misconfigured");
        }
      }

      // Test std::unordered_multiset assignment/comparison
      DYNAMIC_WHEN("the value is assigned from a unordered multiset", idx) {
        auto m = umset {"dark side", "meddle", "meddle", "the wall", "animals"};
        val = m;
        auto rm = dart::convert::cast<umset>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(m == rm);
          REQUIRE(rm == m);
          REQUIRE(val == m);
          REQUIRE(m == val);
          REQUIRE(val == rm);
          REQUIRE(rm == val);
          static_assert(noexcept(m == val), "Dart is misconfigured");
          static_assert(noexcept(val == m), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<umset, pkt>::value, "Dart is misconfigured");
        }
      }

      DYNAMIC_WHEN("the value is assigned from a pair", idx) {
        auto p = pair {"first", "second"};
        val = p;
        auto rp = dart::convert::cast<pair>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(p == rp);
          REQUIRE(rp == p);
          REQUIRE(val == p);
          REQUIRE(p == val);
          REQUIRE(val == rp);
          REQUIRE(rp == val);
          static_assert(noexcept(p == val), "Dart is misconfigured");
          static_assert(noexcept(val == p), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<pair, pkt>::value, "Dart is misconfigured");
        }
      }

      DYNAMIC_WHEN("the value is assigned from a span", idx) {
        auto v = svec {"hello", "world", "yes", "no", "stop", "go"};
        span s = v;
        val = s;
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(val == s);
          REQUIRE(s == val);
          static_assert(noexcept(s == val), "Dart is misconfigured");
          static_assert(noexcept(val == s), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<span, pkt>::value, "Dart is misconfigured");
        }
      }

      DYNAMIC_WHEN("the value is assigned from a time point", idx) {
        auto t = clock::now();
        val = t;
        auto rt = dart::convert::cast<clock::time_point>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(std::chrono::time_point_cast<std::chrono::seconds>(t) == rt);
          REQUIRE(rt == std::chrono::time_point_cast<std::chrono::seconds>(t));
          REQUIRE(val == t);
          REQUIRE(t == val);
          REQUIRE(val == rt);
          REQUIRE(rt == val);
          static_assert(!noexcept(t == val), "Dart is misconfigured");
          static_assert(!noexcept(val == t), "Dart is misconfigured");
          static_assert(!dart::convert::are_nothrow_comparable<clock::time_point, pkt>::value, "Dart is misconfigured");
        }
      }
    });
  }

  GIVEN("a default constructed object type") {
    dart::mutable_object_api_test([] (auto tag, auto idx) {
      using object = typename decltype(tag)::type;
      using map = std::map<std::string, dart::shim::optional<dart::shim::variant<int, double, my_string>>>;
      using mmap = std::multimap<std::string, dart::shim::optional<dart::shim::variant<int, double, my_string>>>;
      using umap =
          std::unordered_map<std::string, dart::shim::optional<dart::shim::variant<int, double, my_string>>>;
      using ummap =
          std::unordered_multimap<std::string, dart::shim::optional<dart::shim::variant<int, double, my_string>>>;

      // Get a default constructed instance.
      object val;
      REQUIRE(val.is_object());
      REQUIRE(val.empty());

      // Test std::map assignment/comparison.
      DYNAMIC_WHEN("the value is assigned from a map", idx) {
        auto m = map {{"pi", 3.14159}, {"truth", 42}, {"best album", "dark side"}};
        val = m;
        auto rm = dart::convert::cast<map>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(m == rm);
          REQUIRE(rm == m);
          REQUIRE(val == m);
          REQUIRE(m == val);
          REQUIRE(val == rm);
          REQUIRE(rm == val);
          static_assert(noexcept(m == val), "Dart is misconfigured");
          static_assert(noexcept(val == m), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<map, object>::value, "Dart is misconfigured");
        }
      }

      // Test std::multimap assignment/comparison.
      DYNAMIC_WHEN("the value is assigned from a multimap", idx) {
        auto m = mmap {{"pi", 3.14159}, {"truth", 42}, {"best album", "dark side"}};
        val = m;
        auto rm = dart::convert::cast<mmap>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(m == rm);
          REQUIRE(rm == m);
          REQUIRE(val == m);
          REQUIRE(m == val);
          REQUIRE(val == rm);
          REQUIRE(rm == val);
          static_assert(noexcept(m == val), "Dart is misconfigured");
          static_assert(noexcept(val == m), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<map, object>::value, "Dart is misconfigured");
        }
      }

      // Test std::unordered_map assignment/comparison.
      DYNAMIC_WHEN("the value is assigned from an unordered map", idx) {
        auto m = umap {{"pi", 3.14159}, {"truth", 42}, {"best album", "dark side"}};
        val = m;
        auto rm = dart::convert::cast<umap>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(m == rm);
          REQUIRE(rm == m);
          REQUIRE(val == m);
          REQUIRE(m == val);
          REQUIRE(val == rm);
          REQUIRE(rm == val);
          static_assert(noexcept(m == val), "Dart is misconfigured");
          static_assert(noexcept(val == m), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<umap, object>::value, "Dart is misconfigured");
        }
      }
      //
      // Test std::unordered_multimap assignment/comparison.
      DYNAMIC_WHEN("the value is assigned from an unordered multimap", idx) {
        auto m = ummap {{"pi", 3.14159}, {"truth", 42}, {"best album", "dark side"}};
        val = m;
        auto rm = dart::convert::cast<ummap>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(m == rm);
          REQUIRE(rm == m);
          REQUIRE(val == m);
          REQUIRE(m == val);
          REQUIRE(val == rm);
          REQUIRE(rm == val);
          static_assert(noexcept(m == val), "Dart is misconfigured");
          static_assert(noexcept(val == m), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<umap, object>::value, "Dart is misconfigured");
        }
      }
    });
  }

  GIVEN("a default constructed array type") {
    dart::mutable_array_api_test([] (auto tag, auto idx) {
      using array = typename decltype(tag)::type;
      using vec = std::vector<dart::shim::optional<dart::shim::variant<int, double, my_string>>>;
      using deq = std::deque<dart::shim::optional<dart::shim::variant<int, double, my_string>>>;
      using arr = std::array<dart::shim::optional<dart::shim::variant<int, double, my_string>>, 4>;
      using set = std::list<std::string>;
      using uset = std::list<std::string>;
      using mset = std::list<std::string>;
      using umset = std::list<std::string>;
      using lst = std::list<dart::shim::optional<dart::shim::variant<int, double, my_string>>>;
      using flst = std::forward_list<dart::shim::optional<dart::shim::variant<int, double, my_string>>>;

      array val;
      REQUIRE(val.is_array());
      REQUIRE(val.empty());

      DYNAMIC_WHEN("the value is assigned from a vector", idx) {
        auto v = vec {1337, 3.14159, "there are many like it, but this vector is mine", dart::shim::nullopt};
        val = v;
        auto rv = dart::convert::cast<vec>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(v == rv);
          REQUIRE(rv == v);
          REQUIRE(val == v);
          REQUIRE(v == val);
          REQUIRE(val == rv);
          REQUIRE(rv == val);
          static_assert(noexcept(v == val), "Dart is misconfigured");
          static_assert(noexcept(val == v), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<vec, array>::value, "Dart is misconfigured");
        }
      }

      DYNAMIC_WHEN("the value is assigned from a deque", idx) {
        auto v = deq {1337, 3.14159, "there are many like it, but this vector is mine", dart::shim::nullopt};
        val = v;
        auto rv = dart::convert::cast<deq>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(v == rv);
          REQUIRE(rv == v);
          REQUIRE(val == v);
          REQUIRE(v == val);
          REQUIRE(val == rv);
          REQUIRE(rv == val);
          static_assert(noexcept(v == val), "Dart is misconfigured");
          static_assert(noexcept(val == v), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<vec, array>::value, "Dart is misconfigured");
        }
      }

      DYNAMIC_WHEN("the value is assigned from an array", idx) {
        auto v = arr {{1337, 3.14159, "there are many like it, but this array is mine", dart::shim::nullopt}};
        val = v;
        auto rv = dart::convert::cast<arr>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(v == rv);
          REQUIRE(rv == v);
          REQUIRE(val == v);
          REQUIRE(v == val);
          REQUIRE(val == rv);
          REQUIRE(rv == val);
          static_assert(noexcept(v == val), "Dart is misconfigured");
          static_assert(noexcept(val == v), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<arr, array>::value, "Dart is misconfigured");
        }
      }

      DYNAMIC_WHEN("the value is assigned from a list", idx) {
        auto v = lst {1337, 3.14159, "there are many like it, but this vector is mine", dart::shim::nullopt};
        val = v;
        auto rv = dart::convert::cast<lst>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(v == rv);
          REQUIRE(rv == v);
          REQUIRE(val == v);
          REQUIRE(v == val);
          REQUIRE(val == rv);
          REQUIRE(rv == val);
          static_assert(noexcept(v == val), "Dart is misconfigured");
          static_assert(noexcept(val == v), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<vec, array>::value, "Dart is misconfigured");
        }
      }

      DYNAMIC_WHEN("the value is assigned from a forward list", idx) {
        auto v = flst {1337, 3.14159, "there are many like it, but this vector is mine", dart::shim::nullopt};
        val = v;
        auto rv = dart::convert::cast<flst>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(v == rv);
          REQUIRE(rv == v);
          REQUIRE(val == v);
          REQUIRE(v == val);
          REQUIRE(val == rv);
          REQUIRE(rv == val);
          static_assert(noexcept(v == val), "Dart is misconfigured");
          static_assert(noexcept(val == v), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<vec, array>::value, "Dart is misconfigured");
        }
      }

      DYNAMIC_WHEN("the value is assigned from a set", idx) {
        auto s = set {"yes", "no", "stop", "go"};
        val = s;
        auto rs = dart::convert::cast<set>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(s == rs);
          REQUIRE(rs == s);
          REQUIRE(val == s);
          REQUIRE(s == val);
          REQUIRE(val == rs);
          REQUIRE(rs == val);
          static_assert(noexcept(s == val), "Dart is misconfigured");
          static_assert(noexcept(val == s), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<vec, array>::value, "Dart is misconfigured");
        }
      }

      DYNAMIC_WHEN("the value is assigned from a unordered set", idx) {
        auto s = uset {"yes", "no", "stop", "go"};
        val = s;
        auto rs = dart::convert::cast<uset>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(s == rs);
          REQUIRE(rs == s);
          REQUIRE(val == s);
          REQUIRE(s == val);
          REQUIRE(val == rs);
          REQUIRE(rs == val);
          static_assert(noexcept(s == val), "Dart is misconfigured");
          static_assert(noexcept(val == s), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<vec, array>::value, "Dart is misconfigured");
        }
      }

      DYNAMIC_WHEN("the value is assigned from a multiset", idx) {
        auto s = mset {"yes", "no", "stop", "go"};
        val = s;
        auto rs = dart::convert::cast<mset>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(s == rs);
          REQUIRE(rs == s);
          REQUIRE(val == s);
          REQUIRE(s == val);
          REQUIRE(val == rs);
          REQUIRE(rs == val);
          static_assert(noexcept(s == val), "Dart is misconfigured");
          static_assert(noexcept(val == s), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<vec, array>::value, "Dart is misconfigured");
        }
      }

      DYNAMIC_WHEN("the value is assigned from an unordered multiset", idx) {
        auto s = umset {"yes", "no", "stop", "go"};
        val = s;
        auto rs = dart::convert::cast<umset>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(s == rs);
          REQUIRE(rs == s);
          REQUIRE(val == s);
          REQUIRE(s == val);
          REQUIRE(val == rs);
          REQUIRE(rs == val);
          static_assert(noexcept(s == val), "Dart is misconfigured");
          static_assert(noexcept(val == s), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<vec, array>::value, "Dart is misconfigured");
        }
      }
    });
  }

  GIVEN("a default constructed string") {
    dart::mutable_string_api_test([] (auto tag, auto idx) {
      using string = typename decltype(tag)::type;

      string val;
      REQUIRE(val.is_str());
      REQUIRE(val.empty());

      DYNAMIC_WHEN("the value is assigned from a string literal", idx) {
        auto* str = "hello world";
        val = str;
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(str == val);
          REQUIRE(val == str);
          static_assert(noexcept(str == val), "Dart is misconfigured");
          static_assert(noexcept(val == str), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<char const*, string>::value, "Dart is misconfigured");
        }
      }

      DYNAMIC_WHEN("the value is assigned from a std::string", idx) {
        std::string str = "hello world";
        val = str;
        auto rstr = dart::convert::cast<std::string>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(str == rstr);
          REQUIRE(rstr == str);
          REQUIRE(str == val);
          REQUIRE(val == str);
          REQUIRE(val == rstr);
          REQUIRE(rstr == val);
          static_assert(noexcept(str == val), "Dart is misconfigured");
          static_assert(noexcept(val == str), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<std::string, string>::value, "Dart is misconfigured");
        }
      }

      DYNAMIC_WHEN("the value is assigned from a std::string_view", idx) {
        dart::shim::string_view str = "hello world";
        val = str;
        auto rstr = dart::convert::cast<dart::shim::string_view>(val);
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(str == rstr);
          REQUIRE(rstr == str);
          REQUIRE(str == val);
          REQUIRE(val == str);
          REQUIRE(val == rstr);
          REQUIRE(rstr == val);
          static_assert(noexcept(str == val), "Dart is misconfigured");
          static_assert(noexcept(val == str), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<dart::shim::string_view, string>::value, "Dart is misconfigured");
        }
      }
    });
  }

  GIVEN("a default constructed number") {
    dart::mutable_number_api_test([] (auto tag, auto idx) {
      using number = typename decltype(tag)::type;

      number val;
      REQUIRE(val.is_numeric());
      REQUIRE(val == 0);

      DYNAMIC_WHEN("the value is assigned from an integer literal", idx) {
        val = 1337;
        val = 1337L;
        val = 1337U;
        val = 1337UL;
        val = 1337LL;
        val = 1337ULL;
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(val == 1337);
          REQUIRE(1337 == val);
          REQUIRE(val == 1337L);
          REQUIRE(1337L == val);
          REQUIRE(val == 1337U);
          REQUIRE(1337U == val);
          REQUIRE(val == 1337UL);
          REQUIRE(1337UL == val);
          REQUIRE(val == 1337LL);
          REQUIRE(1337LL == val);
          REQUIRE(val == 1337ULL);
          REQUIRE(1337ULL == val);
          static_assert(noexcept(val == 1337), "Dart is misconfigured");
          static_assert(noexcept(1337 == val), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<int, number>::value, "Dart is misconfigured");
        }
      }

      DYNAMIC_WHEN("the value is assigned from a decimal literal", idx) {
        // We're not testing the float point precision of the platform
        // so use something that can be precisely represented anywhere.
        val = 0.5F;
        val = 0.5;
        val = 0.5L;
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(val == 0.5F);
          REQUIRE(0.5F == val);
          REQUIRE(val == 0.5);
          REQUIRE(0.5 == val);
          REQUIRE(val == 0.5L);
          REQUIRE(0.5L == val);
          static_assert(noexcept(val == 0.5), "Dart is misconfigured");
          static_assert(noexcept(0.5 == val), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<double, number>::value, "Dart is misconfigured");
        }
      }
    });
  }

  GIVEN("a default constructed boolean") {
    dart::mutable_flag_api_test([] (auto tag, auto idx) {
      using flag = typename decltype(tag)::type;

      flag val;
      REQUIRE(val.is_boolean());
      REQUIRE(val == false);

      DYNAMIC_WHEN("the value is assigned from a bool literal", idx) {
        val = true;
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(val == true);
          REQUIRE(true == val);
          static_assert(noexcept(val == true), "Dart is misconfigured");
          static_assert(noexcept(true == val), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<bool, flag>::value, "Dart is misconfigured");
        }
      }
    });
  }

  GIVEN("a default constructed null") {
    dart::mutable_null_api_test([] (auto tag, auto idx) {
      using null = typename decltype(tag)::type;

      null val;
      REQUIRE(val.is_null());
      REQUIRE(val == null {});

      DYNAMIC_WHEN("the value is assigned from a nullptr literal", idx) {
        val = nullptr;
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(val == nullptr);
          REQUIRE(nullptr == val);
          static_assert(noexcept(val == nullptr), "Dart is misconfigured");
          static_assert(noexcept(nullptr == val), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<std::nullptr_t, null>::value, "Dart is misconfigured");
        }
      }
    });
  }
}
