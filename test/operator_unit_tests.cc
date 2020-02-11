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
    struct to_dart<my_string> {
      template <class Packet>
      Packet cast(my_string const& str) {
        return Packet::make_string(str.val);
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
      using mset = std::multiset<my_string>;
      using pair = std::pair<my_string, my_string>;
      using span = gsl::span<my_string const>;

      // Get a default constructed instance.
      pkt val;
      REQUIRE(val.is_null());

      // Test std::vector, std::variant, and std::optional assignment/comparison
      DYNAMIC_WHEN("the value is assigned from a vector", idx) {
        auto v = vec {1337, 3.14159, "there are many like it, but this vector is mine", dart::shim::nullopt};
        val = v;
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(val == v);
          REQUIRE(v == val);
          static_assert(noexcept(v == val), "Dart is misconfigured");
          static_assert(noexcept(val == v), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<vec, pkt>::value, "Dart is misconfigured");
        }
      }

      // Test std::deque, std::variant, and std::optional assignment/comparison
      DYNAMIC_WHEN("the value is assigned from a deque", idx) {
        auto d = deq {1337, 3.14159, "there are many like it, but this deque is mine", dart::shim::nullopt};
        val = d;
        DYNAMIC_THEN("it takes on the value we expect", idx) {
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
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(val == a);
          REQUIRE(a == val);
          static_assert(noexcept(a == val), "Dart is misconfigured");
          static_assert(noexcept(val == a), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<arr, pkt>::value, "Dart is misconfigured");
        }
      }

      // Test std::list, std::variant, and std::optional assignment/comparison
      DYNAMIC_WHEN("the value is assigned from a list", idx) {
        auto l = lst {1337, 3.14159, "there are many like it, but this list is mine", dart::shim::nullopt};
        val = l;
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(val == l);
          REQUIRE(l == val);
          static_assert(noexcept(l == val), "Dart is misconfigured");
          static_assert(noexcept(val == l), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<lst, pkt>::value, "Dart is misconfigured");
        }
      }

      // Test std::forward_list, std::variant, and std::optional assignment/comparison
      DYNAMIC_WHEN("the value is assigned from a forward list", idx) {
        auto fl = flst {1337, 3.14159, "there are many like it, but this list is mine", dart::shim::nullopt};
        val = fl;
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(val == fl);
          REQUIRE(fl == val);
          static_assert(noexcept(fl == val), "Dart is misconfigured");
          static_assert(noexcept(val == fl), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<flst, pkt>::value, "Dart is misconfigured");
        }
      }

      // Test std::map assignment/comparison
      DYNAMIC_WHEN("the value is assigned from a map", idx) {
        auto m = map {{"hello", "world"}, {"yes", "no"}};
        val = m;
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(val == m);
          REQUIRE(m == val);
          static_assert(noexcept(m == val), "Dart is misconfigured");
          static_assert(noexcept(val == m), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<map, pkt>::value, "Dart is misconfigured");
        }
      }

      // Test std::unordered_map assignment/comparison
      DYNAMIC_WHEN("the value is assigned from an unordered map", idx) {
        auto um = umap {{"hello", "world"}, {"yes", "no"}};
        val = um;
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(val == um);
          REQUIRE(um == val);
          static_assert(noexcept(um == val), "Dart is misconfigured");
          static_assert(noexcept(val == um), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<umap, pkt>::value, "Dart is misconfigured");
        }
      }

      // Test std::multimap assignment/comparison
      DYNAMIC_WHEN("the value is assigned from a multimap", idx) {
        auto m = mmap {{"hello", "world"}, {"yes", "no"}};
        val = m;
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(val == m);
          REQUIRE(m == val);
          static_assert(noexcept(m == val), "Dart is misconfigured");
          static_assert(noexcept(val == m), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<mmap, pkt>::value, "Dart is misconfigured");
        }
      }

      // Test std::multimap assignment/comparison
      DYNAMIC_WHEN("the value is assigned from a multimap", idx) {
        auto um = ummap {{"hello", "world"}, {"yes", "no"}};
        val = um;
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(val == um);
          REQUIRE(um == val);
          static_assert(noexcept(um == val), "Dart is misconfigured");
          static_assert(noexcept(val == um), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<ummap, pkt>::value, "Dart is misconfigured");
        }
      }

      // Test std::set assignment/comparison
      DYNAMIC_WHEN("the value is assigned from a set", idx) {
        auto s = set {"dark side", "meddle", "the wall", "animals"};
        val = s;
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(val == s);
          REQUIRE(s == val);
          static_assert(noexcept(s == val), "Dart is misconfigured");
          static_assert(noexcept(val == s), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<set, pkt>::value, "Dart is misconfigured");
        }
      }

      // Test std::multiset assignment/comparison
      DYNAMIC_WHEN("the value is assigned from a multiset", idx) {
        auto m = mset {"dark side", "meddle", "meddle", "the wall", "animals"};
        val = m;
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(val == m);
          REQUIRE(m == val);
          static_assert(noexcept(m == val), "Dart is misconfigured");
          static_assert(noexcept(val == m), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<mset, pkt>::value, "Dart is misconfigured");
        }
      }

      DYNAMIC_WHEN("the value is assigned from a pair", idx) {
        auto p = pair {"first", "second"};
        val = p;
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(val == p);
          REQUIRE(p == val);
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
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(val == t);
          REQUIRE(t == val);
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
      using umap =
          std::unordered_map<std::string, dart::shim::optional<dart::shim::variant<int, double, my_string>>>;

      // Get a default constructed instance.
      object val;
      REQUIRE(val.is_object());
      REQUIRE(val.empty());

      // Test std::map assignment/comparison.
      DYNAMIC_WHEN("the value is assigned from a map", idx) {
        auto m = map {{"pi", 3.14159}, {"truth", 42}, {"best album", "dark side"}};
        val = m;
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(val == m);
          REQUIRE(m == val);
          static_assert(noexcept(m == val), "Dart is misconfigured");
          static_assert(noexcept(val == m), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<map, object>::value, "Dart is misconfigured");
        }
      }

      // Test std::unordered_map assignment/comparison.
      DYNAMIC_WHEN("the value is assigned from an unordered map", idx) {
        auto m = umap {{"pi", 3.14159}, {"truth", 42}, {"best album", "dark side"}};
        val = m;
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(val == m);
          REQUIRE(m == val);
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
      using arr = std::array<dart::shim::optional<dart::shim::variant<int, double, my_string>>, 4>;

      array val;
      REQUIRE(val.is_array());
      REQUIRE(val.empty());

      DYNAMIC_WHEN("the value is assigned from a vector", idx) {
        auto v = vec {1337, 3.14159, "there are many like it, but this vector is mine", dart::shim::nullopt};
        val = v;
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(val == v);
          REQUIRE(v == val);
          static_assert(noexcept(v == val), "Dart is misconfigured");
          static_assert(noexcept(val == v), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<vec, array>::value, "Dart is misconfigured");
        }
      }

      DYNAMIC_WHEN("the value is assigned from an array", idx) {
        auto v = arr {{1337, 3.14159, "there are many like it, but this array is mine", dart::shim::nullopt}};
        val = v;
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(val == v);
          REQUIRE(v == val);
          static_assert(noexcept(v == val), "Dart is misconfigured");
          static_assert(noexcept(val == v), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<arr, array>::value, "Dart is misconfigured");
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
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(str == val);
          REQUIRE(val == str);
          static_assert(noexcept(str == val), "Dart is misconfigured");
          static_assert(noexcept(val == str), "Dart is misconfigured");
          static_assert(dart::convert::are_nothrow_comparable<std::string, string>::value, "Dart is misconfigured");
        }
      }

      DYNAMIC_WHEN("the value is assigned from a std::string_view", idx) {
        dart::shim::string_view str = "hello world";
        val = str;
        DYNAMIC_THEN("it takes on the value we expect", idx) {
          REQUIRE(str == val);
          REQUIRE(val == str);
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
