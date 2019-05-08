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

SCENARIO("booleans can be created", "[boolean unit]") {
  GIVEN("a boolean") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("bool", dart::heap::make_boolean(true));
      auto boolean = dart::conversion_helper<pkt>(tmp)["bool"];

      REQUIRE(boolean.is_boolean());
      REQUIRE(boolean.get_type() == dart::packet::type::boolean);
      REQUIRE(boolean.boolean());

      DYNAMIC_WHEN("the boolean is finalized", idx) {
        auto new_bool = pkt::make_object("bool", boolean).finalize()["bool"];
        DYNAMIC_THEN("basic properties remain the same", idx) {
          REQUIRE(new_bool.is_boolean());
          REQUIRE(new_bool.get_type() == dart::packet::type::boolean);
          REQUIRE(new_bool.boolean());
        }
      }
    });
  }
}

SCENARIO("booleans can supply a default value", "[boolean unit]") {
  GIVEN("a null object") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto opt = pkt::make_null();

      DYNAMIC_WHEN("retrieving a non-existent boolean", idx) {
        DYNAMIC_THEN("it returns the default", idx) {
          REQUIRE(opt.boolean_or(true));
        }
      }

      DYNAMIC_WHEN("retrieving a boolean", idx) {
        auto new_opt = pkt::make_boolean(false);
        DYNAMIC_THEN("it returns the real value", idx) {
          REQUIRE_FALSE(new_opt.boolean_or(true));
        }
      }
    });
  }
}

SCENARIO("booleans can be compared for equality", "[boolean unit]") {
  GIVEN("three booleans") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      
      auto tmp = dart::heap::make_object("true", true, "false", false);;
      auto bool_one = dart::conversion_helper<pkt>(tmp)["true"];
      auto bool_two = dart::conversion_helper<pkt>(tmp)["true"];
      auto bool_three = dart::conversion_helper<pkt>(tmp)["false"];

      DYNAMIC_WHEN("a boolean is compared against itself", idx) {
        DYNAMIC_THEN("it compares equal", idx) {
          REQUIRE(bool_one == bool_one);
        }

        DYNAMIC_WHEN("that boolean is finalized", idx) {
          auto new_bool_one = pkt::make_object("bool", bool_one).finalize()["bool"];
          DYNAMIC_THEN("it still compares equal to itself", idx) {
            REQUIRE(new_bool_one == new_bool_one);
          }
        }
      }

      DYNAMIC_WHEN("two disparate booleans are compared", idx) {
        DYNAMIC_THEN("their values are compared", idx) {
          REQUIRE(bool_one == bool_two);
          REQUIRE(bool_one != bool_three);
        }

        DYNAMIC_WHEN("they are finalized", idx) {
          auto new_bool_one = pkt::make_object("bool", bool_one).finalize()["bool"];
          auto new_bool_two = pkt::make_object("bool", bool_two).finalize()["bool"];
          auto new_bool_three = pkt::make_object("bool", bool_three).finalize()["bool"];
          DYNAMIC_THEN("they still compare values", idx) {
            REQUIRE(new_bool_one == new_bool_two);
            REQUIRE(new_bool_one != new_bool_three);
          }
        }
      }
    });
  }
}

SCENARIO("booleans cannot be used as an aggregate", "[boolean unit]") {
  GIVEN("a boolean") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("bool", true);
      auto boolean = dart::conversion_helper<pkt>(tmp)["bool"];

      DYNAMIC_WHEN("keys or values are requested", idx) {
        DYNAMIC_THEN("it refuses", idx) {
          REQUIRE_THROWS_AS(boolean.keys(), std::logic_error);
          REQUIRE_THROWS_AS(boolean.values(), std::logic_error);
        }
      }

      DYNAMIC_WHEN("an indexing operation is attempted", idx) {
        DYNAMIC_THEN("it refuses", idx) {
          REQUIRE_THROWS_AS(boolean[0], std::logic_error);
          REQUIRE_THROWS_AS(boolean["oops"], std::logic_error);
        }
      }
    });
  }
}

SCENARIO("naked booleans cannot be finalized", "[boolean unit]") {
  GIVEN("a boolean") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      auto boolean = pkt::make_boolean(true);
      DYNAMIC_WHEN("the boolean is finalized directly", idx) {
        DYNAMIC_THEN("it refuses", idx) {
          REQUIRE_THROWS_AS(boolean.finalize(), std::logic_error);
        }
      }
    });
  }
}
