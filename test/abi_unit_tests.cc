// XXX: Bool to int conversion warnings are too noisy otherwise
#if DART_USING_MSVC
#pragma warning(push)
#pragma warning(disable: 4805)
#endif

/*----- Local Includes -----*/

#include <string.h>
#include "../include/dart/abi.h"
#include "../include/extern/catch.h"

/*----- Types -----*/

template <class Func>
struct scope_guard : Func {
  scope_guard(scope_guard const&) = default;
  scope_guard(scope_guard&&) = default;
  scope_guard(Func const& cb) : Func(cb) {}
  scope_guard(Func&& cb) : Func(std::move(cb)) {}

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
        REQUIRE(static_cast<bool>(dart_is_obj(&pkt)));
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
        REQUIRE(static_cast<bool>(dart_bool_get(&key_four)) == true);
      }

      WHEN("it's finalized, and split along APIs") {
        dart_packet_t low = dart_lower(&pkt);
        dart_heap_t heap = dart_to_heap(&pkt);
        dart_buffer_t buffer = dart_to_buffer(&pkt);
        THEN("everything plays nicely together") {
          auto low_one = dart_obj_get(&pkt, "hello");
          auto heap_one = dart_obj_get(&heap, "hello");
          auto buffer_one = dart_obj_get(&buffer, "hello");
          auto low_two = dart_obj_get(&pkt, "int");
          auto heap_two = dart_obj_get(&heap, "int");
          auto buffer_two = dart_obj_get(&buffer, "int");
          auto low_three = dart_obj_get(&pkt, "pi");
          auto heap_three = dart_obj_get(&heap, "pi");
          auto buffer_three = dart_obj_get(&buffer, "pi");
          auto low_four = dart_obj_get(&pkt, "bool");
          auto heap_four = dart_obj_get(&heap, "bool");
          auto buffer_four = dart_obj_get(&buffer, "bool");
          auto guard = make_scope_guard([&] {
            dart_destroy(&low_one);
            dart_destroy(&low_two);
            dart_destroy(&low_three);
            dart_destroy(&low_four);
            dart_destroy(&heap_one);
            dart_destroy(&heap_two);
            dart_destroy(&heap_three);
            dart_destroy(&heap_four);
            dart_destroy(&buffer_one);
            dart_destroy(&buffer_two);
            dart_destroy(&buffer_three);
            dart_destroy(&buffer_four);
          });

          REQUIRE(dart_is_finalized(&low));
          REQUIRE(!dart_is_finalized(&heap));
          REQUIRE(dart_is_finalized(&buffer));
          REQUIRE(dart_equal(&low, &heap));
          REQUIRE(dart_equal(&low, &buffer));
          REQUIRE(dart_equal(&heap, &buffer));
          REQUIRE(dart_str_get(&low_one) == "world"s);
          REQUIRE(dart_str_get(&heap_one) == "world"s);
          REQUIRE(dart_str_get(&buffer_one) == "world"s);
          REQUIRE(dart_int_get(&low_two) == 5);
          REQUIRE(dart_int_get(&heap_two) == 5);
          REQUIRE(dart_int_get(&buffer_two) == 5);
          REQUIRE(dart_dcm_get(&low_three) == 3.14159);
          REQUIRE(dart_dcm_get(&heap_three) == 3.14159);
          REQUIRE(dart_dcm_get(&buffer_three) == 3.14159);
          REQUIRE(static_cast<bool>(dart_bool_get(&low_four)) == true);
          REQUIRE(static_cast<bool>(dart_bool_get(&heap_four)) == true);
          REQUIRE(static_cast<bool>(dart_bool_get(&buffer_four)) == true);
        }
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

    WHEN("objects are copied") {
      auto copy = dart_copy(&pkt);
      auto guard = make_scope_guard([&] { dart_destroy(&copy); });

      THEN("it is indistinguishable from the original") {
        REQUIRE(dart_equal(&copy, &pkt));
        REQUIRE(dart_size(&copy) == dart_size(&pkt));
        REQUIRE(dart_get_type(&copy) == dart_get_type(&pkt));
      }

      WHEN("modifications are made") {
        dart_obj_insert_str(&copy, "hello", "world");
        THEN("the two are distinguishable") {
          REQUIRE(!dart_equal(&copy, &pkt));
          REQUIRE(dart_size(&copy) != dart_size(&pkt));
          REQUIRE(dart_get_type(&copy) == dart_get_type(&pkt));
        }
      }
    }

    WHEN("objects are moved") {
      auto moved = dart_move(&pkt);
      auto guard = make_scope_guard([&] { dart_destroy(&moved); });
      THEN("the new object steals the contents of the old") {
        REQUIRE(dart_size(&moved) == 0);
        REQUIRE(dart_is_obj(&moved));
        REQUIRE(dart_get_type(&moved) == DART_OBJECT);
        REQUIRE(!dart_is_obj(&pkt));
        REQUIRE(dart_is_null(&pkt));
        REQUIRE(dart_get_type(&pkt) == DART_NULL);
      }
    }
  }
}

SCENARIO("objects can be constructed with many values", "[abi unit]") {
  GIVEN("many test cases to run") {
    WHEN("an object is constructed with many values") {
      auto* str = "runtime";
      auto obj = dart_obj_init_va("Ssbdi", "Str", str, strlen(str),
          "str", "string", "bool", true, "decimal", 2.99792, "integer", 1337);
      auto guard = make_scope_guard([&] { dart_destroy(&obj); });

      THEN("everything winds up where it's supposed to") {
        auto sized_str = dart_obj_get(&obj, "Str");
        auto str = dart_obj_get(&obj, "str");
        auto boolean = dart_obj_get(&obj, "bool");
        auto decimal = dart_obj_get(&obj, "decimal");
        auto integer = dart_obj_get(&obj, "integer");
        auto guard = make_scope_guard([&] {
          dart_destroy(&integer);
          dart_destroy(&decimal);
          dart_destroy(&boolean);
          dart_destroy(&str);
          dart_destroy(&sized_str);
        });

        REQUIRE(dart_str_get(&sized_str) == "runtime"s);
        REQUIRE(dart_str_get(&str) == "string"s);
        REQUIRE(static_cast<bool>(dart_bool_get(&boolean)) == true);
        REQUIRE(dart_dcm_get(&decimal) == Approx(2.99792));
        REQUIRE(dart_int_get(&integer) == 1337);
      }

      WHEN("that object is cleared") {
        dart_obj_clear(&obj);
        THEN("all key value pairs are gone") {
          REQUIRE(dart_size(&obj) == 0U);

          auto sized_str = dart_obj_get(&obj, "Str");
          auto str = dart_obj_get(&obj, "str");
          auto boolean = dart_obj_get(&obj, "bool");
          auto decimal = dart_obj_get(&obj, "decimal");
          auto integer = dart_obj_get(&obj, "integer");
          auto guard = make_scope_guard([&] {
            dart_destroy(&integer);
            dart_destroy(&decimal);
            dart_destroy(&boolean);
            dart_destroy(&str);
            dart_destroy(&sized_str);
          });

          REQUIRE(dart_is_null(&sized_str));
          REQUIRE(dart_is_null(&str));
          REQUIRE(dart_is_null(&boolean));
          REQUIRE(dart_is_null(&decimal));
          REQUIRE(dart_is_null(&integer));
        }
      }
    }

    WHEN("an object is constructed with many nested objects") {
      auto* str = "runtime";
      auto obj = dart_obj_init_va("Soos,i,as", "str", str, strlen(str),
          "nested", "double_nested", "double_nested_str", "deep", "integer", 10, "arr", "last");
      THEN("everything winds up where it's supposed to") {
        auto str = dart_obj_get(&obj, "str");
        auto nested = dart_obj_get(&obj, "nested");
        auto double_nested = dart_obj_get(&nested, "double_nested");
        auto double_nested_str = dart_obj_get(&double_nested, "double_nested_str");
        auto integer = dart_obj_get(&nested, "integer");
        auto arr = dart_obj_get(&obj, "arr");
        auto last = dart_arr_get(&arr, 0);
        auto guard = make_scope_guard([&] {
          dart_destroy(&last);
          dart_destroy(&arr);
          dart_destroy(&integer);
          dart_destroy(&double_nested_str);
          dart_destroy(&double_nested);
          dart_destroy(&nested);
          dart_destroy(&str);
        });

        REQUIRE(dart_str_get(&str) == "runtime"s);
        REQUIRE(dart_is_obj(&nested));
        REQUIRE(dart_size(&nested) == 2U);
        REQUIRE(dart_is_obj(&double_nested));
        REQUIRE(dart_size(&double_nested) == 1U);
        REQUIRE(dart_str_get(&double_nested_str) == "deep"s);
        REQUIRE(dart_int_get(&integer) == 10);
        REQUIRE(dart_is_arr(&arr));
        REQUIRE(dart_size(&arr) == 1U);
        REQUIRE(dart_str_get(&last) == "last"s);
      }
    }
  }
}

SCENARIO("objects can be iterated over", "[abi unit]") {
  GIVEN("an object with contents") {
    auto* dyn = "dynamic";
    auto obj = dart_obj_init_va("idbsS", "int", 1,
        "decimal", 3.14159, "bool", 0, "str", "fixed", "Str", dyn, strlen(dyn));
    auto guard = make_scope_guard([&] { dart_destroy(&obj); });

    WHEN("we create an iterator") {
      // Initialize an iterator for our array.
      dart_iterator_t it;
      dart_iterator_init_from_err(&it, &obj);

      THEN("it visits all values") {
        REQUIRE(!dart_iterator_done(&it));
        auto one = dart_iterator_get(&it);
        dart_iterator_next(&it);
        auto two = dart_iterator_get(&it);
        dart_iterator_next(&it);
        auto three = dart_iterator_get(&it);
        dart_iterator_next(&it);
        auto four = dart_iterator_get(&it);
        dart_iterator_next(&it);
        auto five = dart_iterator_get(&it);
        dart_iterator_next(&it);
        auto guard = make_scope_guard([&] {
          dart_destroy(&one);
          dart_destroy(&two);
          dart_destroy(&three);
          dart_destroy(&four);
          dart_destroy(&five);
        });
        REQUIRE(dart_iterator_done(&it));
        dart_iterator_destroy(&it);

        REQUIRE(dart_is_str(&one));
        REQUIRE(dart_str_get(&one) == "dynamic"s);
        REQUIRE(dart_is_int(&two));
        REQUIRE(dart_int_get(&two) == 1);
        REQUIRE(dart_is_str(&three));
        REQUIRE(dart_str_get(&three) == "fixed"s);
        REQUIRE(dart_is_bool(&four));
        REQUIRE(dart_bool_get(&four) == false);
        REQUIRE(dart_is_dcm(&five));
        REQUIRE(dart_dcm_get(&five) == 3.14159);
      }
    }

    WHEN("we use automatic iteration") {
      int idx = 0;
      dart_packet_t val;
      auto arr = dart_arr_init_va("Sisbd", dyn, strlen(dyn), 1, "fixed", 0, 3.14159);
      auto guard = make_scope_guard([&] { dart_destroy(&arr); });
      THEN("it visits all values in the expected order") {
        dart_for_each(&obj, &val) {
          // Get the value manually.
          auto verify = dart_arr_get(&arr, idx++);
          auto guard = make_scope_guard([&] { dart_destroy(&verify); });

          // Check it
          REQUIRE(!dart_is_null(&val));
          REQUIRE(!dart_is_null(&verify));
          REQUIRE(dart_get_type(&val) == dart_get_type(&verify));
          REQUIRE(dart_equal(&val, &verify));
        }
      }
    }
  }
}

SCENARIO("arrays can be constructed with many values", "[abi unit]") {
  GIVEN("many test cases to run") {
    WHEN("an array is constructed with many values") {
      auto* str = "runtime";
      auto arr = dart_arr_init_va("Ssbdi",
          str, strlen(str), "string", true, 2.99792, 1337);

      THEN("everything winds up where it's supposed to") {
        auto sized_str = dart_arr_get(&arr, 0);
        auto str = dart_arr_get(&arr, 1);
        auto boolean = dart_arr_get(&arr, 2);
        auto decimal = dart_arr_get(&arr, 3);
        auto integer = dart_arr_get(&arr, 4);
        auto guard = make_scope_guard([&] {
          dart_destroy(&integer);
          dart_destroy(&decimal);
          dart_destroy(&boolean);
          dart_destroy(&str);
          dart_destroy(&sized_str);
        });

        REQUIRE(dart_str_get(&sized_str) == "runtime"s);
        REQUIRE(dart_str_get(&str) == "string"s);
        REQUIRE(static_cast<bool>(dart_bool_get(&boolean)) == true);
        REQUIRE(dart_dcm_get(&decimal) == Approx(2.99792));
        REQUIRE(dart_int_get(&integer) == 1337);
      }

      WHEN("that array is cleared") {
        dart_arr_clear(&arr);
        THEN("all the elements are gone") {
          auto sized_str = dart_arr_get(&arr, 0);
          auto str = dart_arr_get(&arr, 1);
          auto boolean = dart_arr_get(&arr, 2);
          auto decimal = dart_arr_get(&arr, 3);
          auto integer = dart_arr_get(&arr, 4);
          auto guard = make_scope_guard([&] {
            dart_destroy(&integer);
            dart_destroy(&decimal);
            dart_destroy(&boolean);
            dart_destroy(&str);
            dart_destroy(&sized_str);
          });

          REQUIRE(dart_is_null(&sized_str));
          REQUIRE(dart_is_null(&str));
          REQUIRE(dart_is_null(&boolean));
          REQUIRE(dart_is_null(&decimal));
          REQUIRE(dart_is_null(&integer));
        }
      }
    }
  }
}

SCENARIO("arrays can be iterated over", "[abi unit]") {
  GIVEN("an array with contents") {
    auto* dyn = "dynamic";
    auto arr = dart_arr_init_va("idbsS", 1, 3.14159, 0, "fixed", dyn, strlen(dyn));
    auto guard = make_scope_guard([&] { dart_destroy(&arr); });

    WHEN("we create an iterator") {
      // Initialize an iterator for our array.
      dart_iterator_t it;
      dart_iterator_init_from_err(&it, &arr);

      THEN("it visits all values") {
        REQUIRE(!dart_iterator_done(&it));
        auto one = dart_iterator_get(&it);
        dart_iterator_next(&it);
        auto two = dart_iterator_get(&it);
        dart_iterator_next(&it);
        auto three = dart_iterator_get(&it);
        dart_iterator_next(&it);
        auto four = dart_iterator_get(&it);
        dart_iterator_next(&it);
        auto five = dart_iterator_get(&it);
        dart_iterator_next(&it);
        auto guard = make_scope_guard([&] {
          dart_destroy(&one);
          dart_destroy(&two);
          dart_destroy(&three);
          dart_destroy(&four);
          dart_destroy(&five);
        });
        REQUIRE(dart_iterator_done(&it));
        dart_iterator_destroy(&it);

        REQUIRE(dart_is_int(&one));
        REQUIRE(dart_int_get(&one) == 1);
        REQUIRE(dart_is_dcm(&two));
        REQUIRE(dart_dcm_get(&two) == Approx(3.14159));
        REQUIRE(dart_is_bool(&three));
        REQUIRE(dart_bool_get(&three) == false);
        REQUIRE(dart_is_str(&four));
        REQUIRE(dart_str_get(&four) == "fixed"s);
        REQUIRE(dart_str_get(&five) == "dynamic"s);
      }
    }

    WHEN("we use automatic iteration") {
      int idx = 0;
      dart_packet_t val;
      THEN("it visits all values in order") {
        dart_for_each(&arr, &val) {
          // Get the value manually.
          auto verify = dart_arr_get(&arr, idx++);
          auto guard = make_scope_guard([&] { dart_destroy(&verify); });

          // Check it
          REQUIRE(!dart_is_null(&val));
          REQUIRE(!dart_is_null(&verify));
          REQUIRE(dart_get_type(&val) == dart_get_type(&verify));
          REQUIRE(dart_equal(&val, &verify));
        }
      }
    }
  }
}

#if DART_USING_MSVC
#pragma warning(pop)
#endif
