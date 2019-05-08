/*----- System Includes -----*/

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

/*----- Local Includes -----*/

#include "dart_tests.h"

/*----- Namespace Inclusions -----*/

using namespace dart::literals;

/*----- Function Implementations -----*/

SCENARIO("nulls can be created", "[null unit]") {
  GIVEN("a null") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto null = pkt::make_null();
      REQUIRE(null.is_null());
      REQUIRE(null.get_type() == dart::packet::type::null);

      DYNAMIC_WHEN("the null is finalized", idx) {
        auto new_null = pkt::make_object("null", null).finalize()["null"];
        DYNAMIC_THEN("basic properties remain the same", idx) {
          REQUIRE(new_null.is_null());
          REQUIRE(new_null.get_type() == dart::packet::type::null);
        }
      }
    });
  }
}

SCENARIO("nulls can be compared for equality", "[null unit]") {
  GIVEN("three nulls") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto null_one = pkt::make_null(), null_two = pkt::make_null(), null_three = pkt::make_null();
      DYNAMIC_WHEN("a null is compared to a null", idx) {
        DYNAMIC_THEN("all nulls are always equal to all nulls", idx) {
          REQUIRE(null_one == null_two);
          REQUIRE(null_two == null_three);
          REQUIRE(null_three == null_one);
        }
      }

      DYNAMIC_WHEN("a null is compared to anything but null", idx) {
        DYNAMIC_THEN("all nulls are not equal to non-nulls", idx) {
          REQUIRE(null_one != 1337);
          REQUIRE(null_two != 3.14159);
          REQUIRE(null_three != "str");
        }
      }
    });
  }
}

SCENARIO("nulls can be checked for existence", "[null unit]") {
  GIVEN("a null") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      auto null = pkt::make_null();
      DYNAMIC_WHEN("the null is converted into a boolean", idx) {
        DYNAMIC_THEN("it converts to false", idx) {
          REQUIRE_FALSE(static_cast<bool>(null));
        }
      }
    });
  }
}

SCENARIO("nulls cannot be used as an aggregate", "[null unit]") {
  GIVEN("a null") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      auto null = pkt::make_null();
      DYNAMIC_WHEN("keys or values are requested", idx) {
        DYNAMIC_THEN("it refuses", idx) {
          REQUIRE_THROWS_AS(null.keys(), std::logic_error);
          REQUIRE_THROWS_AS(null.values(), std::logic_error);
        }
      }

      DYNAMIC_WHEN("an indexing operation is attempted", idx) {
        DYNAMIC_THEN("it refuses", idx) {
          REQUIRE_THROWS_AS(null[0], std::logic_error);
          REQUIRE_THROWS_AS(null["oops"], std::logic_error);
        }
      }
    });
  }
}

SCENARIO("naked nulls cannot be finalized", "[null unit]") {
  GIVEN("a null") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      auto null = pkt::make_null();
      DYNAMIC_WHEN("the null is finalized directly", idx) {
        DYNAMIC_THEN("it refuses", idx) {
          REQUIRE_THROWS_AS(null.finalize(), std::logic_error);
        }
      }
    });
  }
}
