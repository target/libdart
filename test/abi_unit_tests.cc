/*----- Local Includes -----*/

#include "../include/dart/abi.h"
#include "../include/extern/catch.h"

/*----- Types -----*/

template <class Func>
struct scope_guard : Func {
  ~scope_guard() noexcept {
    try {
      (*this)();
    } catch (...) {
      // Oops...
    }
  }
};

template <class Func>
auto make_scope_guard(Func&& cb) {
  return scope_guard<std::decay_t<Func>> {cb};
}

/*----- Namespace Inclusions -----*/

using namespace std::string_literals;

/*----- Tests -----*/

SCENARIO("objects are regular types", "[abi unit]") {
  GIVEN("a default constructed object") {
    // Get an object, make sure it's cleaned up.
    auto pkt = dart_obj_init();
    auto guard = make_scope_guard([&] { dart_destroy(&pkt); });

    WHEN("the object is queried") {
      THEN("its basic properties make sense") {
        REQUIRE(dart_size(&pkt) == 0);
        REQUIRE(dart_is_obj(&pkt));
        REQUIRE(pkt.rtti.p_id == DART_PACKET);
        REQUIRE(pkt.rtti.rc_id == DART_RC_SAFE);
        REQUIRE(dart_get_type(&pkt) == DART_OBJECT);
      }
    }

    WHEN("keys are inserted") {
      // Insert some values into our object.
      dart_obj_insert_str(&pkt, "hello", "world");
      dart_obj_insert_int(&pkt, "int", 5);
      dart_obj_insert_dcm(&pkt, "pi", 3.14159);
      dart_obj_insert_bool(&pkt, "bool", true);

      THEN("the keys are accessible") {
        // Grab each key for validation, make sure it's cleaned up.
        REQUIRE(dart_size(&pkt) == 4U);
        auto key_one = dart_obj_get(&pkt, "hello");
        auto key_two = dart_obj_get(&pkt, "int");
        auto key_three = dart_obj_get(&pkt, "pi");
        auto key_four = dart_obj_get(&pkt, "bool");
        auto guard = make_scope_guard([&] {
          dart_destroy(&key_one);
          dart_destroy(&key_two);
          dart_destroy(&key_three);
          dart_destroy(&key_four);
        });
        REQUIRE(dart_is_str(&key_one));
        REQUIRE(dart_str_get(&key_one) == "world"s);
        REQUIRE(dart_is_int(&key_two));
        REQUIRE(dart_int_get(&key_two) == 5);
        REQUIRE(dart_is_dcm(&key_three));
        REQUIRE(dart_dcm_get(&key_three) == 3.14159);
        REQUIRE(dart_is_bool(&key_four));
        REQUIRE(dart_bool_get(&key_four) == true);
      }
    }

    WHEN("aggregates are inserted") {
      auto nested = dart_obj_init();
      auto guard = make_scope_guard([&] { dart_destroy(&nested); });
      dart_obj_insert_str(&nested, "a nested", "string");
      dart_obj_insert_dart(&pkt, "nested", &nested);
      THEN("it's recursively queryable") {
        auto nested_copy = dart_obj_get(&pkt, "nested");
        auto nested_str = dart_obj_get(&nested_copy, "a nested");
        auto guard = make_scope_guard([&] {
          dart_destroy(&nested_str);
          dart_destroy(&nested_copy);
        });
        REQUIRE(dart_is_str(&nested_str));
        REQUIRE(dart_str_get(&nested_str) == "string"s);
        REQUIRE(dart_size(&pkt) == 1U);
        REQUIRE(dart_is_obj(&nested_copy));
        REQUIRE(dart_size(&nested_copy) == 1U);
        REQUIRE(dart_equal(&nested_copy, &nested));
      }
    }
  }
}
