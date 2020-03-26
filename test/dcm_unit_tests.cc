/*----- System Includes -----*/

#include <vector>
#include <string>
#include <limits>
#include <iostream>
#include <algorithm>

/*----- Local Includes -----*/

#include "dart_tests.h"

/*----- Namspace Inclusions -----*/

using namespace dart::literals;

/*----- Function Implementations -----*/

SCENARIO("decimals can be created", "[decimal unit]") {
  GIVEN("a decimal") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("dcm", 3.14159);
      auto decimal = dart::conversion_helper<pkt>(tmp)["dcm"];

      REQUIRE(decimal.is_decimal());
      REQUIRE(decimal.get_type() == dart::packet::type::decimal);
      REQUIRE(decimal.decimal() == 3.14159);

      DYNAMIC_WHEN("the decimal is finalized", idx) {
        auto new_decimal = pkt::make_object("dcm", decimal).finalize()["dcm"];
        DYNAMIC_THEN("basic properties remain the same", idx) {
          REQUIRE(new_decimal.is_decimal());
          REQUIRE(new_decimal.get_type() == dart::packet::type::decimal);
          REQUIRE(new_decimal.decimal() == 3.14159);
        }
      }
    });
  }
}

SCENARIO("decimals can supply a default value", "[decimal unit]") {
  GIVEN("a null object") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto opt = pkt::make_null();

      DYNAMIC_WHEN("retrieving a non-existent decimal", idx) {
        DYNAMIC_THEN("it returns the default", idx) {
          REQUIRE(opt.decimal_or(3.14159) == 3.14159);
        }
      }

      DYNAMIC_WHEN("retrieving a decimal", idx) {
        opt = dart::conversion_helper<pkt>(dart::packet::make_decimal(2.99792));
        DYNAMIC_THEN("it returns the real value", idx) {
          REQUIRE(opt.decimal_or(3.14159) == 2.99792);
        }
      }
    });
  }
}

SCENARIO("decimals can be accessed as numeric values", "[decimal unit]") {
  GIVEN("a decimal") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto dcm = dart::conversion_helper<pkt>(dart::heap::make_object("dcm", 3.14159))["dcm"];
      DYNAMIC_WHEN("the decimal is accessed via the numeric call", idx) {
        auto val = dcm.numeric();
        auto is_numeric = dcm.is_numeric();
        DYNAMIC_THEN("it checks out", idx) {
          REQUIRE(is_numeric);
          REQUIRE(Approx(val) == 3.14159);
        }
      }
    });
  }
}

SCENARIO("numeric values can be supplied with defaults", "[decimal unit]") {
  GIVEN("a null value") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto null = pkt::make_null();
      DYNAMIC_WHEN("the value is accessed via the optional numeric call", idx) {
        auto is_numeric = null.is_numeric();
        auto val = null.numeric_or(std::numeric_limits<double>::quiet_NaN());
        DYNAMIC_THEN("it checks out", idx) {
          REQUIRE_FALSE(is_numeric);
          REQUIRE_FALSE(val == val);
        }
      }
    });
  }
}

SCENARIO("decimals can be compared for equality", "[decimal unit]") {
  GIVEN("three decimals") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("pi", 3.14159, "c", 2.99792);
      auto decimal_one = dart::conversion_helper<pkt>(tmp)["pi"];
      auto decimal_two = dart::conversion_helper<pkt>(tmp)["pi"];
      auto decimal_three = dart::conversion_helper<pkt>(tmp)["c"];

      DYNAMIC_WHEN("a decimal is compared against itself", idx) {
        DYNAMIC_THEN("it compares equal", idx) {
          REQUIRE(decimal_one == decimal_one);
        }

        DYNAMIC_WHEN("that decimal is finalized", idx) {
          auto new_decimal_one = pkt::make_object("dcm", decimal_one).finalize()["dcm"];
          DYNAMIC_THEN("it still compares equal to itself", idx) {
            REQUIRE(new_decimal_one == new_decimal_one);
          }
        }
      }

      DYNAMIC_WHEN("two disparate decimals are compared", idx) {
        DYNAMIC_THEN("their values are compared", idx) {
          REQUIRE(decimal_one == decimal_two);
          REQUIRE(decimal_one != decimal_three);
        }

        DYNAMIC_WHEN("they are finalized", idx) {
          auto new_decimal_one = pkt::make_object("dcm", decimal_one).finalize()["dcm"];
          auto new_decimal_two = pkt::make_object("dcm", decimal_two).finalize()["dcm"];
          auto new_decimal_three = pkt::make_object("dcm", decimal_three).finalize()["dcm"];
          DYNAMIC_THEN("their values are still comapred", idx) {
            REQUIRE(new_decimal_one == new_decimal_two);
            REQUIRE(new_decimal_one != new_decimal_three);
          }
        }
      }
    });
  }
}

SCENARIO("decimals cannot be used as an aggregate", "[decimal unit]") {
  GIVEN("a decimal") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("dcm", 3.14159);
      auto decimal = dart::conversion_helper<pkt>(tmp)["dcm"];
      DYNAMIC_WHEN("keys or values are requested", idx) {
        DYNAMIC_THEN("it refuses", idx) {
          REQUIRE_THROWS_AS(decimal.keys(), std::logic_error);
          REQUIRE_THROWS_AS(decimal.values(), std::logic_error);
        }
      }

      DYNAMIC_WHEN("an indexing operation is attempted", idx) {
        DYNAMIC_THEN("it refuses", idx) {
          REQUIRE_THROWS_AS(decimal[0], std::logic_error);
          REQUIRE_THROWS_AS(decimal["oops"], std::logic_error);
        }
      }
    });
  }
}

SCENARIO("naked decimals cannot be finalized", "[decimal unit]") {
  GIVEN("a decimal") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      auto decimal = pkt::make_decimal(3.14159);
      DYNAMIC_WHEN("the decimal is finalized directly", idx) {
        DYNAMIC_THEN("it refuses", idx) {
          REQUIRE_THROWS_AS(decimal.finalize(), std::logic_error);
        }
      }
    });
  }
}
