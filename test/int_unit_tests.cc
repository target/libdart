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

SCENARIO("integers can be created", "[integer unit]") {
  GIVEN("an integer") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("int", 1337);
      auto integer = dart::conversion_helper<pkt>(tmp)["int"];

      REQUIRE(integer.is_integer());
      REQUIRE(integer.get_type() == dart::packet::type::integer);
      REQUIRE(integer.integer() == 1337);

      DYNAMIC_WHEN("the integer is finalized", idx) {
        auto new_integer = pkt::make_object("int", integer).finalize()["int"];
        DYNAMIC_THEN("basic properties remain the same", idx) {
          REQUIRE(new_integer.is_integer());
          REQUIRE(new_integer.get_type() == dart::packet::type::integer);
          REQUIRE(new_integer.integer() == 1337);
        }
      }
    });
  }
}

SCENARIO("integers can supply a default value", "[integer unit]") {
  GIVEN("a null object") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto opt = pkt::make_null();
      DYNAMIC_WHEN("retrieving a non-existent integer", idx) {
        DYNAMIC_THEN("it returns the default", idx) {
          REQUIRE(opt.integer_or(1337) == 1337);
        }
      }

      DYNAMIC_WHEN("retrieving an integer", idx) {
        opt = dart::conversion_helper<pkt>(dart::packet::make_integer(28008));
        DYNAMIC_THEN("it returns the real value", idx) {
          REQUIRE(opt.integer_or(1337) == 28008);
        }
      }
    });
  }
}

SCENARIO("integers can be accessed as numeric values", "[integer unit]") {
  GIVEN("an integer") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("int", 1337);
      auto integer = dart::conversion_helper<pkt>(tmp)["int"];
      DYNAMIC_WHEN("the integer is accessed via the numeric call", idx) {
        auto val = integer.numeric();
        auto is_numeric = integer.is_numeric();
        DYNAMIC_THEN("it checks out", idx) {
          REQUIRE(is_numeric);
          REQUIRE(val == 1337.0);
        }
      }
    });
  }
}

SCENARIO("integers can be compared for equality", "[integer unit]") {
  GIVEN("three integers") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("leet", 1337, "doomsday", 2808);
      auto int_one = dart::conversion_helper<pkt>(tmp)["leet"];
      auto int_two = dart::conversion_helper<pkt>(tmp)["leet"];
      auto int_three = dart::conversion_helper<pkt>(tmp)["doomsday"];

      DYNAMIC_WHEN("an integer is compared against itself", idx) {
        DYNAMIC_THEN("it compares equal", idx) {
          REQUIRE(int_one == int_one);
        }

        DYNAMIC_WHEN("that integer is finalized", idx) {
          auto new_int_one = pkt::make_object("int", int_one).finalize()["int"];
          DYNAMIC_THEN("it still compares equal to itseif", idx) {
            REQUIRE(new_int_one == new_int_one);
          }
        }
      }

      DYNAMIC_WHEN("two disparate integers are compared", idx) {
        DYNAMIC_THEN("their values are compared", idx) {
          REQUIRE(int_one == int_two);
          REQUIRE(int_one != int_three);
        }

        DYNAMIC_WHEN("they are finalized", idx) {
          auto new_int_one = pkt::make_object("int", int_one).finalize()["int"];
          auto new_int_two = pkt::make_object("int", int_two).finalize()["int"];
          auto new_int_three = pkt::make_object("int", int_three).finalize()["int"];
          DYNAMIC_THEN("their values are still compared", idx) {
            REQUIRE(new_int_one == new_int_two);
            REQUIRE(new_int_one != new_int_three);
          }
        }
      }
    });
  }
}

SCENARIO("integers cannot be used as an aggregate", "[integer unit]") {
  GIVEN("an integer") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("leet", 1337);
      auto integer = dart::conversion_helper<pkt>(tmp)["leet"];
      DYNAMIC_WHEN("keys or values are requested", idx) {
        DYNAMIC_THEN("it refuses", idx) {
          REQUIRE_THROWS_AS(integer.keys(), std::logic_error);
          REQUIRE_THROWS_AS(integer.values(), std::logic_error);
        }
      }

      DYNAMIC_WHEN("an indexing operation is attempted", idx) {
        DYNAMIC_THEN("it refuses", idx) {
          REQUIRE_THROWS_AS(integer[0], std::logic_error);
          REQUIRE_THROWS_AS(integer["oops"], std::logic_error);
        }
      }
    });
  }
}

SCENARIO("naked integers cannot be finalized", "[integer unit]") {
  GIVEN("a integer") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      auto integer = dart::conversion_helper<pkt>(dart::packet::make_integer(1337));
      DYNAMIC_WHEN("the integer is finalized directly", idx) {
        DYNAMIC_THEN("it refuses", idx) {
          REQUIRE_THROWS_AS(integer.finalize(), std::logic_error);
        }
      }
    });
  }
}
