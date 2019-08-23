/*----- Local Includes -----*/

#include "dart_tests.h"
#include "../include/dart/support/optional.h"

/*---- Type Declarations -----*/

struct move_checker {
  move_checker() : has_value(true) {}
  move_checker(move_checker const&) : has_value(true) {}
  move_checker(move_checker&& other) : has_value(other.has_value) {
    other.has_value = false;
  }

  bool has_value;
};

/*----- Function Implementations -----*/

SCENARIO("optional values can be created", "[optional unit]") {
  GIVEN("a default constructed optional") {
    dart::optional<int> opt;

    THEN("its basic properties make sense") {
      REQUIRE(!opt);
      REQUIRE(!opt.has_value());
      REQUIRE_THROWS_AS(opt.value(), dart::bad_optional_access);
      REQUIRE(opt.value_or(-1) == -1);
    }
  }

  GIVEN("an optional with a value") {
    dart::optional<std::string> opt {"hello world"};

    THEN("its basic properties make sense") {
      REQUIRE(!!opt);
      REQUIRE(opt == "hello world");
      REQUIRE(*opt == "hello world");
      REQUIRE(opt.value() == "hello world");
      REQUIRE(opt.value_or("nope") == "hello world");
    }
  }
}

SCENARIO("optional values can be copied", "[optional unit]") {
  GIVEN("an optional string") {
    dart::optional<std::string> opt {"hello world"};

    WHEN("the string is copied") {
      auto copy = opt;
      THEN("all of its properties are copied") {
        REQUIRE(copy == opt);
        REQUIRE(copy.has_value() == opt.has_value());
        REQUIRE(copy->size() == opt->size());
        REQUIRE(*copy == *opt);
        REQUIRE(!(copy < opt));
        REQUIRE(!(copy > opt));
        REQUIRE(copy <= opt);
        REQUIRE(copy >= opt);
      }
    }
  }
}

SCENARIO("optional values can be moved", "[optional unit]") {
  GIVEN("an optional moveable type") {
    // Emplace directly to avoid a possible initial move.
    dart::optional<move_checker> opt;
    opt.emplace();

    WHEN("the optional is moved") {
      auto moved = std::move(opt);
      THEN("the value is also moved") {
        REQUIRE(moved.has_value());
        REQUIRE(moved->has_value);
        REQUIRE(opt.has_value());
        REQUIRE(!opt->has_value);
      }
    }
  }
}
