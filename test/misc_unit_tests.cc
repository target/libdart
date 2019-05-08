/*----- System Includes -----*/

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

/*----- Local Includes -----*/

#include "dart_tests.h"

/*----- Namescape Inclusions -----*/

using namespace dart::literals;

/*----- Function Implementations -----*/

SCENARIO("packets can be compared directly to native types", "[misc unit]") {
  WHEN("packets are compared against strings") {
    THEN("they compare underlying values") {
      REQUIRE("hello, world!"_dart == "hello, world!");
      REQUIRE("hello, world!" == "hello, world!"_dart);
      REQUIRE_FALSE("goodbye, world!"_dart == "hello, world!");
      REQUIRE_FALSE("goodbye, world!" == "hello, world!"_dart);
    }
  }

  WHEN("packets are compared against integers") {
    THEN("they compare underlying values") {
      REQUIRE(5_dart == 5);
      REQUIRE(5 == 5_dart);
      REQUIRE_FALSE(5_dart == 6);
      REQUIRE_FALSE(6 == 5_dart);
    }
  }

  WHEN("packets are compared against booleans") {
    THEN("they compare underlying values") {
      REQUIRE(true == dart::packet::make_boolean(true));
      REQUIRE(dart::packet::make_boolean(true) == true);
      REQUIRE_FALSE(false == dart::packet::make_boolean(true));
      REQUIRE_FALSE(dart::packet::make_boolean(true) == false);
    }
  }

  WHEN("packets are compared against nullptr_t literals") {
    THEN("they compare based on null status") {
      REQUIRE(dart::packet::make_null() == nullptr);
      REQUIRE(nullptr == dart::packet::make_null());
      REQUIRE_FALSE(dart::packet::make_boolean(false) == nullptr);
      REQUIRE_FALSE(nullptr == dart::packet::make_boolean(false));
    }
  }
}
