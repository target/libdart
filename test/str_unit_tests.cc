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

SCENARIO("strings can be created", "[string unit]") {
  GIVEN("a string") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("str", "hello world!");
      auto str = dart::conversion_helper<pkt>(tmp)["str"];

      REQUIRE(str.is_str());
      REQUIRE(str.get_type() == dart::packet::type::string);
      REQUIRE(str == "hello world!");
      REQUIRE(str.strv() == "hello world!");

      DYNAMIC_WHEN("the string is finalized", idx) {
        auto new_str = pkt::make_object("str", str).finalize()["str"];
        DYNAMIC_THEN("basic properties remain the same", idx) {
          REQUIRE(new_str.is_str());
          REQUIRE(new_str.get_type() == dart::packet::type::string);
          REQUIRE(new_str == "hello world!");
          REQUIRE(new_str.strv() == "hello world!");
        }
      }
    });
  }

  GIVEN("a very long string") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      std::string stl_str(1 << 20, '!');
      auto str = dart::conversion_helper<pkt>(dart::heap::make_object("str", stl_str))["str"];

      REQUIRE(str.is_str());
      REQUIRE(str.get_type() == dart::packet::type::string);
      REQUIRE(str == stl_str);
      REQUIRE(str.strv() == stl_str);

      DYNAMIC_WHEN("the string is finalized", idx) {
        auto new_str = pkt::make_object("str", str).finalize()["str"];
        DYNAMIC_THEN("basic properties remain the same", idx) {
          REQUIRE(new_str.is_str());
          REQUIRE(new_str.get_type() == dart::packet::type::string);
          REQUIRE(new_str == stl_str);
          REQUIRE(new_str.strv() == stl_str);
        }
      }
    });
  }
}

SCENARIO("strings can supply a default value", "[string unit]") {
  GIVEN("a null object") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto opt = pkt::make_null();
      DYNAMIC_WHEN("retrieving a non-existent string", idx) {
        DYNAMIC_THEN("it returns the default", idx) {
          REQUIRE(opt.strv_or("hello") == "hello");
          REQUIRE_FALSE(strcmp(opt.str_or("hello"), "hello"));
        }
      }

      DYNAMIC_WHEN("retrieving a string", idx) {
        auto new_opt = dart::conversion_helper<pkt>(dart::packet::make_string("goodbye"));
        DYNAMIC_THEN("it returns the real value", idx) {
          REQUIRE(new_opt.strv_or("hello") == "goodbye");
          REQUIRE_FALSE(strcmp(new_opt.str_or("hello"), "goodbye"));
        }
      }
    });
  }
}

SCENARIO("strings can be compared for equality", "[string unit]") {
  GIVEN("three strings") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("one", "one", "three", "three");
      auto str_one = dart::conversion_helper<pkt>(tmp)["one"];
      auto str_two = dart::conversion_helper<pkt>(tmp)["one"];
      auto str_three = dart::conversion_helper<pkt>(tmp)["three"];

      DYNAMIC_WHEN("a string is compared against itself", idx) {
        DYNAMIC_THEN("it compares equal", idx) {
          REQUIRE(str_one == str_one);
        }

        DYNAMIC_WHEN("that string is finalized", idx) {
          auto new_str_one = pkt::make_object("str", str_one).finalize()["str"];
          DYNAMIC_THEN("it still compares equal to itself", idx) {
            REQUIRE(new_str_one == new_str_one);
          }
        }
      }

      DYNAMIC_WHEN("two disparate strings are compared", idx) {
        DYNAMIC_THEN("their values are compared", idx) {
          REQUIRE(str_one == str_two);
          REQUIRE(str_one != str_three);
        }

        DYNAMIC_WHEN("they are finalized", idx) {
          auto new_str_one = pkt::make_object("str", str_one).finalize()["str"];
          auto new_str_two = pkt::make_object("str", str_two).finalize()["str"];
          auto new_str_three = pkt::make_object("str", str_three).finalize()["str"];
          DYNAMIC_THEN("their values are still compared", idx) {
            REQUIRE(new_str_one == new_str_two);
            REQUIRE(new_str_one != new_str_three);
          }
        }
      }
    });
  }
}

SCENARIO("strings cannot be used as an aggregate", "[string unit]") {
  GIVEN("a string") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("str", "hello world!");
      auto str = dart::conversion_helper<pkt>(tmp)["str"];
      DYNAMIC_WHEN("keys or values are requested", idx) {
        DYNAMIC_THEN("it refuses", idx) {
          REQUIRE_THROWS_AS(str.keys(), std::logic_error);
          REQUIRE_THROWS_AS(str.values(), std::logic_error);
        }
      }

      DYNAMIC_WHEN("an indexing operation is attempted", idx) {
        DYNAMIC_THEN("it refuses", idx) {
          REQUIRE_THROWS_AS(str[0], std::logic_error);
          REQUIRE_THROWS_AS(str["oops"], std::logic_error);
        }
      }
    });
  }
}

SCENARIO("naked strings cannot be finalized", "[string unit]") {
  GIVEN("a string") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      auto str = dart::conversion_helper<pkt>(dart::packet::make_string("hello world!"));
      DYNAMIC_WHEN("the string is finalized directly", idx) {
        DYNAMIC_THEN("it refuses", idx) {
          REQUIRE_THROWS_AS(str.finalize(), std::logic_error);
        }
      }
    });
  }
}
