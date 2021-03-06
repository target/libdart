// XXX: Bool to int conversion warnings are too noisy otherwise
#if DART_USING_MSVC
#pragma warning(push)
#pragma warning(disable: 4805)
#endif

/*----- Local Includes -----*/

#include <cstring>
#include <iostream>
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
      std::cerr << "A scope guard block threw an unexpected exception!" << std::endl;
      std::abort();
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

SCENARIO("dart heaps are regular types", "[heap abi unit]") {
  GIVEN("a default constructed object") {
    // Get an object, make sure it's cleaned up.
    auto pkt = dart_heap_obj_init();
    auto guard = make_scope_guard([&] { dart_heap_destroy(&pkt); });

    WHEN("the object is queried") {
      THEN("its basic properties make sense") {
        REQUIRE(dart_heap_size(&pkt) == 0);
        REQUIRE(static_cast<bool>(dart_heap_is_obj(&pkt)));
        REQUIRE(pkt.rtti.p_id == DART_HEAP);
        REQUIRE(pkt.rtti.rc_id == DART_RC_SAFE);
        REQUIRE(dart_heap_get_type(&pkt) == DART_OBJECT);
      }
    }

    WHEN("keys are inserted") {
      // Insert some values into our object.
      dart_heap_obj_insert_str(&pkt, "hello", "world");
      dart_heap_obj_insert_int(&pkt, "int", 5);
      dart_heap_obj_insert_dcm(&pkt, "pi", 3.14159);
      dart_heap_obj_insert_bool(&pkt, "bool", true);

      THEN("the keys are accessible") {
        // Grab each key for validation, make sure it's cleaned up.
        REQUIRE(dart_heap_size(&pkt) == 4U);
        auto key_one = dart_heap_obj_get(&pkt, "hello");
        auto key_two = dart_heap_obj_get_len(&pkt, "int", strlen("int"));
        auto key_three = dart_heap_obj_get(&pkt, "pi");
        auto key_four = dart_heap_obj_get_len(&pkt, "bool", strlen("bool"));
        auto guard = make_scope_guard([&] {
          dart_heap_destroy(&key_one);
          dart_heap_destroy(&key_two);
          dart_heap_destroy(&key_three);
          dart_heap_destroy(&key_four);
        });
        REQUIRE(dart_heap_is_str(&key_one));
        REQUIRE(dart_heap_str_get(&key_one) == "world"s);
        REQUIRE(dart_heap_is_int(&key_two));
        REQUIRE(dart_heap_int_get(&key_two) == 5);
        REQUIRE(dart_heap_is_dcm(&key_three));
        REQUIRE(dart_heap_dcm_get(&key_three) == 3.14159);
        REQUIRE(dart_heap_is_bool(&key_four));
        REQUIRE(dart_heap_bool_get(&key_four) == 1);
      }

      WHEN("it's finalized, and split along APIs") {
        dart_buffer_t low = dart_heap_lower(&pkt);
        auto guard = make_scope_guard([&] { dart_destroy(&low); });
        THEN("everything plays nicely together") {
          auto low_one = dart_obj_get(&low, "hello");
          auto heap_one = dart_heap_obj_get(&pkt, "hello");
          auto low_two = dart_obj_get(&low, "int");
          auto heap_two = dart_heap_obj_get(&pkt, "int");
          auto low_three = dart_obj_get(&low, "pi");
          auto heap_three = dart_heap_obj_get(&pkt, "pi");
          auto low_four = dart_obj_get(&low, "bool");
          auto heap_four = dart_heap_obj_get(&pkt, "bool");
          auto guard = make_scope_guard([&] {
            dart_heap_destroy(&heap_four);
            dart_destroy(&low_four);
            dart_heap_destroy(&heap_three);
            dart_destroy(&low_three);
            dart_heap_destroy(&heap_two);
            dart_destroy(&low_two);
            dart_heap_destroy(&heap_one);
            dart_destroy(&low_one);
          });

          REQUIRE(dart_is_finalized(&low));
          REQUIRE(!dart_is_finalized(&pkt));
          REQUIRE(dart_equal(&low, &pkt));
          REQUIRE(dart_equal(&pkt, &low));
          REQUIRE(dart_str_get(&low_one) == "world"s);
          REQUIRE(dart_heap_str_get(&heap_one) == "world"s);
          REQUIRE(dart_int_get(&low_two) == 5);
          REQUIRE(dart_heap_int_get(&heap_two) == 5);
          REQUIRE(dart_dcm_get(&low_three) == 3.14159);
          REQUIRE(dart_heap_dcm_get(&heap_three) == 3.14159);
          REQUIRE(dart_bool_get(&low_four) == 1);
          REQUIRE(dart_heap_bool_get(&heap_four) == 1);
        }
      }
    }

    WHEN("aggregates are inserted") {
      auto nested = dart_heap_obj_init_rc(DART_RC_SAFE);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&nested); });
      dart_heap_obj_insert_str(&nested, "a nested", "string");
      dart_heap_obj_insert_heap(&pkt, "nested", &nested);
      THEN("it's recursively queryable") {
        auto nested_copy = dart_heap_obj_get(&pkt, "nested");
        auto nested_str = dart_heap_obj_get(&nested_copy, "a nested");
        auto guard = make_scope_guard([&] {
          dart_heap_destroy(&nested_str);
          dart_heap_destroy(&nested_copy);
        });
        REQUIRE(dart_heap_is_str(&nested_str));
        REQUIRE(dart_heap_str_get(&nested_str) == "string"s);
        REQUIRE(dart_heap_size(&pkt) == 1U);
        REQUIRE(dart_heap_is_obj(&nested_copy));
        REQUIRE(dart_heap_size(&nested_copy) == 1U);
        REQUIRE(dart_heap_equal(&nested_copy, &nested));
      }
    }

    WHEN("objects are copied") {
      auto copy = dart_heap_copy(&pkt);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&copy); });

      THEN("it is indistinguishable from the original") {
        REQUIRE(dart_heap_equal(&copy, &pkt));
        REQUIRE(dart_heap_size(&copy) == dart_heap_size(&pkt));
        REQUIRE(dart_heap_get_type(&copy) == dart_heap_get_type(&pkt));
      }

      WHEN("modifications are made") {
        dart_heap_obj_insert_str(&copy, "hello", "world");
        THEN("the two are distinguishable") {
          REQUIRE(!dart_heap_equal(&copy, &pkt));
          REQUIRE(dart_heap_size(&copy) != dart_heap_size(&pkt));
          REQUIRE(dart_heap_get_type(&copy) == dart_heap_get_type(&pkt));
        }
      }
    }

    WHEN("objects are moved") {
      auto moved = dart_heap_move(&pkt);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&moved); });
      THEN("the new object steals the contents of the old") {
        REQUIRE(dart_heap_size(&moved) == 0);
        REQUIRE(dart_heap_is_obj(&moved));
        REQUIRE(dart_heap_get_type(&moved) == DART_OBJECT);
        REQUIRE(!dart_heap_is_obj(&pkt));
        REQUIRE(dart_heap_is_null(&pkt));
        REQUIRE(dart_heap_get_type(&pkt) == DART_NULL);
      }
    }
  }

  GIVEN("a default constructed array") {
    // Get an array, make sure it's cleaned up.
    auto pkt = dart_heap_arr_init();
    auto guard = make_scope_guard([&] { dart_heap_destroy(&pkt); });

    WHEN("the array is queried") {
      THEN("its basic properties make sense") {
        REQUIRE(dart_heap_size(&pkt) == 0);
        REQUIRE(dart_heap_is_arr(&pkt));
        REQUIRE(pkt.rtti.p_id == DART_HEAP);
        REQUIRE(pkt.rtti.rc_id == DART_RC_SAFE);
        REQUIRE(dart_heap_get_type(&pkt) == DART_ARRAY);
      }
    }

    WHEN("arrays are copied") {
      auto copy = dart_heap_copy(&pkt);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&copy); });

      THEN("it is indistinguishable from the original") {
        REQUIRE(dart_heap_equal(&copy, &pkt));
        REQUIRE(dart_heap_size(&copy) == dart_heap_size(&pkt));
        REQUIRE(dart_heap_get_type(&copy) == dart_heap_get_type(&pkt));
      }

      WHEN("modifications are made") {
        dart_heap_arr_insert_str(&copy, 0, "world");
        THEN("the two are distinguishable") {
          REQUIRE(!dart_heap_equal(&copy, &pkt));
          REQUIRE(dart_heap_size(&copy) != dart_heap_size(&pkt));
          REQUIRE(dart_heap_get_type(&copy) == dart_heap_get_type(&pkt));
        }
      }
    }

    WHEN("arrays are moved") {
      auto moved = dart_heap_move(&pkt);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&moved); });
      THEN("the new array steals the contents of the old") {
        REQUIRE(dart_heap_size(&moved) == 0);
        REQUIRE(dart_heap_is_arr(&moved));
        REQUIRE(dart_heap_get_type(&moved) == DART_ARRAY);
        REQUIRE(!dart_heap_is_arr(&pkt));
        REQUIRE(dart_heap_is_null(&pkt));
        REQUIRE(dart_heap_get_type(&pkt) == DART_NULL);
      }
    }
  }

  GIVEN("a default constructed string") {
    // Get an array, make sure it's cleaned up.
    auto pktone = dart_heap_str_init("");
    auto pkttwo = dart_heap_str_init_len("", 0);
    auto pktthree = dart_heap_str_init_rc_len(DART_RC_SAFE, "", 0);
    auto guard = make_scope_guard([&] {
      dart_heap_destroy(&pktthree);
      dart_heap_destroy(&pkttwo);
      dart_heap_destroy(&pktone);
    });

    WHEN("the string is queried") {
      THEN("its basic properties make sense") {
        REQUIRE(dart_heap_size(&pktone) == 0);
        REQUIRE(dart_heap_size(&pkttwo) == 0);
        REQUIRE(dart_heap_size(&pktthree) == 0);
        REQUIRE(dart_heap_is_str(&pktone));
        REQUIRE(dart_heap_is_str(&pkttwo));
        REQUIRE(dart_heap_is_str(&pktthree));
        REQUIRE(dart_heap_str_get(&pktone) == ""s);
        REQUIRE(dart_heap_str_get(&pkttwo) == ""s);
        REQUIRE(dart_heap_str_get(&pktthree) == ""s);
        REQUIRE(pktone.rtti.p_id == DART_HEAP);
        REQUIRE(pkttwo.rtti.p_id == DART_HEAP);
        REQUIRE(pktthree.rtti.p_id == DART_HEAP);
        REQUIRE(pktone.rtti.rc_id == DART_RC_SAFE);
        REQUIRE(pkttwo.rtti.rc_id == DART_RC_SAFE);
        REQUIRE(pktthree.rtti.rc_id == DART_RC_SAFE);
        REQUIRE(dart_heap_get_type(&pktone) == DART_STRING);
        REQUIRE(dart_heap_get_type(&pkttwo) == DART_STRING);
        REQUIRE(dart_heap_get_type(&pktthree) == DART_STRING);
      }
    }

    WHEN("strings are copied") {
      auto copy = dart_heap_copy(&pktone);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&copy); });

      THEN("it is indistinguishable from the original") {
        REQUIRE(dart_heap_equal(&copy, &pktone));
        REQUIRE(dart_heap_size(&copy) == dart_heap_size(&pktone));
        REQUIRE(dart_heap_get_type(&copy) == dart_heap_get_type(&pktone));
      }
    }

    WHEN("strings are moved") {
      auto moved = dart_heap_move(&pktone);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&moved); });
      THEN("the new string steals the contents of the old") {
        REQUIRE(dart_heap_size(&moved) == 0);
        REQUIRE(dart_heap_is_str(&moved));
        REQUIRE(dart_heap_get_type(&moved) == DART_STRING);
        REQUIRE(!dart_heap_is_str(&pktone));
        REQUIRE(dart_heap_is_null(&pktone));
        REQUIRE(dart_heap_get_type(&pktone) == DART_NULL);
      }
    }
  }

  GIVEN("a default constructed integer") {
    // Get an array, make sure it's cleaned up.
    auto pktone = dart_heap_int_init(0);
    auto pkttwo = dart_heap_int_init_rc(DART_RC_SAFE, 0);
    auto guard = make_scope_guard([&] {
      dart_heap_destroy(&pkttwo);
      dart_heap_destroy(&pktone);
    });

    WHEN("the integer is queried") {
      THEN("its basic properties make sense") {
        REQUIRE(dart_heap_is_int(&pktone));
        REQUIRE(dart_heap_is_int(&pkttwo));
        REQUIRE(dart_heap_int_get(&pktone) == 0);
        REQUIRE(dart_heap_int_get(&pkttwo) == 0);
        REQUIRE(pktone.rtti.p_id == DART_HEAP);
        REQUIRE(pkttwo.rtti.p_id == DART_HEAP);
        REQUIRE(pktone.rtti.rc_id == DART_RC_SAFE);
        REQUIRE(pkttwo.rtti.rc_id == DART_RC_SAFE);
        REQUIRE(dart_heap_get_type(&pktone) == DART_INTEGER);
        REQUIRE(dart_heap_get_type(&pkttwo) == DART_INTEGER);
      }
    }

    WHEN("integers are copied") {
      auto copy = dart_heap_copy(&pktone);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&copy); });

      THEN("it is indistinguishable from the original") {
        REQUIRE(dart_heap_equal(&copy, &pktone));
        REQUIRE(dart_heap_get_type(&copy) == dart_heap_get_type(&pktone));
      }
    }

    WHEN("integers are moved") {
      auto moved = dart_heap_move(&pktone);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&moved); });
      THEN("the new integer steals the contents of the old") {
        REQUIRE(dart_heap_is_int(&moved));
        REQUIRE(dart_heap_get_type(&moved) == DART_INTEGER);
        REQUIRE(dart_heap_int_get(&moved) == 0);
        REQUIRE(!dart_heap_is_int(&pktone));
        REQUIRE(dart_heap_is_null(&pktone));
        REQUIRE(dart_heap_get_type(&pktone) == DART_NULL);
      }
    }
  }

  GIVEN("a default constructed decimal") {
    // Get an array, make sure it's cleaned up.
    auto pktone = dart_heap_dcm_init(0.0);
    auto pkttwo = dart_heap_dcm_init_rc(DART_RC_SAFE, 0.0);
    auto guard = make_scope_guard([&] {
      dart_heap_destroy(&pkttwo);
      dart_heap_destroy(&pktone);
    });

    WHEN("the decimal is queried") {
      THEN("its basic properties make sense") {
        REQUIRE(dart_heap_is_dcm(&pktone));
        REQUIRE(dart_heap_dcm_get(&pktone) == 0.0);
        REQUIRE(pktone.rtti.p_id == DART_HEAP);
        REQUIRE(pktone.rtti.rc_id == DART_RC_SAFE);
        REQUIRE(dart_heap_get_type(&pktone) == DART_DECIMAL);
      }
    }

    WHEN("decimals are copied") {
      auto copy = dart_heap_copy(&pktone);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&copy); });

      THEN("it is indistinguishable from the original") {
        REQUIRE(dart_heap_equal(&copy, &pktone));
        REQUIRE(dart_heap_get_type(&copy) == dart_heap_get_type(&pktone));
      }
    }

    WHEN("decimals are moved") {
      auto moved = dart_heap_move(&pktone);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&moved); });
      THEN("the new decimal steals the contents of the old") {
        REQUIRE(dart_heap_is_dcm(&moved));
        REQUIRE(dart_heap_get_type(&moved) == DART_DECIMAL);
        REQUIRE(dart_heap_dcm_get(&moved) == 0.0);
        REQUIRE(!dart_heap_is_dcm(&pktone));
        REQUIRE(dart_heap_is_null(&pktone));
        REQUIRE(dart_heap_get_type(&pktone) == DART_NULL);
      }
    }
  }

  GIVEN("a default constructed boolean") {
    // Get an array, make sure it's cleaned up.
    auto pktone = dart_heap_bool_init(false);
    auto pkttwo = dart_heap_bool_init_rc(DART_RC_SAFE, false);
    auto guard = make_scope_guard([&] {
      dart_heap_destroy(&pkttwo);
      dart_heap_destroy(&pktone);
    });

    WHEN("the bool is queried") {
      THEN("its basic properties make sense") {
        REQUIRE(dart_heap_is_bool(&pktone));
        REQUIRE(dart_heap_bool_get(&pktone) == 0);
        REQUIRE(pktone.rtti.p_id == DART_HEAP);
        REQUIRE(pktone.rtti.rc_id == DART_RC_SAFE);
        REQUIRE(dart_heap_get_type(&pktone) == DART_BOOLEAN);
      }
    }

    WHEN("bools are copied") {
      auto copy = dart_heap_copy(&pktone);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&copy); });

      THEN("it is indistinguishable from the original") {
        REQUIRE(dart_heap_equal(&copy, &pktone));
        REQUIRE(dart_heap_get_type(&copy) == dart_heap_get_type(&pktone));
      }
    }

    WHEN("bools are moved") {
      auto moved = dart_heap_move(&pktone);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&moved); });
      THEN("the new bool steals the contents of the old") {
        REQUIRE(dart_heap_is_bool(&moved));
        REQUIRE(dart_heap_get_type(&moved) == DART_BOOLEAN);
        REQUIRE(dart_heap_bool_get(&moved) == 0);
        REQUIRE(!dart_heap_is_bool(&pktone));
        REQUIRE(dart_heap_is_null(&pktone));
        REQUIRE(dart_heap_get_type(&pktone) == DART_NULL);
      }
    }
  }

  GIVEN("a default constructed null") {
    // Get an array, make sure it's cleaned up.
    auto pkt = dart_heap_null_init();
    auto guard = make_scope_guard([&] { dart_heap_destroy(&pkt); });

    WHEN("the null is queried") {
      THEN("its basic properties make sense") {
        REQUIRE(dart_heap_is_null(&pkt));
        REQUIRE(dart_heap_bool_get(&pkt) == 0);
        REQUIRE(pkt.rtti.p_id == DART_HEAP);
        REQUIRE(pkt.rtti.rc_id == DART_RC_SAFE);
        REQUIRE(dart_heap_get_type(&pkt) == DART_NULL);
      }
    }

    WHEN("the null is copied") {
      auto copy = dart_heap_copy(&pkt);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&copy); });

      THEN("it is indistinguishable from the original") {
        REQUIRE(dart_heap_equal(&copy, &pkt));
        REQUIRE(dart_heap_get_type(&copy) == dart_heap_get_type(&pkt));
      }
    }

    WHEN("the null is moved") {
      auto moved = dart_heap_move(&pkt);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&moved); });
      THEN("null instances are indistinguishable") {
        auto third = dart_heap_init();
        auto fourth = dart_heap_init_rc(DART_RC_SAFE);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&third); });

        REQUIRE(dart_heap_is_null(&moved));
        REQUIRE(dart_heap_is_null(&pkt));
        REQUIRE(dart_heap_equal(&moved, &pkt));
        REQUIRE(dart_equal(&third, &pkt));
        REQUIRE(dart_equal(&third, &moved));
        REQUIRE(dart_equal(&fourth, &pkt));
        REQUIRE(dart_equal(&fourth, &moved));
      }
    }
  }
}

SCENARIO("dart heaps with unsafe reference counting are regular types", "[heap abi unit]") {
  GIVEN("a default constructed object") {
    // Get an object, make sure it's cleaned up.
    auto pkt = dart_heap_obj_init_rc(DART_RC_UNSAFE);
    auto guard = make_scope_guard([&] { dart_heap_destroy(&pkt); });

    WHEN("the object is queried") {
      THEN("its basic properties make sense") {
        REQUIRE(dart_heap_size(&pkt) == 0);
        REQUIRE(static_cast<bool>(dart_heap_is_obj(&pkt)));
        REQUIRE(pkt.rtti.p_id == DART_HEAP);
        REQUIRE(pkt.rtti.rc_id == DART_RC_UNSAFE);
        REQUIRE(dart_heap_get_type(&pkt) == DART_OBJECT);
      }
    }

    WHEN("keys are inserted") {
      // Insert some values into our object.
      dart_heap_obj_insert_str(&pkt, "hello", "world");
      dart_heap_obj_insert_int(&pkt, "int", 5);
      dart_heap_obj_insert_dcm(&pkt, "pi", 3.14159);
      dart_heap_obj_insert_bool(&pkt, "bool", true);

      THEN("the keys are accessible") {
        // Grab each key for validation, make sure it's cleaned up.
        REQUIRE(dart_heap_size(&pkt) == 4U);
        auto key_one = dart_heap_obj_get(&pkt, "hello");
        auto key_two = dart_heap_obj_get_len(&pkt, "int", strlen("int"));
        auto key_three = dart_heap_obj_get(&pkt, "pi");
        auto key_four = dart_heap_obj_get_len(&pkt, "bool", strlen("bool"));
        auto guard = make_scope_guard([&] {
          dart_heap_destroy(&key_one);
          dart_heap_destroy(&key_two);
          dart_heap_destroy(&key_three);
          dart_heap_destroy(&key_four);
        });
        REQUIRE(dart_heap_is_str(&key_one));
        REQUIRE(dart_heap_str_get(&key_one) == "world"s);
        REQUIRE(dart_heap_is_int(&key_two));
        REQUIRE(dart_heap_int_get(&key_two) == 5);
        REQUIRE(dart_heap_is_dcm(&key_three));
        REQUIRE(dart_heap_dcm_get(&key_three) == 3.14159);
        REQUIRE(dart_heap_is_bool(&key_four));
        REQUIRE(dart_heap_bool_get(&key_four) == 1);
      }

      WHEN("it's finalized, and split along APIs") {
        dart_buffer_t low = dart_heap_lower(&pkt);
        auto guard = make_scope_guard([&] { dart_destroy(&low); });
        THEN("everything plays nicely together") {
          auto low_one = dart_obj_get(&low, "hello");
          auto heap_one = dart_heap_obj_get(&pkt, "hello");
          auto low_two = dart_obj_get(&low, "int");
          auto heap_two = dart_heap_obj_get(&pkt, "int");
          auto low_three = dart_obj_get(&low, "pi");
          auto heap_three = dart_heap_obj_get(&pkt, "pi");
          auto low_four = dart_obj_get(&low, "bool");
          auto heap_four = dart_heap_obj_get(&pkt, "bool");
          auto guard = make_scope_guard([&] {
            dart_heap_destroy(&heap_four);
            dart_destroy(&low_four);
            dart_heap_destroy(&heap_three);
            dart_destroy(&low_three);
            dart_heap_destroy(&heap_two);
            dart_destroy(&low_two);
            dart_heap_destroy(&heap_one);
            dart_destroy(&low_one);
          });

          REQUIRE(dart_is_finalized(&low));
          REQUIRE(!dart_is_finalized(&pkt));
          REQUIRE(dart_equal(&low, &pkt));
          REQUIRE(dart_equal(&pkt, &low));
          REQUIRE(dart_str_get(&low_one) == "world"s);
          REQUIRE(dart_heap_str_get(&heap_one) == "world"s);
          REQUIRE(dart_int_get(&low_two) == 5);
          REQUIRE(dart_heap_int_get(&heap_two) == 5);
          REQUIRE(dart_dcm_get(&low_three) == 3.14159);
          REQUIRE(dart_heap_dcm_get(&heap_three) == 3.14159);
          REQUIRE(dart_bool_get(&low_four) == 1);
          REQUIRE(dart_heap_bool_get(&heap_four) == 1);
        }
      }
    }

    WHEN("aggregates are inserted") {
      auto nested = dart_heap_obj_init_rc(DART_RC_UNSAFE);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&nested); });
      dart_heap_obj_insert_str(&nested, "a nested", "string");
      dart_heap_obj_insert_heap(&pkt, "nested", &nested);
      THEN("it's recursively queryable") {
        auto nested_copy = dart_heap_obj_get(&pkt, "nested");
        auto nested_str = dart_heap_obj_get(&nested_copy, "a nested");
        auto guard = make_scope_guard([&] {
          dart_heap_destroy(&nested_str);
          dart_heap_destroy(&nested_copy);
        });
        REQUIRE(dart_heap_is_str(&nested_str));
        REQUIRE(dart_heap_str_get(&nested_str) == "string"s);
        REQUIRE(dart_heap_size(&pkt) == 1U);
        REQUIRE(dart_heap_is_obj(&nested_copy));
        REQUIRE(dart_heap_size(&nested_copy) == 1U);
        REQUIRE(dart_heap_equal(&nested_copy, &nested));
      }
    }

    WHEN("objects are copied") {
      auto copy = dart_heap_copy(&pkt);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&copy); });

      THEN("it is indistinguishable from the original") {
        REQUIRE(dart_heap_equal(&copy, &pkt));
        REQUIRE(dart_heap_size(&copy) == dart_heap_size(&pkt));
        REQUIRE(dart_heap_get_type(&copy) == dart_heap_get_type(&pkt));
      }

      WHEN("modifications are made") {
        dart_heap_obj_insert_str(&copy, "hello", "world");
        THEN("the two are distinguishable") {
          REQUIRE(!dart_heap_equal(&copy, &pkt));
          REQUIRE(dart_heap_size(&copy) != dart_heap_size(&pkt));
          REQUIRE(dart_heap_get_type(&copy) == dart_heap_get_type(&pkt));
        }
      }
    }

    WHEN("objects are moved") {
      auto moved = dart_heap_move(&pkt);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&moved); });
      THEN("the new object steals the contents of the old") {
        REQUIRE(dart_heap_size(&moved) == 0);
        REQUIRE(dart_heap_is_obj(&moved));
        REQUIRE(dart_heap_get_type(&moved) == DART_OBJECT);
        REQUIRE(!dart_heap_is_obj(&pkt));
        REQUIRE(dart_heap_is_null(&pkt));
        REQUIRE(dart_heap_get_type(&pkt) == DART_NULL);
      }
    }
  }

  GIVEN("a default constructed array") {
    // Get an array, make sure it's cleaned up.
    auto pkt = dart_heap_arr_init_rc(DART_RC_UNSAFE);
    auto guard = make_scope_guard([&] { dart_heap_destroy(&pkt); });

    WHEN("the array is queried") {
      THEN("its basic properties make sense") {
        REQUIRE(dart_heap_size(&pkt) == 0);
        REQUIRE(dart_heap_is_arr(&pkt));
        REQUIRE(pkt.rtti.p_id == DART_HEAP);
        REQUIRE(pkt.rtti.rc_id == DART_RC_UNSAFE);
        REQUIRE(dart_heap_get_type(&pkt) == DART_ARRAY);
      }
    }

    WHEN("arrays are copied") {
      auto copy = dart_heap_copy(&pkt);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&copy); });

      THEN("it is indistinguishable from the original") {
        REQUIRE(dart_heap_equal(&copy, &pkt));
        REQUIRE(dart_heap_size(&copy) == dart_heap_size(&pkt));
        REQUIRE(dart_heap_get_type(&copy) == dart_heap_get_type(&pkt));
      }

      WHEN("modifications are made") {
        dart_heap_arr_insert_str(&copy, 0, "world");
        THEN("the two are distinguishable") {
          REQUIRE(!dart_heap_equal(&copy, &pkt));
          REQUIRE(dart_heap_size(&copy) != dart_heap_size(&pkt));
          REQUIRE(dart_heap_get_type(&copy) == dart_heap_get_type(&pkt));
        }
      }
    }

    WHEN("arrays are moved") {
      auto moved = dart_heap_move(&pkt);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&moved); });
      THEN("the new array steals the contents of the old") {
        REQUIRE(dart_heap_size(&moved) == 0);
        REQUIRE(dart_heap_is_arr(&moved));
        REQUIRE(dart_heap_get_type(&moved) == DART_ARRAY);
        REQUIRE(!dart_heap_is_arr(&pkt));
        REQUIRE(dart_heap_is_null(&pkt));
        REQUIRE(dart_heap_get_type(&pkt) == DART_NULL);
      }
    }
  }

  GIVEN("a default constructed string") {
    // Get an array, make sure it's cleaned up.
    auto pktone = dart_heap_str_init_rc(DART_RC_UNSAFE, "");
    auto pkttwo = dart_heap_str_init_rc_len(DART_RC_UNSAFE, "", 0);
    auto pktthree = dart_heap_str_init_rc_len(DART_RC_UNSAFE, "", 0);
    auto guard = make_scope_guard([&] {
      dart_heap_destroy(&pktthree);
      dart_heap_destroy(&pkttwo);
      dart_heap_destroy(&pktone);
    });

    WHEN("the string is queried") {
      THEN("its basic properties make sense") {
        REQUIRE(dart_heap_size(&pktone) == 0);
        REQUIRE(dart_heap_size(&pkttwo) == 0);
        REQUIRE(dart_heap_size(&pktthree) == 0);
        REQUIRE(dart_heap_is_str(&pktone));
        REQUIRE(dart_heap_is_str(&pkttwo));
        REQUIRE(dart_heap_is_str(&pktthree));
        REQUIRE(dart_heap_str_get(&pktone) == ""s);
        REQUIRE(dart_heap_str_get(&pkttwo) == ""s);
        REQUIRE(dart_heap_str_get(&pktthree) == ""s);
        REQUIRE(pktone.rtti.p_id == DART_HEAP);
        REQUIRE(pkttwo.rtti.p_id == DART_HEAP);
        REQUIRE(pktthree.rtti.p_id == DART_HEAP);
        REQUIRE(pktone.rtti.rc_id == DART_RC_UNSAFE);
        REQUIRE(pkttwo.rtti.rc_id == DART_RC_UNSAFE);
        REQUIRE(pktthree.rtti.rc_id == DART_RC_UNSAFE);
        REQUIRE(dart_heap_get_type(&pktone) == DART_STRING);
        REQUIRE(dart_heap_get_type(&pkttwo) == DART_STRING);
        REQUIRE(dart_heap_get_type(&pktthree) == DART_STRING);
      }
    }

    WHEN("strings are copied") {
      auto copy = dart_heap_copy(&pktone);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&copy); });

      THEN("it is indistinguishable from the original") {
        REQUIRE(dart_heap_equal(&copy, &pktone));
        REQUIRE(dart_heap_size(&copy) == dart_heap_size(&pktone));
        REQUIRE(dart_heap_get_type(&copy) == dart_heap_get_type(&pktone));
      }
    }

    WHEN("strings are moved") {
      auto moved = dart_heap_move(&pktone);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&moved); });
      THEN("the new string steals the contents of the old") {
        REQUIRE(dart_heap_size(&moved) == 0);
        REQUIRE(dart_heap_is_str(&moved));
        REQUIRE(dart_heap_get_type(&moved) == DART_STRING);
        REQUIRE(!dart_heap_is_str(&pktone));
        REQUIRE(dart_heap_is_null(&pktone));
        REQUIRE(dart_heap_get_type(&pktone) == DART_NULL);
      }
    }
  }

  GIVEN("a default constructed integer") {
    // Get an array, make sure it's cleaned up.
    auto pktone = dart_heap_int_init_rc(DART_RC_UNSAFE, 0);
    auto pkttwo = dart_heap_int_init_rc(DART_RC_UNSAFE, 0);
    auto guard = make_scope_guard([&] {
      dart_heap_destroy(&pkttwo);
      dart_heap_destroy(&pktone);
    });

    WHEN("the integer is queried") {
      THEN("its basic properties make sense") {
        REQUIRE(dart_heap_is_int(&pktone));
        REQUIRE(dart_heap_is_int(&pkttwo));
        REQUIRE(dart_heap_int_get(&pktone) == 0);
        REQUIRE(dart_heap_int_get(&pkttwo) == 0);
        REQUIRE(pktone.rtti.p_id == DART_HEAP);
        REQUIRE(pkttwo.rtti.p_id == DART_HEAP);
        REQUIRE(pktone.rtti.rc_id == DART_RC_UNSAFE);
        REQUIRE(pkttwo.rtti.rc_id == DART_RC_UNSAFE);
        REQUIRE(dart_heap_get_type(&pktone) == DART_INTEGER);
        REQUIRE(dart_heap_get_type(&pkttwo) == DART_INTEGER);
      }
    }

    WHEN("integers are copied") {
      auto copy = dart_heap_copy(&pktone);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&copy); });

      THEN("it is indistinguishable from the original") {
        REQUIRE(dart_heap_equal(&copy, &pktone));
        REQUIRE(dart_heap_get_type(&copy) == dart_heap_get_type(&pktone));
      }
    }

    WHEN("integers are moved") {
      auto moved = dart_heap_move(&pktone);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&moved); });
      THEN("the new integer steals the contents of the old") {
        REQUIRE(dart_heap_is_int(&moved));
        REQUIRE(dart_heap_get_type(&moved) == DART_INTEGER);
        REQUIRE(dart_heap_int_get(&moved) == 0);
        REQUIRE(!dart_heap_is_int(&pktone));
        REQUIRE(dart_heap_is_null(&pktone));
        REQUIRE(dart_heap_get_type(&pktone) == DART_NULL);
      }
    }
  }

  GIVEN("a default constructed decimal") {
    // Get an array, make sure it's cleaned up.
    auto pktone = dart_heap_dcm_init_rc(DART_RC_UNSAFE, 0.0);
    auto pkttwo = dart_heap_dcm_init_rc(DART_RC_UNSAFE, 0.0);
    auto guard = make_scope_guard([&] {
      dart_heap_destroy(&pkttwo);
      dart_heap_destroy(&pktone);
    });

    WHEN("the decimal is queried") {
      THEN("its basic properties make sense") {
        REQUIRE(dart_heap_is_dcm(&pktone));
        REQUIRE(dart_heap_dcm_get(&pktone) == 0.0);
        REQUIRE(pktone.rtti.p_id == DART_HEAP);
        REQUIRE(pktone.rtti.rc_id == DART_RC_UNSAFE);
        REQUIRE(dart_heap_get_type(&pktone) == DART_DECIMAL);
      }
    }

    WHEN("decimals are copied") {
      auto copy = dart_heap_copy(&pktone);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&copy); });

      THEN("it is indistinguishable from the original") {
        REQUIRE(dart_heap_equal(&copy, &pktone));
        REQUIRE(dart_heap_get_type(&copy) == dart_heap_get_type(&pktone));
      }
    }

    WHEN("decimals are moved") {
      auto moved = dart_heap_move(&pktone);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&moved); });
      THEN("the new decimal steals the contents of the old") {
        REQUIRE(dart_heap_is_dcm(&moved));
        REQUIRE(dart_heap_get_type(&moved) == DART_DECIMAL);
        REQUIRE(dart_heap_dcm_get(&moved) == 0.0);
        REQUIRE(!dart_heap_is_dcm(&pktone));
        REQUIRE(dart_heap_is_null(&pktone));
        REQUIRE(dart_heap_get_type(&pktone) == DART_NULL);
      }
    }
  }

  GIVEN("a default constructed boolean") {
    // Get an array, make sure it's cleaned up.
    auto pktone = dart_heap_bool_init_rc(DART_RC_UNSAFE, false);
    auto pkttwo = dart_heap_bool_init_rc(DART_RC_UNSAFE, false);
    auto guard = make_scope_guard([&] {
      dart_heap_destroy(&pkttwo);
      dart_heap_destroy(&pktone);
    });

    WHEN("the bool is queried") {
      THEN("its basic properties make sense") {
        REQUIRE(dart_heap_is_bool(&pktone));
        REQUIRE(dart_heap_bool_get(&pktone) == 0);
        REQUIRE(pktone.rtti.p_id == DART_HEAP);
        REQUIRE(pktone.rtti.rc_id == DART_RC_UNSAFE);
        REQUIRE(dart_heap_get_type(&pktone) == DART_BOOLEAN);
      }
    }

    WHEN("bools are copied") {
      auto copy = dart_heap_copy(&pktone);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&copy); });

      THEN("it is indistinguishable from the original") {
        REQUIRE(dart_heap_equal(&copy, &pktone));
        REQUIRE(dart_heap_get_type(&copy) == dart_heap_get_type(&pktone));
      }
    }

    WHEN("bools are moved") {
      auto moved = dart_heap_move(&pktone);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&moved); });
      THEN("the new bool steals the contents of the old") {
        REQUIRE(dart_heap_is_bool(&moved));
        REQUIRE(dart_heap_get_type(&moved) == DART_BOOLEAN);
        REQUIRE(dart_heap_bool_get(&moved) == 0);
        REQUIRE(!dart_heap_is_bool(&pktone));
        REQUIRE(dart_heap_is_null(&pktone));
        REQUIRE(dart_heap_get_type(&pktone) == DART_NULL);
      }
    }
  }

  GIVEN("a default constructed null") {
    // Get an array, make sure it's cleaned up.
    auto pkt = dart_heap_null_init_rc(DART_RC_UNSAFE);
    auto guard = make_scope_guard([&] { dart_heap_destroy(&pkt); });

    WHEN("the null is queried") {
      THEN("its basic properties make sense") {
        REQUIRE(dart_heap_is_null(&pkt));
        REQUIRE(dart_heap_bool_get(&pkt) == 0);
        REQUIRE(pkt.rtti.p_id == DART_HEAP);
        REQUIRE(pkt.rtti.rc_id == DART_RC_UNSAFE);
        REQUIRE(dart_heap_get_type(&pkt) == DART_NULL);
      }
    }

    WHEN("the null is copied") {
      auto copy = dart_heap_copy(&pkt);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&copy); });

      THEN("it is indistinguishable from the original") {
        REQUIRE(dart_heap_equal(&copy, &pkt));
        REQUIRE(dart_heap_get_type(&copy) == dart_heap_get_type(&pkt));
      }
    }

    WHEN("the null is moved") {
      auto moved = dart_heap_move(&pkt);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&moved); });
      THEN("null instances are indistinguishable") {
        auto third = dart_heap_init_rc(DART_RC_UNSAFE);
        auto fourth = dart_heap_init_rc(DART_RC_UNSAFE);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&third); });

        REQUIRE(dart_heap_is_null(&moved));
        REQUIRE(dart_heap_is_null(&pkt));
        REQUIRE(dart_heap_equal(&moved, &pkt));
        REQUIRE(dart_equal(&third, &pkt));
        REQUIRE(dart_equal(&third, &moved));
        REQUIRE(dart_equal(&fourth, &pkt));
        REQUIRE(dart_equal(&fourth, &moved));
      }
    }
  }
}

SCENARIO("heap objects can be constructed with many values", "[heap abi unit]") {
  GIVEN("many test cases to run") {
    WHEN("an object is constructed with many values") {
      auto* str = "runtime";
      auto obj = dart_heap_obj_init_va("Ssbdi", "Str", str, strlen(str),
          "str", "string", "bool", true, "decimal", 2.99792, "integer", 1337);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&obj); });

      THEN("everything winds up where it's supposed to") {
        auto sized_str = dart_heap_obj_get(&obj, "Str");
        auto str = dart_heap_obj_get(&obj, "str");
        auto boolean = dart_heap_obj_get(&obj, "bool");
        auto decimal = dart_heap_obj_get(&obj, "decimal");
        auto integer = dart_heap_obj_get(&obj, "integer");
        auto guard = make_scope_guard([&] {
          dart_heap_destroy(&integer);
          dart_heap_destroy(&decimal);
          dart_heap_destroy(&boolean);
          dart_heap_destroy(&str);
          dart_heap_destroy(&sized_str);
        });

        REQUIRE(dart_heap_str_get(&sized_str) == "runtime"s);
        REQUIRE(dart_heap_str_get(&str) == "string"s);
        REQUIRE(dart_heap_bool_get(&boolean) == 1);
        REQUIRE(dart_heap_dcm_get(&decimal) == Approx(2.99792));
        REQUIRE(dart_heap_int_get(&integer) == 1337);
      }

      WHEN("that object is cleared") {
        dart_heap_obj_clear(&obj);
        THEN("all key value pairs are gone") {
          REQUIRE(dart_heap_size(&obj) == 0U);

          auto sized_str = dart_heap_obj_get(&obj, "Str");
          auto str = dart_heap_obj_get(&obj, "str");
          auto boolean = dart_heap_obj_get(&obj, "bool");
          auto decimal = dart_heap_obj_get(&obj, "decimal");
          auto integer = dart_heap_obj_get(&obj, "integer");
          auto guard = make_scope_guard([&] {
            dart_heap_destroy(&integer);
            dart_heap_destroy(&decimal);
            dart_heap_destroy(&boolean);
            dart_heap_destroy(&str);
            dart_heap_destroy(&sized_str);
          });

          REQUIRE(dart_heap_is_null(&sized_str));
          REQUIRE(dart_heap_is_null(&str));
          REQUIRE(dart_heap_is_null(&boolean));
          REQUIRE(dart_heap_is_null(&decimal));
          REQUIRE(dart_heap_is_null(&integer));
        }
      }
    }

    WHEN("an object is constructed with many nested objects") {
      auto* str = "runtime";
      auto obj = dart_heap_obj_init_va("Soos,i,as", "str", str, strlen(str),
          "nested", "double_nested", "double_nested_str", "deep", "integer", 10, "arr", "last");
      auto guard = make_scope_guard([&] { dart_heap_destroy(&obj); });

      THEN("everything winds up where it's supposed to") {
        auto str = dart_heap_obj_get(&obj, "str");
        auto nested = dart_heap_obj_get(&obj, "nested");
        auto double_nested = dart_heap_obj_get(&nested, "double_nested");
        auto double_nested_str = dart_heap_obj_get(&double_nested, "double_nested_str");
        auto integer = dart_heap_obj_get(&nested, "integer");
        auto arr = dart_heap_obj_get(&obj, "arr");
        auto last = dart_heap_arr_get(&arr, 0);
        auto guard = make_scope_guard([&] {
          dart_heap_destroy(&last);
          dart_heap_destroy(&arr);
          dart_heap_destroy(&integer);
          dart_heap_destroy(&double_nested_str);
          dart_heap_destroy(&double_nested);
          dart_heap_destroy(&nested);
          dart_heap_destroy(&str);
        });

        REQUIRE(dart_heap_str_get(&str) == "runtime"s);
        REQUIRE(dart_heap_is_obj(&nested));
        REQUIRE(dart_heap_size(&nested) == 2U);
        REQUIRE(dart_heap_is_obj(&double_nested));
        REQUIRE(dart_heap_size(&double_nested) == 1U);
        REQUIRE(dart_heap_str_get(&double_nested_str) == "deep"s);
        REQUIRE(dart_heap_int_get(&integer) == 10);
        REQUIRE(dart_heap_is_arr(&arr));
        REQUIRE(dart_heap_size(&arr) == 1U);
        REQUIRE(dart_heap_str_get(&last) == "last"s);
      }
    }
  }
}

SCENARIO("heap objects with unsafe refcounting can be constructed with many values", "[heap abi unit]") {
  GIVEN("many test cases to run") {
    WHEN("an object is constructed with many values") {
      auto* str = "runtime";
      auto obj = dart_heap_obj_init_va_rc(DART_RC_UNSAFE, "Ssbdi", "Str", str, strlen(str),
          "str", "string", "bool", true, "decimal", 2.99792, "integer", 1337);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&obj); });

      THEN("everything winds up where it's supposed to") {
        auto sized_str = dart_heap_obj_get(&obj, "Str");
        auto str = dart_heap_obj_get(&obj, "str");
        auto boolean = dart_heap_obj_get(&obj, "bool");
        auto decimal = dart_heap_obj_get(&obj, "decimal");
        auto integer = dart_heap_obj_get(&obj, "integer");
        auto guard = make_scope_guard([&] {
          dart_heap_destroy(&integer);
          dart_heap_destroy(&decimal);
          dart_heap_destroy(&boolean);
          dart_heap_destroy(&str);
          dart_heap_destroy(&sized_str);
        });

        REQUIRE(dart_heap_str_get(&sized_str) == "runtime"s);
        REQUIRE(dart_heap_str_get(&str) == "string"s);
        REQUIRE(dart_heap_bool_get(&boolean) == 1);
        REQUIRE(dart_heap_dcm_get(&decimal) == Approx(2.99792));
        REQUIRE(dart_heap_int_get(&integer) == 1337);
      }

      WHEN("that object is cleared") {
        dart_heap_obj_clear(&obj);
        THEN("all key value pairs are gone") {
          REQUIRE(dart_heap_size(&obj) == 0U);

          auto sized_str = dart_heap_obj_get(&obj, "Str");
          auto str = dart_heap_obj_get(&obj, "str");
          auto boolean = dart_heap_obj_get(&obj, "bool");
          auto decimal = dart_heap_obj_get(&obj, "decimal");
          auto integer = dart_heap_obj_get(&obj, "integer");
          auto guard = make_scope_guard([&] {
            dart_heap_destroy(&integer);
            dart_heap_destroy(&decimal);
            dart_heap_destroy(&boolean);
            dart_heap_destroy(&str);
            dart_heap_destroy(&sized_str);
          });

          REQUIRE(dart_heap_is_null(&sized_str));
          REQUIRE(dart_heap_is_null(&str));
          REQUIRE(dart_heap_is_null(&boolean));
          REQUIRE(dart_heap_is_null(&decimal));
          REQUIRE(dart_heap_is_null(&integer));
        }
      }
    }

    WHEN("an object is constructed with many nested objects") {
      auto* str = "runtime";
      auto obj = dart_heap_obj_init_va_rc(DART_RC_UNSAFE, "Soos,i,as", "str", str, strlen(str),
          "nested", "double_nested", "double_nested_str", "deep", "integer", 10, "arr", "last");
      auto guard = make_scope_guard([&] { dart_heap_destroy(&obj); });

      THEN("everything winds up where it's supposed to") {
        auto str = dart_heap_obj_get(&obj, "str");
        auto nested = dart_heap_obj_get(&obj, "nested");
        auto double_nested = dart_heap_obj_get(&nested, "double_nested");
        auto double_nested_str = dart_heap_obj_get(&double_nested, "double_nested_str");
        auto integer = dart_heap_obj_get(&nested, "integer");
        auto arr = dart_heap_obj_get(&obj, "arr");
        auto last = dart_heap_arr_get(&arr, 0);
        auto guard = make_scope_guard([&] {
          dart_heap_destroy(&last);
          dart_heap_destroy(&arr);
          dart_heap_destroy(&integer);
          dart_heap_destroy(&double_nested_str);
          dart_heap_destroy(&double_nested);
          dart_heap_destroy(&nested);
          dart_heap_destroy(&str);
        });

        REQUIRE(dart_heap_str_get(&str) == "runtime"s);
        REQUIRE(dart_heap_is_obj(&nested));
        REQUIRE(dart_heap_size(&nested) == 2U);
        REQUIRE(dart_heap_is_obj(&double_nested));
        REQUIRE(dart_heap_size(&double_nested) == 1U);
        REQUIRE(dart_heap_str_get(&double_nested_str) == "deep"s);
        REQUIRE(dart_heap_int_get(&integer) == 10);
        REQUIRE(dart_heap_is_arr(&arr));
        REQUIRE(dart_heap_size(&arr) == 1U);
        REQUIRE(dart_heap_str_get(&last) == "last"s);
      }
    }
  }
}

SCENARIO("heap objects can insert any type", "[heap abi unit]") {
  GIVEN("an object to insert into") {
    auto obj = dart_heap_obj_init();
    auto guard = make_scope_guard([&] { dart_heap_destroy(&obj); });

    WHEN("we insert another dart type") {
      auto nested = dart_heap_obj_init_va_rc(DART_RC_SAFE, "ss", "hello", "world", "yes", "no");
      auto guard = make_scope_guard([&] { dart_heap_destroy(&nested); });
      dart_heap_obj_insert_heap(&obj, "nested", &nested);

      THEN("the object is reachable, and the original copy is preserved") {
        auto grabbed = dart_heap_obj_get(&obj, "nested");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&grabbed); });

        REQUIRE(dart_heap_is_obj(&nested));
        REQUIRE(dart_heap_is_obj(&grabbed));
        REQUIRE(dart_heap_size(&nested) == 2U);
        REQUIRE(dart_heap_size(&grabbed) == 2U);
        REQUIRE(dart_heap_equal(&nested, &grabbed));
        REQUIRE(dart_heap_get_type(&nested) == DART_OBJECT);
        REQUIRE(dart_heap_get_type(&grabbed) == DART_OBJECT);
      }
    }

    WHEN("we take another dart type") {
      dart_heap_t nested;
      dart_heap_obj_init_va_err(&nested, "ss", "hello", "world", "yes", "no");
      auto guard = make_scope_guard([&] { dart_heap_destroy(&nested); });
      dart_heap_obj_insert_take_heap(&obj, "nested", &nested);

      THEN("the object is reachable, and the original copy is reset to null") {
        auto grabbed = dart_heap_obj_get(&obj, "nested");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&grabbed); });

        REQUIRE(dart_heap_is_obj(&grabbed));
        // REQUIRE(dart_heap_is_null(&nested)); <-- Unfortunately doesn't actually happen at the moment.
        REQUIRE(dart_heap_size(&grabbed) == 2U);
        REQUIRE(dart_heap_get_type(&grabbed) == DART_OBJECT);
        // REQUIRE(dart_heap_get_type(&nested) == DART_NULL); <-- Unfortuantely doesn't actually happen at the moment
      }
    }

    WHEN("we insert a string") {
      dart_heap_obj_insert_str(&obj, "key", "value");
      THEN("the string is reachable and has the correct value") {
        auto str = dart_heap_obj_get(&obj, "key");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&str); });

        REQUIRE(dart_heap_is_str(&str));
        REQUIRE(dart_heap_size(&str) == strlen("value"));
        REQUIRE(dart_heap_str_get(&str) == "value"s);
      }
    }

    WHEN("we insert an integer") {
      dart_heap_obj_insert_int(&obj, "int", 6);
      THEN("the integer is reachable and has the correct value") {
        auto integer = dart_heap_obj_get(&obj, "int");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&integer); });

        REQUIRE(dart_heap_is_int(&integer));
        REQUIRE(dart_heap_int_get(&integer) == 6);
      }
    }

    WHEN("we insert a decimal") {
      dart_heap_obj_insert_dcm(&obj, "pi", 3.14159);
      THEN("the decimal is reachable and has the correct value") {
        auto dcm = dart_heap_obj_get(&obj, "pi");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&dcm); });

        REQUIRE(dart_heap_is_dcm(&dcm));
        REQUIRE(dart_heap_dcm_get(&dcm) == 3.14159);
      }
    }

    WHEN("we insert a boolean") {
      dart_heap_obj_insert_bool(&obj, "truth", true);
      THEN("the boolean is reachable and has the correct value") {
        auto boolean = dart_heap_obj_get(&obj, "truth");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&boolean); });

        REQUIRE(dart_heap_is_bool(&boolean));
        REQUIRE(dart_heap_bool_get(&boolean) == 1);
      }
    }

    WHEN("we insert a null") {
      dart_heap_obj_insert_null(&obj, "none");
      THEN("the null is reachable") {
        auto null = dart_heap_obj_get(&obj, "none");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&null); });

        REQUIRE(dart_heap_is_null(&null));
        REQUIRE(dart_heap_obj_has_key(&obj, "none"));
        REQUIRE(dart_heap_get_type(&null) == DART_NULL);
      }
    }
  }
}

SCENARIO("heap objects with unsafe refcounting can insert any type", "[heap abi unit]") {
  GIVEN("an object to insert into") {
    auto obj = dart_heap_obj_init_rc(DART_RC_UNSAFE);
    auto guard = make_scope_guard([&] { dart_heap_destroy(&obj); });

    WHEN("we insert another dart type") {
      auto nested = dart_heap_obj_init_va_rc(DART_RC_UNSAFE, "ss", "hello", "world", "yes", "no");
      auto guard = make_scope_guard([&] { dart_heap_destroy(&nested); });
      dart_heap_obj_insert_heap(&obj, "nested", &nested);

      THEN("the object is reachable, and the original copy is preserved") {
        auto grabbed = dart_heap_obj_get(&obj, "nested");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&grabbed); });

        REQUIRE(dart_heap_is_obj(&nested));
        REQUIRE(dart_heap_is_obj(&grabbed));
        REQUIRE(dart_heap_size(&nested) == 2U);
        REQUIRE(dart_heap_size(&grabbed) == 2U);
        REQUIRE(dart_heap_equal(&nested, &grabbed));
        REQUIRE(dart_heap_get_type(&nested) == DART_OBJECT);
        REQUIRE(dart_heap_get_type(&grabbed) == DART_OBJECT);
      }
    }

    WHEN("we take another dart type") {
      dart_heap_t nested;
      dart_heap_obj_init_va_rc_err(&nested, DART_RC_UNSAFE, "ss", "hello", "world", "yes", "no");
      auto guard = make_scope_guard([&] { dart_heap_destroy(&nested); });
      dart_heap_obj_insert_take_heap(&obj, "nested", &nested);

      THEN("the object is reachable, and the original copy is reset to null") {
        auto grabbed = dart_heap_obj_get(&obj, "nested");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&grabbed); });

        REQUIRE(dart_heap_is_obj(&grabbed));
        // REQUIRE(dart_heap_is_null(&nested)); <-- Unfortunately doesn't actually happen at the moment.
        REQUIRE(dart_heap_size(&grabbed) == 2U);
        REQUIRE(dart_heap_get_type(&grabbed) == DART_OBJECT);
        // REQUIRE(dart_heap_get_type(&nested) == DART_NULL); <-- Unfortuantely doesn't actually happen at the moment
      }
    }

    WHEN("we insert a string") {
      dart_heap_obj_insert_str(&obj, "key", "value");
      THEN("the string is reachable and has the correct value") {
        auto str = dart_heap_obj_get(&obj, "key");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&str); });

        REQUIRE(dart_heap_is_str(&str));
        REQUIRE(dart_heap_size(&str) == strlen("value"));
        REQUIRE(dart_heap_str_get(&str) == "value"s);
      }
    }

    WHEN("we insert an integer") {
      dart_heap_obj_insert_int(&obj, "int", 6);
      THEN("the integer is reachable and has the correct value") {
        auto integer = dart_heap_obj_get(&obj, "int");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&integer); });

        REQUIRE(dart_heap_is_int(&integer));
        REQUIRE(dart_heap_int_get(&integer) == 6);
      }
    }

    WHEN("we insert a decimal") {
      dart_heap_obj_insert_dcm(&obj, "pi", 3.14159);
      THEN("the decimal is reachable and has the correct value") {
        auto dcm = dart_heap_obj_get(&obj, "pi");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&dcm); });

        REQUIRE(dart_heap_is_dcm(&dcm));
        REQUIRE(dart_heap_dcm_get(&dcm) == 3.14159);
      }
    }

    WHEN("we insert a boolean") {
      dart_heap_obj_insert_bool(&obj, "truth", true);
      THEN("the boolean is reachable and has the correct value") {
        auto boolean = dart_heap_obj_get(&obj, "truth");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&boolean); });

        REQUIRE(dart_heap_is_bool(&boolean));
        REQUIRE(dart_heap_bool_get(&boolean) == 1);
      }
    }

    WHEN("we insert a null") {
      dart_heap_obj_insert_null(&obj, "none");
      THEN("the null is reachable") {
        auto null = dart_heap_obj_get(&obj, "none");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&null); });

        REQUIRE(dart_heap_is_null(&null));
        REQUIRE(dart_heap_obj_has_key(&obj, "none"));
        REQUIRE(dart_heap_get_type(&null) == DART_NULL);
      }
    }
  }
}

SCENARIO("heap objects can assign to existing indices", "[heap abi unit]") {
  GIVEN("an object with existing values") {
    auto obj = dart_heap_obj_init_va("os,sidbn", "nested", "yes",
        "no", "hello", "world", "age", 27, "c", 2.99792, "lies", false, "none");
    auto guard = make_scope_guard([&] { dart_heap_destroy(&obj); });

    WHEN("the nested object is assigned to") {
      dart_heap_t nested;
      dart_heap_obj_init_va_rc_err(&nested, DART_RC_SAFE, "s", "stop", "go");
      auto guard = make_scope_guard([&] { dart_heap_destroy(&nested); });
      dart_heap_obj_set_heap(&obj, "nested", &nested);
      THEN("it takes on the value we expect") {
        auto nes = dart_heap_obj_get(&obj, "nested");
        auto str = dart_heap_obj_get(&nes, "stop");
        auto guard = make_scope_guard([&] {
          dart_heap_destroy(&str);
          dart_heap_destroy(&nes);
        });
        REQUIRE(dart_heap_is_obj(&nes));
        REQUIRE(dart_heap_is_obj(&nested));
        REQUIRE(dart_heap_size(&nes) == 1U);
        REQUIRE(dart_heap_size(&nested) == 1U);
        REQUIRE(dart_heap_equal(&nested, &nes));
        REQUIRE(dart_heap_str_get(&str) == "go"s);
      }
    }

    WHEN("the nested object is move assigned to") {
      auto nested = dart_heap_obj_init_va("s", "stop", "go");
      auto guard = make_scope_guard([&] { dart_heap_destroy(&nested); });
      dart_heap_obj_set_take_heap(&obj, "nested", &nested);
      THEN("it takes on the value we expect, and resets the original to null") {
        auto nes = dart_heap_obj_get(&obj, "nested");
        auto str = dart_heap_obj_get(&nes, "stop");
        auto guard = make_scope_guard([&] {
          dart_heap_destroy(&str);
          dart_heap_destroy(&nes);
        });
        REQUIRE(dart_heap_is_obj(&nes));
        REQUIRE(dart_heap_is_null(&nested));
        REQUIRE(dart_heap_size(&nes) == 1U);
        REQUIRE(!dart_heap_equal(&nested, &nes));
        REQUIRE(dart_heap_str_get(&str) == "go"s);
      }
    }

    WHEN("the nested object is assigned to from a disparate type") {
      dart_heap_obj_set_null(&obj, "nested");
      THEN("it takes on the value we expect") {
        auto prevobj = dart_heap_obj_get(&obj, "nested");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&prevobj); });
        REQUIRE(dart_heap_is_null(&prevobj));
        REQUIRE(dart_heap_obj_has_key(&obj, "nested"));
      }
    }

    WHEN("the string value is assigned to") {
      dart_heap_obj_set_str(&obj, "hello", "life");
      THEN("it takes on the value we expect") {
        auto str = dart_heap_obj_get(&obj, "hello");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&str); });
        REQUIRE(dart_heap_is_str(&str));
        REQUIRE(dart_heap_size(&str) == strlen("life"));
        REQUIRE(dart_heap_str_get(&str) == "life"s);
      }
    }

    WHEN("the string value is assigned from a disparate type") {
      dart_heap_obj_set_bool(&obj, "hello", true);
      THEN("it takes on the value we expect") {
        auto prevstr = dart_heap_obj_get(&obj, "hello");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&prevstr); });

        REQUIRE(dart_heap_is_bool(&prevstr));
        REQUIRE(dart_heap_bool_get(&prevstr));
      }
    }

    WHEN("the integer value is assigned to") {
      dart_heap_obj_set_int(&obj, "age", 72);
      THEN("it takes on the value we expect") {
        auto integer = dart_heap_obj_get(&obj, "age");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&integer); });
        REQUIRE(dart_heap_is_int(&integer));
        REQUIRE(dart_heap_int_get(&integer) == 72);
      }
    }

    WHEN("the integer value is assigned from a disparate type") {
      dart_heap_obj_set_dcm(&obj, "age", 27.5);
      THEN("it takes on the value we expect") {
        auto prevint = dart_heap_obj_get(&obj, "age");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&prevint); });
        REQUIRE(dart_heap_is_dcm(&prevint));
        REQUIRE(dart_heap_dcm_get(&prevint) == 27.5);
      }
    }

    WHEN("the decimal value is assigned to") {
      dart_heap_obj_set_dcm(&obj, "c", 3.0);
      THEN("it takes on the value we expect") {
        auto dcm = dart_heap_obj_get(&obj, "c");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&dcm); });
        REQUIRE(dart_heap_is_dcm(&dcm));
        REQUIRE(dart_heap_dcm_get(&dcm) == 3.0);
      }
    }

    WHEN("the decimal value is assigned from a disparate type") {
      dart_heap_obj_set_int(&obj, "c", 3);
      THEN("it takes on the value we expect") {
        auto prevdcm = dart_heap_obj_get(&obj, "c");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&prevdcm); });
        REQUIRE(dart_heap_is_int(&prevdcm));
        REQUIRE(dart_heap_int_get(&prevdcm) == 3);
      }
    }

    WHEN("the boolean value is assigned to") {
      dart_heap_obj_set_bool(&obj, "lies", true);
      THEN("it takes on the value we expect") {
        auto boolean = dart_heap_obj_get(&obj, "lies");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&boolean); });
        REQUIRE(dart_heap_is_bool(&boolean));
        REQUIRE(dart_heap_bool_get(&boolean) == 1);
      }
    }

    WHEN("the boolean value is assigned to from a disparate type") {
      dart_heap_obj_set_str(&obj, "lies", "true");
      THEN("it takes on the value we expect") {
        auto prevbool = dart_heap_obj_get(&obj, "lies");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&prevbool); });
        REQUIRE(dart_heap_is_str(&prevbool));
        REQUIRE(dart_heap_str_get(&prevbool) == "true"s);
      }
    }

    WHEN("the null is assigned to") {
      dart_heap_obj_set_null(&obj, "none");
      THEN("it retains the value we expect") {
        auto null = dart_heap_obj_get(&obj, "none");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&null); });
        REQUIRE(dart_heap_is_null(&null));
      }
    }

    WHEN("the null is assigned to from a disparate type") {
      auto nested = dart_heap_obj_init_va("sss", "hello", "world", "yes", "no", "stop", "go");
      dart_heap_obj_set_take_heap(&obj, "none", &nested);
      dart_heap_destroy(&nested);
      THEN("it takes on the value we expect") {
        auto nes = dart_heap_obj_get(&obj, "none");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&nes); });
        REQUIRE(dart_heap_is_obj(&nes));
        REQUIRE(dart_heap_size(&nes) == 3U);
      }
    }
  }
}

SCENARIO("heap objects with unsafe refcounting can assign to existing indices", "[heap abi unit]") {
  GIVEN("an object with existing values") {
    auto obj = dart_heap_obj_init_va_rc(DART_RC_UNSAFE, "os,sidbn", "nested", "yes",
        "no", "hello", "world", "age", 27, "c", 2.99792, "lies", false, "none");
    auto guard = make_scope_guard([&] { dart_heap_destroy(&obj); });

    WHEN("the nested object is assigned to") {
      dart_heap_t nested;
      dart_heap_obj_init_va_rc_err(&nested, DART_RC_UNSAFE, "s", "stop", "go");
      auto guard = make_scope_guard([&] { dart_heap_destroy(&nested); });
      dart_heap_obj_set_heap(&obj, "nested", &nested);
      THEN("it takes on the value we expect") {
        auto nes = dart_heap_obj_get(&obj, "nested");
        auto str = dart_heap_obj_get(&nes, "stop");
        auto guard = make_scope_guard([&] {
          dart_heap_destroy(&str);
          dart_heap_destroy(&nes);
        });
        REQUIRE(dart_heap_is_obj(&nes));
        REQUIRE(dart_heap_is_obj(&nested));
        REQUIRE(dart_heap_size(&nes) == 1U);
        REQUIRE(dart_heap_size(&nested) == 1U);
        REQUIRE(dart_heap_equal(&nested, &nes));
        REQUIRE(dart_heap_str_get(&str) == "go"s);
      }
    }

    WHEN("the nested object is move assigned to") {
      auto nested = dart_heap_obj_init_va_rc(DART_RC_UNSAFE, "s", "stop", "go");
      auto guard = make_scope_guard([&] { dart_heap_destroy(&nested); });
      dart_heap_obj_set_take_heap(&obj, "nested", &nested);
      THEN("it takes on the value we expect, and resets the original to null") {
        auto nes = dart_heap_obj_get(&obj, "nested");
        auto str = dart_heap_obj_get(&nes, "stop");
        auto guard = make_scope_guard([&] {
          dart_heap_destroy(&str);
          dart_heap_destroy(&nes);
        });
        REQUIRE(dart_heap_is_obj(&nes));
        REQUIRE(dart_heap_is_null(&nested));
        REQUIRE(dart_heap_size(&nes) == 1U);
        REQUIRE(!dart_heap_equal(&nested, &nes));
        REQUIRE(dart_heap_str_get(&str) == "go"s);
      }
    }

    WHEN("the nested object is assigned to from a disparate type") {
      dart_heap_obj_set_null(&obj, "nested");
      THEN("it takes on the value we expect") {
        auto prevobj = dart_heap_obj_get(&obj, "nested");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&prevobj); });
        REQUIRE(dart_heap_is_null(&prevobj));
        REQUIRE(dart_heap_obj_has_key(&obj, "nested"));
      }
    }

    WHEN("the string value is assigned to") {
      dart_heap_obj_set_str(&obj, "hello", "life");
      THEN("it takes on the value we expect") {
        auto str = dart_heap_obj_get(&obj, "hello");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&str); });
        REQUIRE(dart_heap_is_str(&str));
        REQUIRE(dart_heap_size(&str) == strlen("life"));
        REQUIRE(dart_heap_str_get(&str) == "life"s);
      }
    }

    WHEN("the string value is assigned from a disparate type") {
      dart_heap_obj_set_bool(&obj, "hello", true);
      THEN("it takes on the value we expect") {
        auto prevstr = dart_heap_obj_get(&obj, "hello");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&prevstr); });

        REQUIRE(dart_heap_is_bool(&prevstr));
        REQUIRE(dart_heap_bool_get(&prevstr));
      }
    }

    WHEN("the integer value is assigned to") {
      dart_heap_obj_set_int(&obj, "age", 72);
      THEN("it takes on the value we expect") {
        auto integer = dart_heap_obj_get(&obj, "age");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&integer); });
        REQUIRE(dart_heap_is_int(&integer));
        REQUIRE(dart_heap_int_get(&integer) == 72);
      }
    }

    WHEN("the integer value is assigned from a disparate type") {
      dart_heap_obj_set_dcm(&obj, "age", 27.5);
      THEN("it takes on the value we expect") {
        auto prevint = dart_heap_obj_get(&obj, "age");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&prevint); });
        REQUIRE(dart_heap_is_dcm(&prevint));
        REQUIRE(dart_heap_dcm_get(&prevint) == 27.5);
      }
    }

    WHEN("the decimal value is assigned to") {
      dart_heap_obj_set_dcm(&obj, "c", 3.0);
      THEN("it takes on the value we expect") {
        auto dcm = dart_heap_obj_get(&obj, "c");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&dcm); });
        REQUIRE(dart_heap_is_dcm(&dcm));
        REQUIRE(dart_heap_dcm_get(&dcm) == 3.0);
      }
    }

    WHEN("the decimal value is assigned from a disparate type") {
      dart_heap_obj_set_int(&obj, "c", 3);
      THEN("it takes on the value we expect") {
        auto prevdcm = dart_heap_obj_get(&obj, "c");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&prevdcm); });
        REQUIRE(dart_heap_is_int(&prevdcm));
        REQUIRE(dart_heap_int_get(&prevdcm) == 3);
      }
    }

    WHEN("the boolean value is assigned to") {
      dart_heap_obj_set_bool(&obj, "lies", true);
      THEN("it takes on the value we expect") {
        auto boolean = dart_heap_obj_get(&obj, "lies");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&boolean); });
        REQUIRE(dart_heap_is_bool(&boolean));
        REQUIRE(dart_heap_bool_get(&boolean) == 1);
      }
    }

    WHEN("the boolean value is assigned to from a disparate type") {
      dart_heap_obj_set_str(&obj, "lies", "true");
      THEN("it takes on the value we expect") {
        auto prevbool = dart_heap_obj_get(&obj, "lies");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&prevbool); });
        REQUIRE(dart_heap_is_str(&prevbool));
        REQUIRE(dart_heap_str_get(&prevbool) == "true"s);
      }
    }

    WHEN("the null is assigned to") {
      dart_heap_obj_set_null(&obj, "none");
      THEN("it retains the value we expect") {
        auto null = dart_heap_obj_get(&obj, "none");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&null); });
        REQUIRE(dart_heap_is_null(&null));
      }
    }

    WHEN("the null is assigned to from a disparate type") {
      auto nested = dart_heap_obj_init_va_rc(DART_RC_UNSAFE, "sss", "hello", "world", "yes", "no", "stop", "go");
      dart_heap_obj_set_take_heap(&obj, "none", &nested);
      dart_heap_destroy(&nested);
      THEN("it takes on the value we expect") {
        auto nes = dart_heap_obj_get(&obj, "none");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&nes); });
        REQUIRE(dart_heap_is_obj(&nes));
        REQUIRE(dart_heap_size(&nes) == 3U);
      }
    }
  }
}

SCENARIO("heap objects can erase existing indices", "[heap abi unit]") {
  GIVEN("an object with existing values") {
    auto obj = dart_heap_obj_init_va("sidbn", "hello", "world", "age", 27, "c", 2.99792, "lies", false, "none");
    auto guard = make_scope_guard([&] { dart_heap_destroy(&obj); });

    WHEN("the string value is erased") {
      dart_heap_obj_erase(&obj, "hello");
      THEN("it takes on the value we expect") {
        auto str = dart_heap_obj_get(&obj, "hello");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&str); });
        REQUIRE(dart_heap_is_null(&str));
      }
    }

    WHEN("the integer value is assigned to") {
      dart_heap_obj_erase(&obj, "age");
      THEN("it takes on the value we expect") {
        auto integer = dart_heap_obj_get(&obj, "age");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&integer); });
        REQUIRE(dart_heap_is_null(&integer));
      }
    }

    WHEN("the decimal value is assigned to") {
      dart_heap_obj_erase(&obj, "c");
      THEN("it takes on the value we expect") {
        auto dcm = dart_heap_obj_get(&obj, "c");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&dcm); });
        REQUIRE(dart_heap_is_null(&dcm));
      }
    }

    WHEN("the decimal value is assigned to") {
      dart_heap_obj_erase(&obj, "lies");
      THEN("it takes on the value we expect") {
        auto boolean = dart_heap_obj_get(&obj, "lies");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&boolean); });
        REQUIRE(dart_heap_is_null(&boolean));
      }
    }
  }
}

SCENARIO("heap objects with unsafe refcounting can erase existing indices", "[heap abi unit]") {
  GIVEN("an object with existing values") {
    auto obj = dart_heap_obj_init_va_rc(DART_RC_UNSAFE, "sidbn", "hello", "world", "age", 27, "c", 2.99792, "lies", false, "none");
    auto guard = make_scope_guard([&] { dart_heap_destroy(&obj); });

    WHEN("the string value is erased") {
      dart_heap_obj_erase(&obj, "hello");
      THEN("it takes on the value we expect") {
        auto str = dart_heap_obj_get(&obj, "hello");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&str); });
        REQUIRE(dart_heap_is_null(&str));
      }
    }

    WHEN("the integer value is assigned to") {
      dart_heap_obj_erase(&obj, "age");
      THEN("it takes on the value we expect") {
        auto integer = dart_heap_obj_get(&obj, "age");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&integer); });
        REQUIRE(dart_heap_is_null(&integer));
      }
    }

    WHEN("the decimal value is assigned to") {
      dart_heap_obj_erase(&obj, "c");
      THEN("it takes on the value we expect") {
        auto dcm = dart_heap_obj_get(&obj, "c");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&dcm); });
        REQUIRE(dart_heap_is_null(&dcm));
      }
    }

    WHEN("the decimal value is assigned to") {
      dart_heap_obj_erase(&obj, "lies");
      THEN("it takes on the value we expect") {
        auto boolean = dart_heap_obj_get(&obj, "lies");
        auto guard = make_scope_guard([&] { dart_heap_destroy(&boolean); });
        REQUIRE(dart_heap_is_null(&boolean));
      }
    }
  }
}

SCENARIO("heap objects can be iterated over", "[heap abi unit]") {
  GIVEN("an object with contents") {
    auto* dyn = "dynamic";
    auto obj = dart_heap_obj_init_va("idbsS", "int", 1,
        "decimal", 3.14159, "bool", 0, "str", "fixed", "Str", dyn, strlen(dyn));
    auto guard = make_scope_guard([&] { dart_heap_destroy(&obj); });

    WHEN("we default initialize an iterator") {
      dart_iterator_t it;
      dart_iterator_init_err(&it);
      auto guard = make_scope_guard([&] { dart_iterator_destroy(&it); });
      THEN("it goes nowhere") {
        REQUIRE(dart_iterator_done(&it));
      }
    }

    WHEN("we create an iterator") {
      // Initialize an iterator for our object.
      dart_iterator_t it;
      dart_iterator_init_from_err(&it, &obj);
      auto guard = make_scope_guard([&] { dart_iterator_destroy(&it); });

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
          dart_destroy(&five);
          dart_destroy(&four);
          dart_destroy(&three);
          dart_destroy(&two);
          dart_destroy(&one);
        });
        REQUIRE(dart_iterator_done(&it));

        REQUIRE(dart_is_str(&one));
        REQUIRE(dart_str_get(&one) == "dynamic"s);
        REQUIRE(dart_is_int(&two));
        REQUIRE(dart_int_get(&two) == 1);
        REQUIRE(dart_is_str(&three));
        REQUIRE(dart_str_get(&three) == "fixed"s);
        REQUIRE(dart_is_bool(&four));
        REQUIRE(dart_bool_get(&four) == 0);
        REQUIRE(dart_is_dcm(&five));
        REQUIRE(dart_dcm_get(&five) == 3.14159);
      }

      WHEN("we create a copy") {
        dart_iterator_t copy;
        dart_iterator_copy_err(&copy, &it);
        auto guard = make_scope_guard([&] { dart_iterator_destroy(&copy); });

        size_t spins = 0;
        while (!dart_iterator_done(&it)) {
          dart_iterator_next(&it);
          ++spins;
        }
        REQUIRE(dart_iterator_done(&it));
        REQUIRE(!dart_iterator_done(&copy));

        while (!dart_iterator_done(&copy)) {
          dart_iterator_next(&copy);
          --spins;
        }
        REQUIRE(dart_iterator_done(&copy));
        REQUIRE(spins == 0);
      }

      WHEN("we move into a new iterator") {
        dart_iterator_t moved;
        dart_iterator_move_err(&moved, &it);
        auto guard = make_scope_guard([&] { dart_iterator_destroy(&moved); });
        THEN("it resets the original iterator") {
          REQUIRE(dart_iterator_done(&it));
        }
      }
    }

    WHEN("we create a key iterator") {
      // Initialize a key iterator for our object
      dart_iterator_t it;
      dart_iterator_init_key_from_err(&it, &obj);
      auto guard = make_scope_guard([&] { dart_iterator_destroy(&it); });

      THEN("it visits all keys") {
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
          dart_destroy(&five);
          dart_destroy(&four);
          dart_destroy(&three);
          dart_destroy(&two);
          dart_destroy(&one);
        });
        REQUIRE(dart_iterator_done(&it));

        REQUIRE(dart_is_str(&one));
        REQUIRE(dart_str_get(&one) == "Str"s);
        REQUIRE(dart_is_str(&two));
        REQUIRE(dart_str_get(&two) == "int"s);
        REQUIRE(dart_is_str(&three));
        REQUIRE(dart_str_get(&three) == "str"s);
        REQUIRE(dart_is_str(&four));
        REQUIRE(dart_str_get(&four) == "bool"s);
        REQUIRE(dart_is_str(&five));
        REQUIRE(dart_str_get(&five) == "decimal"s);
      }
    }

    WHEN("we use automatic iteration") {
      int idx = 0;
      dart_packet_t val;
      auto arr = dart_heap_arr_init_va("Sisbd", dyn, strlen(dyn), 1, "fixed", 0, 3.14159);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&arr); });
      THEN("it visits all values in the expected order") {
        dart_for_each(&obj, &val) {
          // Get the value manually.
          auto verify = dart_heap_arr_get(&arr, idx++);
          auto guard = make_scope_guard([&] { dart_heap_destroy(&verify); });

          // Check it
          REQUIRE(!dart_is_null(&val));
          REQUIRE(!dart_heap_is_null(&verify));
          REQUIRE(dart_get_type(&val) == dart_get_type(&verify));
          REQUIRE(dart_equal(&val, &verify));
        }
      }
    }

    WHEN("we use automatic key iteration") {
      int idx = 0;
      dart_packet_t val;
      auto arr = dart_heap_arr_init_va_rc(DART_RC_SAFE, "sssss", "Str", "int", "str", "bool", "decimal");
      auto guard = make_scope_guard([&] { dart_heap_destroy(&arr); });
      THEN("it visits all keys in the expected order") {
        dart_for_each_key(&obj, &val) {
          // Get the key manually.
          auto verify = dart_heap_arr_get(&arr, idx++);
          auto guard = make_scope_guard([&] { dart_heap_destroy(&verify); });

          // Check it.
          REQUIRE(dart_equal(&val, &verify));
        }
      }
    }
  }
}

SCENARIO("heap objects with unsafe refcounting can be iterated over", "[heap abi unit]") {
  GIVEN("an object with contents") {
    auto* dyn = "dynamic";
    auto obj = dart_heap_obj_init_va_rc(DART_RC_UNSAFE, "idbsS", "int", 1,
        "decimal", 3.14159, "bool", 0, "str", "fixed", "Str", dyn, strlen(dyn));
    auto guard = make_scope_guard([&] { dart_heap_destroy(&obj); });

    WHEN("we default initialize an iterator") {
      dart_iterator_t it;
      dart_iterator_init_err(&it);
      auto guard = make_scope_guard([&] { dart_iterator_destroy(&it); });
      THEN("it goes nowhere") {
        REQUIRE(dart_iterator_done(&it));
      }
    }

    WHEN("we create an iterator") {
      // Initialize an iterator for our object.
      dart_iterator_t it;
      dart_iterator_init_from_err(&it, &obj);
      auto guard = make_scope_guard([&] { dart_iterator_destroy(&it); });

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
          dart_destroy(&five);
          dart_destroy(&four);
          dart_destroy(&three);
          dart_destroy(&two);
          dart_destroy(&one);
        });
        REQUIRE(dart_iterator_done(&it));

        REQUIRE(dart_is_str(&one));
        REQUIRE(dart_str_get(&one) == "dynamic"s);
        REQUIRE(dart_is_int(&two));
        REQUIRE(dart_int_get(&two) == 1);
        REQUIRE(dart_is_str(&three));
        REQUIRE(dart_str_get(&three) == "fixed"s);
        REQUIRE(dart_is_bool(&four));
        REQUIRE(dart_bool_get(&four) == 0);
        REQUIRE(dart_is_dcm(&five));
        REQUIRE(dart_dcm_get(&five) == 3.14159);
      }

      WHEN("we create a copy") {
        dart_iterator_t copy;
        dart_iterator_copy_err(&copy, &it);
        auto guard = make_scope_guard([&] { dart_iterator_destroy(&copy); });

        size_t spins = 0;
        while (!dart_iterator_done(&it)) {
          dart_iterator_next(&it);
          ++spins;
        }
        REQUIRE(dart_iterator_done(&it));
        REQUIRE(!dart_iterator_done(&copy));

        while (!dart_iterator_done(&copy)) {
          dart_iterator_next(&copy);
          --spins;
        }
        REQUIRE(dart_iterator_done(&copy));
        REQUIRE(spins == 0);
      }

      WHEN("we move into a new iterator") {
        dart_iterator_t moved;
        dart_iterator_move_err(&moved, &it);
        auto guard = make_scope_guard([&] { dart_iterator_destroy(&moved); });
        THEN("it resets the original iterator") {
          REQUIRE(dart_iterator_done(&it));
        }
      }
    }

    WHEN("we create a key iterator") {
      // Initialize a key iterator for our object
      dart_iterator_t it;
      dart_iterator_init_key_from_err(&it, &obj);
      auto guard = make_scope_guard([&] { dart_iterator_destroy(&it); });

      THEN("it visits all keys") {
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
          dart_destroy(&five);
          dart_destroy(&four);
          dart_destroy(&three);
          dart_destroy(&two);
          dart_destroy(&one);
        });
        REQUIRE(dart_iterator_done(&it));

        REQUIRE(dart_is_str(&one));
        REQUIRE(dart_str_get(&one) == "Str"s);
        REQUIRE(dart_is_str(&two));
        REQUIRE(dart_str_get(&two) == "int"s);
        REQUIRE(dart_is_str(&three));
        REQUIRE(dart_str_get(&three) == "str"s);
        REQUIRE(dart_is_str(&four));
        REQUIRE(dart_str_get(&four) == "bool"s);
        REQUIRE(dart_is_str(&five));
        REQUIRE(dart_str_get(&five) == "decimal"s);
      }
    }

    WHEN("we use automatic iteration") {
      int idx = 0;
      dart_packet_t val;
      auto arr = dart_heap_arr_init_va_rc(DART_RC_UNSAFE, "Sisbd", dyn, strlen(dyn), 1, "fixed", 0, 3.14159);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&arr); });
      THEN("it visits all values in the expected order") {
        dart_for_each(&obj, &val) {
          // Get the value manually.
          auto verify = dart_heap_arr_get(&arr, idx++);
          auto guard = make_scope_guard([&] { dart_heap_destroy(&verify); });

          // Check it
          REQUIRE(!dart_is_null(&val));
          REQUIRE(!dart_heap_is_null(&verify));
          REQUIRE(dart_get_type(&val) == dart_get_type(&verify));
          REQUIRE(dart_equal(&val, &verify));
        }
      }
    }

    WHEN("we use automatic key iteration") {
      int idx = 0;
      dart_packet_t val;
      auto arr = dart_heap_arr_init_va_rc(DART_RC_UNSAFE, "sssss", "Str", "int", "str", "bool", "decimal");
      auto guard = make_scope_guard([&] { dart_heap_destroy(&arr); });
      THEN("it visits all keys in the expected order") {
        dart_for_each_key(&obj, &val) {
          // Get the key manually.
          auto verify = dart_heap_arr_get(&arr, idx++);
          auto guard = make_scope_guard([&] { dart_heap_destroy(&verify); });

          // Check it.
          REQUIRE(dart_equal(&val, &verify));
        }
      }
    }
  }
}

SCENARIO("heap objects can switch between finalized and non-finalized representations", "[heap abi unit]") {
  GIVEN("an object with lots of contents") {
    auto obj = dart_heap_obj_init_va("sass,oidb,sidbn",
        "hello", "world", "arr", "one", "two",
        "obj", "nest_int", 1337, "nest_dcm", 3.14159, "nest_bool", true,
        "yes", "no", "int", 1337, "dcm", 3.14159, "bool", true, "none");
    auto guard = make_scope_guard([&] { dart_heap_destroy(&obj); });

    WHEN("the object is finalized") {
      // These functions are equivalent
      auto fin = dart_finalize(&obj);
      auto low = dart_lower(&obj);
      auto guard = make_scope_guard([&] {
        dart_destroy(&low);
        dart_destroy(&fin);
      });

      THEN("it still compares equal with its original representation") {
        REQUIRE(dart_is_finalized(&fin));
        REQUIRE(dart_is_finalized(&low));
        REQUIRE(dart_equal(&fin, &low));
        REQUIRE(dart_equal(&obj, &fin));
        REQUIRE(dart_equal(&fin, &obj));
        REQUIRE(dart_equal(&obj, &low));
        REQUIRE(dart_equal(&low, &obj));
      }

      WHEN("the object is de-finalized again") {
        auto liftd = dart_lift(&low);
        auto nofin = dart_definalize(&fin);
        auto guard = make_scope_guard([&] {
          dart_destroy(&nofin);
          dart_destroy(&liftd);
        });

        THEN("comparisons still check out in all directions") {
          REQUIRE(!dart_is_finalized(&liftd));
          REQUIRE(!dart_is_finalized(&nofin));
          REQUIRE(dart_equal(&liftd, &nofin));
          REQUIRE(dart_equal(&liftd, &obj));
          REQUIRE(dart_equal(&nofin, &obj));
          REQUIRE(dart_equal(&liftd, &low));
          REQUIRE(dart_equal(&nofin, &fin));
          REQUIRE(dart_equal(&low, &liftd));
          REQUIRE(dart_equal(&nofin, &fin));
        }
      }
    }
  }
}

SCENARIO("heap objects with unsafe refcounting can switch between finalized and non-finalized representations", "[heap abi unit]") {
  GIVEN("an object with lots of contents") {
    auto obj = dart_heap_obj_init_va_rc(DART_RC_UNSAFE, "sass,oidb,sidbn",
        "hello", "world", "arr", "one", "two",
        "obj", "nest_int", 1337, "nest_dcm", 3.14159, "nest_bool", true,
        "yes", "no", "int", 1337, "dcm", 3.14159, "bool", true, "none");
    auto guard = make_scope_guard([&] { dart_heap_destroy(&obj); });

    WHEN("the object is finalized") {
      // These functions are equivalent
      auto fin = dart_finalize(&obj);
      auto low = dart_lower(&obj);
      auto guard = make_scope_guard([&] {
        dart_destroy(&low);
        dart_destroy(&fin);
      });

      THEN("it still compares equal with its original representation") {
        REQUIRE(dart_is_finalized(&fin));
        REQUIRE(dart_is_finalized(&low));
        REQUIRE(dart_equal(&fin, &low));
        REQUIRE(dart_equal(&obj, &fin));
        REQUIRE(dart_equal(&fin, &obj));
        REQUIRE(dart_equal(&obj, &low));
        REQUIRE(dart_equal(&low, &obj));
      }

      WHEN("the object is de-finalized again") {
        auto liftd = dart_lift(&low);
        auto nofin = dart_definalize(&fin);
        auto guard = make_scope_guard([&] {
          dart_destroy(&nofin);
          dart_destroy(&liftd);
        });

        THEN("comparisons still check out in all directions") {
          REQUIRE(!dart_is_finalized(&liftd));
          REQUIRE(!dart_is_finalized(&nofin));
          REQUIRE(dart_equal(&liftd, &nofin));
          REQUIRE(dart_equal(&liftd, &obj));
          REQUIRE(dart_equal(&nofin, &obj));
          REQUIRE(dart_equal(&liftd, &low));
          REQUIRE(dart_equal(&nofin, &fin));
          REQUIRE(dart_equal(&low, &liftd));
          REQUIRE(dart_equal(&nofin, &fin));
        }
      }
    }
  }
}

SCENARIO("heap finalized objects have unique object representations") {
  GIVEN("two independent, but equivalent, objects") {
    auto objone = dart_heap_obj_init_va("sass,oidb,sidbn",
        "hello", "world", "arr", "one", "two",
        "obj", "nest_int", 1337, "nest_dcm", 3.14159, "nest_bool", true,
        "yes", "no", "int", 1337, "dcm", 3.14159, "bool", true, "none");
    auto objtwo = dart_heap_obj_init_va("sass,oidb,sidbn",
        "hello", "world", "arr", "one", "two",
        "obj", "nest_int", 1337, "nest_dcm", 3.14159, "nest_bool", true,
        "yes", "no", "int", 1337, "dcm", 3.14159, "bool", true, "none");
    auto guard = make_scope_guard([&] {
      dart_heap_destroy(&objtwo);
      dart_heap_destroy(&objone);
    });

    WHEN("the objects are finalized") {
      auto finone = dart_lower(&objone);
      auto fintwo = dart_lower(&objtwo);
      auto guard = make_scope_guard([&] {
        dart_destroy(&fintwo);
        dart_destroy(&finone);
      });

      THEN("they produce the same byte representation") {
        size_t lenone, lentwo;
        auto* bytesone = dart_get_bytes(&finone, &lenone);
        auto* bytestwo = dart_get_bytes(&fintwo, &lentwo);
        REQUIRE(lenone == lentwo);
        REQUIRE(std::memcmp(bytesone, bytestwo, lenone) == 0);

        auto* ownone = dart_dup_bytes(&finone, &lenone);
        auto* owntwo = dart_dup_bytes(&fintwo, &lentwo);
        auto guard = make_scope_guard([&] {
          dart_aligned_free(owntwo);
          dart_aligned_free(ownone);
        });
        REQUIRE(lenone == lentwo);
        REQUIRE(std::memcmp(ownone, owntwo, lenone) == 0);
      }

      THEN("they can be reconstituted") {
        size_t lenone;
        auto* bytes = dart_get_bytes(&finone, &lenone);
        auto recone = dart_from_bytes(bytes, lenone);
        auto guard = make_scope_guard([&] { dart_destroy(&recone); });

        REQUIRE(dart_equal(&recone, &objone));
        REQUIRE(dart_equal(&recone, &finone));
        REQUIRE(std::memcmp(dart_get_bytes(&recone, nullptr), bytes, lenone) == 0);
      }
    }
  }
}

SCENARIO("heap finalized objects with unsafe refcounting have unique object representations") {
  GIVEN("two independent, but equivalent, objects") {
    auto objone = dart_heap_obj_init_va_rc(DART_RC_UNSAFE, "sass,oidb,sidbn",
        "hello", "world", "arr", "one", "two",
        "obj", "nest_int", 1337, "nest_dcm", 3.14159, "nest_bool", true,
        "yes", "no", "int", 1337, "dcm", 3.14159, "bool", true, "none");
    auto objtwo = dart_heap_obj_init_va_rc(DART_RC_UNSAFE, "sass,oidb,sidbn",
        "hello", "world", "arr", "one", "two",
        "obj", "nest_int", 1337, "nest_dcm", 3.14159, "nest_bool", true,
        "yes", "no", "int", 1337, "dcm", 3.14159, "bool", true, "none");
    auto guard = make_scope_guard([&] {
      dart_heap_destroy(&objtwo);
      dart_heap_destroy(&objone);
    });

    WHEN("the objects are finalized") {
      auto finone = dart_lower(&objone);
      auto fintwo = dart_lower(&objtwo);
      auto guard = make_scope_guard([&] {
        dart_destroy(&fintwo);
        dart_destroy(&finone);
      });

      THEN("they produce the same byte representation") {
        size_t lenone, lentwo;
        auto* bytesone = dart_get_bytes(&finone, &lenone);
        auto* bytestwo = dart_get_bytes(&fintwo, &lentwo);
        REQUIRE(lenone == lentwo);
        REQUIRE(std::memcmp(bytesone, bytestwo, lenone) == 0);

        auto* ownone = dart_dup_bytes(&finone, &lenone);
        auto* owntwo = dart_dup_bytes(&fintwo, &lentwo);
        auto guard = make_scope_guard([&] {
          dart_aligned_free(owntwo);
          dart_aligned_free(ownone);
        });
        REQUIRE(lenone == lentwo);
        REQUIRE(std::memcmp(ownone, owntwo, lenone) == 0);
      }

      THEN("they can be reconstituted") {
        size_t lenone;
        auto* bytes = dart_get_bytes(&finone, &lenone);
        auto recone = dart_from_bytes(bytes, lenone);
        auto guard = make_scope_guard([&] { dart_destroy(&recone); });

        REQUIRE(dart_equal(&recone, &objone));
        REQUIRE(dart_equal(&recone, &finone));
        REQUIRE(std::memcmp(dart_get_bytes(&recone, nullptr), bytes, lenone) == 0);
      }
    }
  }
}

SCENARIO("heap arrays can be constructed with many values", "[heap abi unit]") {
  GIVEN("many test cases to run") {
    WHEN("an array is constructed with many values") {
      auto* str = "runtime";
      dart_heap_t arr;
      dart_heap_arr_init_va_err(&arr, "Ssbdi",
          str, strlen(str), "string", true, 2.99792, 1337);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&arr); });

      THEN("everything winds up where it's supposed to") {
        auto sized_str = dart_heap_arr_get(&arr, 0);
        auto str = dart_heap_arr_get(&arr, 1);
        auto boolean = dart_heap_arr_get(&arr, 2);
        auto decimal = dart_heap_arr_get(&arr, 3);
        auto integer = dart_heap_arr_get(&arr, 4);
        auto guard = make_scope_guard([&] {
          dart_heap_destroy(&integer);
          dart_heap_destroy(&decimal);
          dart_heap_destroy(&boolean);
          dart_heap_destroy(&str);
          dart_heap_destroy(&sized_str);
        });

        REQUIRE(dart_heap_str_get(&sized_str) == "runtime"s);
        REQUIRE(dart_heap_str_get(&str) == "string"s);
        REQUIRE(dart_heap_bool_get(&boolean) == 1);
        REQUIRE(dart_heap_dcm_get(&decimal) == Approx(2.99792));
        REQUIRE(dart_heap_int_get(&integer) == 1337);
      }

      WHEN("that array is cleared") {
        dart_heap_arr_clear(&arr);
        THEN("all the elements are gone") {
          auto sized_str = dart_heap_arr_get(&arr, 0);
          auto str = dart_heap_arr_get(&arr, 1);
          auto boolean = dart_heap_arr_get(&arr, 2);
          auto decimal = dart_heap_arr_get(&arr, 3);
          auto integer = dart_heap_arr_get(&arr, 4);
          auto guard = make_scope_guard([&] {
            dart_heap_destroy(&integer);
            dart_heap_destroy(&decimal);
            dart_heap_destroy(&boolean);
            dart_heap_destroy(&str);
            dart_heap_destroy(&sized_str);
          });

          REQUIRE(dart_heap_is_null(&sized_str));
          REQUIRE(dart_heap_is_null(&str));
          REQUIRE(dart_heap_is_null(&boolean));
          REQUIRE(dart_heap_is_null(&decimal));
          REQUIRE(dart_heap_is_null(&integer));
        }
      }
    }
  }
}

SCENARIO("heap arrays with unsafe refcounting can be constructed with many values", "[heap abi unit]") {
  GIVEN("many test cases to run") {
    WHEN("an array is constructed with many values") {
      auto* str = "runtime";
      dart_heap_t arr;
      dart_heap_arr_init_va_rc_err(&arr, DART_RC_UNSAFE, "Ssbdi",
          str, strlen(str), "string", true, 2.99792, 1337);
      auto guard = make_scope_guard([&] { dart_heap_destroy(&arr); });

      THEN("everything winds up where it's supposed to") {
        auto sized_str = dart_heap_arr_get(&arr, 0);
        auto str = dart_heap_arr_get(&arr, 1);
        auto boolean = dart_heap_arr_get(&arr, 2);
        auto decimal = dart_heap_arr_get(&arr, 3);
        auto integer = dart_heap_arr_get(&arr, 4);
        auto guard = make_scope_guard([&] {
          dart_heap_destroy(&integer);
          dart_heap_destroy(&decimal);
          dart_heap_destroy(&boolean);
          dart_heap_destroy(&str);
          dart_heap_destroy(&sized_str);
        });

        REQUIRE(dart_heap_str_get(&sized_str) == "runtime"s);
        REQUIRE(dart_heap_str_get(&str) == "string"s);
        REQUIRE(dart_heap_bool_get(&boolean) == 1);
        REQUIRE(dart_heap_dcm_get(&decimal) == Approx(2.99792));
        REQUIRE(dart_heap_int_get(&integer) == 1337);
      }

      WHEN("that array is cleared") {
        dart_heap_arr_clear(&arr);
        THEN("all the elements are gone") {
          auto sized_str = dart_heap_arr_get(&arr, 0);
          auto str = dart_heap_arr_get(&arr, 1);
          auto boolean = dart_heap_arr_get(&arr, 2);
          auto decimal = dart_heap_arr_get(&arr, 3);
          auto integer = dart_heap_arr_get(&arr, 4);
          auto guard = make_scope_guard([&] {
            dart_heap_destroy(&integer);
            dart_heap_destroy(&decimal);
            dart_heap_destroy(&boolean);
            dart_heap_destroy(&str);
            dart_heap_destroy(&sized_str);
          });

          REQUIRE(dart_heap_is_null(&sized_str));
          REQUIRE(dart_heap_is_null(&str));
          REQUIRE(dart_heap_is_null(&boolean));
          REQUIRE(dart_heap_is_null(&decimal));
          REQUIRE(dart_heap_is_null(&integer));
        }
      }
    }
  }
}

SCENARIO("heap arrays can insert any type", "[heap abi unit]") {
  GIVEN("an array to insert into") {
    auto arr = dart_heap_arr_init_rc(DART_RC_SAFE);
    auto guard = make_scope_guard([&] { dart_heap_destroy(&arr); });

    WHEN("we insert another dart type") {
      auto nested = dart_heap_obj_init_va("ss", "hello", "world", "yes", "no");
      auto guard = make_scope_guard([&] { dart_heap_destroy(&nested); });
      dart_heap_arr_insert_heap(&arr, 0, &nested);

      THEN("the object is reachable, and the original copy is preserved") {
        auto grabbed = dart_heap_arr_get(&arr, 0);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&grabbed); });

        REQUIRE(dart_heap_is_obj(&nested));
        REQUIRE(dart_heap_is_obj(&grabbed));
        REQUIRE(dart_heap_size(&arr) == 1U);
        REQUIRE(dart_heap_size(&nested) == 2U);
        REQUIRE(dart_heap_size(&grabbed) == 2U);
        REQUIRE(dart_heap_equal(&nested, &grabbed));
      }
    }

    WHEN("we take another dart type") {
      auto nested = dart_heap_obj_init_va("ss", "hello", "world", "yes", "no");
      auto guard = make_scope_guard([&] { dart_heap_destroy(&nested); });
      dart_heap_arr_insert_take_heap(&arr, 0, &nested);

      THEN("the object is reachable, and the original copy is reset to null") {
        auto grabbed = dart_heap_arr_get(&arr, 0);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&grabbed); });

        REQUIRE(dart_heap_is_null(&nested));
        REQUIRE(dart_heap_is_obj(&grabbed));
        REQUIRE(dart_heap_size(&arr) == 1U);
        REQUIRE(dart_heap_size(&grabbed) == 2U);
        REQUIRE(!dart_heap_equal(&nested, &grabbed));
      }
    }

    WHEN("we insert a string") {
      dart_heap_arr_insert_str(&arr, 0, "testing");
      THEN("the string is reachable and has the correct value") {
        auto str = dart_heap_arr_get(&arr, 0);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&str); });

        REQUIRE(dart_heap_is_str(&str));
        REQUIRE(dart_heap_str_get(&str) == "testing"s);
      }
    }

    WHEN("we insert an integer") {
      dart_heap_arr_insert_int(&arr, 0, 1337);
      THEN("the integer is reachable and has the correct value") {
        auto integer = dart_heap_arr_get(&arr, 0);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&integer); });

        REQUIRE(dart_heap_is_int(&integer));
        REQUIRE(dart_heap_int_get(&integer) == 1337);
      }
    }

    WHEN("we insert a decimal") {
      dart_heap_arr_insert_dcm(&arr, 0, 3.14159);
      THEN("the decimal is reachable and has the correct value") {
        auto dcm = dart_heap_arr_get(&arr, 0);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&dcm); });

        REQUIRE(dart_heap_is_dcm(&dcm));
        REQUIRE(dart_heap_dcm_get(&dcm) == 3.14159);
      }
    }

    WHEN("we insert a boolean") {
      dart_heap_arr_insert_bool(&arr, 0, true);
      THEN("the boolean is reachable and has the correct value") {
        auto boolean = dart_heap_arr_get(&arr, 0);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&boolean); });

        REQUIRE(dart_heap_is_bool(&boolean));
        REQUIRE(dart_heap_bool_get(&boolean));
      }
    }

    WHEN("we insert a null") {
      dart_heap_arr_insert_null(&arr, 0);
      THEN("the null is reachable") {
        auto null = dart_heap_arr_get(&arr, 0);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&null); });

        REQUIRE(dart_heap_is_null(&null));
        REQUIRE(dart_heap_size(&arr) == 1U);
      }
    }
  }
}

SCENARIO("heap arrays with unsafe refcounting can insert any type", "[heap abi unit]") {
  GIVEN("an array to insert into") {
    auto arr = dart_heap_arr_init_rc(DART_RC_UNSAFE);
    auto guard = make_scope_guard([&] { dart_heap_destroy(&arr); });

    WHEN("we insert another dart type") {
      auto nested = dart_heap_obj_init_va_rc(DART_RC_UNSAFE, "ss", "hello", "world", "yes", "no");
      auto guard = make_scope_guard([&] { dart_heap_destroy(&nested); });
      dart_heap_arr_insert_heap(&arr, 0, &nested);

      THEN("the object is reachable, and the original copy is preserved") {
        auto grabbed = dart_heap_arr_get(&arr, 0);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&grabbed); });

        REQUIRE(dart_heap_is_obj(&nested));
        REQUIRE(dart_heap_is_obj(&grabbed));
        REQUIRE(dart_heap_size(&arr) == 1U);
        REQUIRE(dart_heap_size(&nested) == 2U);
        REQUIRE(dart_heap_size(&grabbed) == 2U);
        REQUIRE(dart_heap_equal(&nested, &grabbed));
      }
    }

    WHEN("we take another dart type") {
      auto nested = dart_heap_obj_init_va_rc(DART_RC_UNSAFE, "ss", "hello", "world", "yes", "no");
      auto guard = make_scope_guard([&] { dart_heap_destroy(&nested); });
      dart_heap_arr_insert_take_heap(&arr, 0, &nested);

      THEN("the object is reachable, and the original copy is reset to null") {
        auto grabbed = dart_heap_arr_get(&arr, 0);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&grabbed); });

        REQUIRE(dart_heap_is_null(&nested));
        REQUIRE(dart_heap_is_obj(&grabbed));
        REQUIRE(dart_heap_size(&arr) == 1U);
        REQUIRE(dart_heap_size(&grabbed) == 2U);
        REQUIRE(!dart_heap_equal(&nested, &grabbed));
      }
    }

    WHEN("we insert a string") {
      dart_heap_arr_insert_str(&arr, 0, "testing");
      THEN("the string is reachable and has the correct value") {
        auto str = dart_heap_arr_get(&arr, 0);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&str); });

        REQUIRE(dart_heap_is_str(&str));
        REQUIRE(dart_heap_str_get(&str) == "testing"s);
      }
    }

    WHEN("we insert an integer") {
      dart_heap_arr_insert_int(&arr, 0, 1337);
      THEN("the integer is reachable and has the correct value") {
        auto integer = dart_heap_arr_get(&arr, 0);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&integer); });

        REQUIRE(dart_heap_is_int(&integer));
        REQUIRE(dart_heap_int_get(&integer) == 1337);
      }
    }

    WHEN("we insert a decimal") {
      dart_heap_arr_insert_dcm(&arr, 0, 3.14159);
      THEN("the decimal is reachable and has the correct value") {
        auto dcm = dart_heap_arr_get(&arr, 0);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&dcm); });

        REQUIRE(dart_heap_is_dcm(&dcm));
        REQUIRE(dart_heap_dcm_get(&dcm) == 3.14159);
      }
    }

    WHEN("we insert a boolean") {
      dart_heap_arr_insert_bool(&arr, 0, true);
      THEN("the boolean is reachable and has the correct value") {
        auto boolean = dart_heap_arr_get(&arr, 0);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&boolean); });

        REQUIRE(dart_heap_is_bool(&boolean));
        REQUIRE(dart_heap_bool_get(&boolean));
      }
    }

    WHEN("we insert a null") {
      dart_heap_arr_insert_null(&arr, 0);
      THEN("the null is reachable") {
        auto null = dart_heap_arr_get(&arr, 0);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&null); });

        REQUIRE(dart_heap_is_null(&null));
        REQUIRE(dart_heap_size(&arr) == 1U);
      }
    }
  }
}

SCENARIO("heap arrays can assign to existing indices", "[heap abi unit]") {
  GIVEN("an array full of stuff") {
    dart_heap_t arr;
    dart_heap_arr_init_va_rc_err(&arr, DART_RC_SAFE, "sos,idbn", "hello", "yes", "no", 27, 2.99792, false);
    auto guard = make_scope_guard([&] { dart_heap_destroy(&arr); });

    WHEN("the nested object is assigned to") {
      auto nested = dart_heap_obj_init_va("s", "stop", "go");
      auto guard = make_scope_guard([&] { dart_heap_destroy(&nested); });
      dart_heap_arr_set_heap(&arr, 1, &nested);
      THEN("it takes on the value we expect") {
        auto nes = dart_heap_arr_get(&arr, 1);
        auto str = dart_heap_obj_get(&nes, "stop");
        auto guard = make_scope_guard([&] {
          dart_heap_destroy(&str);
          dart_heap_destroy(&nes);
        });
        REQUIRE(dart_heap_is_obj(&nes));
        REQUIRE(dart_heap_is_obj(&nested));
        REQUIRE(dart_heap_size(&nes) == 1U);
        REQUIRE(dart_heap_size(&nested) == 1U);
        REQUIRE(dart_heap_equal(&nested, &nes));
        REQUIRE(dart_heap_str_get(&str) == "go"s);
        REQUIRE(dart_heap_size(&arr) == 6U);
      }
    }

    WHEN("the nested object is move assigned to") {
      auto nested = dart_heap_obj_init_va("s", "stop", "go");
      auto guard = make_scope_guard([&] { dart_heap_destroy(&nested); });
      dart_heap_arr_set_take_heap(&arr, 1, &nested);
      THEN("it takes on the value we expect") {
        auto nes = dart_heap_arr_get(&arr, 1);
        auto str = dart_heap_obj_get(&nes, "stop");
        auto guard = make_scope_guard([&] {
          dart_heap_destroy(&str);
          dart_heap_destroy(&nes);
        });
        REQUIRE(dart_heap_is_obj(&nes));
        REQUIRE(dart_heap_is_null(&nested));
        REQUIRE(dart_heap_size(&nes) == 1U);
        REQUIRE(!dart_heap_equal(&nested, &nes));
        REQUIRE(dart_heap_str_get(&str) == "go"s);
        REQUIRE(dart_heap_size(&arr) == 6U);
      }
    }

    WHEN("the nested object is assigned to from a disparate type") {
      dart_heap_arr_set_null(&arr, 1);
      THEN("it takes on the value we expect") {
        auto prevobj = dart_heap_arr_get(&arr, 1);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&prevobj); });
        REQUIRE(dart_heap_is_null(&prevobj));
        REQUIRE(dart_heap_size(&arr) == 6U);
      }
    }

    WHEN("the string value is assigned to") {
      dart_heap_arr_set_str(&arr, 0, "goodbye");
      THEN("it takes on the value we expect") {
        auto str = dart_heap_arr_get(&arr, 0);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&str); });
        REQUIRE(dart_heap_is_str(&str));
        REQUIRE(dart_heap_size(&str) == strlen("goodbye"));
        REQUIRE(dart_heap_str_get(&str) == "goodbye"s);
      }
    }

    WHEN("the string value is assigned from a disparate type") {
      dart_heap_arr_set_bool(&arr, 0, true);
      THEN("it takes on the value we expect") {
        auto prevstr = dart_heap_arr_get(&arr, 0);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&prevstr); });

        REQUIRE(dart_heap_is_bool(&prevstr));
        REQUIRE(dart_heap_bool_get(&prevstr));
      }
    }

    WHEN("the integer value is assigned to") {
      dart_heap_arr_set_int(&arr, 2, 72);
      THEN("it takes on the value we expect") {
        auto integer = dart_heap_arr_get(&arr, 2);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&arr); });
        REQUIRE(dart_heap_is_int(&integer));
        REQUIRE(dart_heap_int_get(&integer) == 72);
      }
    }

    WHEN("the integer value is assigned from a disparate type") {
      dart_heap_arr_set_dcm(&arr, 2, 27.5);
      THEN("it takes on the value we expect") {
        auto prevint = dart_heap_arr_get(&arr, 2);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&prevint); });
        REQUIRE(dart_heap_is_dcm(&prevint));
        REQUIRE(dart_heap_dcm_get(&prevint) == 27.5);
      }
    }

    WHEN("the decimal value is assigned to") {
      dart_heap_arr_set_dcm(&arr, 3, 3.0);
      THEN("it takes on the value we expect") {
        auto dcm = dart_heap_arr_get(&arr, 3);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&dcm); });
        REQUIRE(dart_heap_is_dcm(&dcm));
        REQUIRE(dart_heap_dcm_get(&dcm) == 3.0);
      }
    }

    WHEN("the decimal is assigned to from a disparate type") {
      dart_heap_arr_set_int(&arr, 3, 3);
      THEN("it takes on the value we expect") {
        auto prevdcm = dart_heap_arr_get(&arr, 3);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&prevdcm); });
        REQUIRE(dart_heap_is_int(&prevdcm));
        REQUIRE(dart_heap_int_get(&prevdcm) == 3);
      }
    }

    WHEN("the boolean value is assigned to") {
      dart_heap_arr_set_bool(&arr, 4, true);
      THEN("it takes on the value we expect") {
        auto boolean = dart_heap_arr_get(&arr, 4);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&boolean); });
        REQUIRE(dart_heap_is_bool(&boolean));
        REQUIRE(dart_heap_bool_get(&boolean));
      }
    }

    WHEN("the boolean is assigned to from a disparate type") {
      dart_heap_arr_set_str(&arr, 4, "true");
      THEN("it takes on the value we expect") {
        auto prevbool = dart_heap_arr_get(&arr, 4);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&prevbool); });
        REQUIRE(dart_heap_is_str(&prevbool));
        REQUIRE(dart_heap_str_get(&prevbool) == "true"s);
      }
    }

    WHEN("the null is assigned to") {
      dart_heap_arr_set_null(&arr, 5);
      THEN("it retains the value we expect") {
        auto null = dart_heap_arr_get(&arr, 5);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&null); });
        REQUIRE(dart_heap_is_null(&null));
      }
    }

    WHEN("the null is assigned to from a disparate type") {
      auto nested = dart_heap_obj_init_va("sss", "hello", "world", "yes", "no", "stop", "go");
      dart_heap_arr_set_take_heap(&arr, 5, &nested);
      dart_heap_destroy(&nested);
      THEN("it takes on the value we expect") {
        auto nes = dart_heap_arr_get(&arr, 5);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&nes); });
        REQUIRE(dart_heap_is_obj(&nes));
        REQUIRE(dart_heap_size(&nes) == 3U);
      }
    }
  }
}

SCENARIO("heap arrays with unsafe refcounting can assign to existing indices", "[heap abi unit]") {
  GIVEN("an array full of stuff") {
    dart_heap_t arr;
    dart_heap_arr_init_va_rc_err(&arr, DART_RC_UNSAFE, "sos,idbn", "hello", "yes", "no", 27, 2.99792, false);
    auto guard = make_scope_guard([&] { dart_heap_destroy(&arr); });

    WHEN("the nested object is assigned to") {
      auto nested = dart_heap_obj_init_va_rc(DART_RC_UNSAFE, "s", "stop", "go");
      auto guard = make_scope_guard([&] { dart_heap_destroy(&nested); });
      dart_heap_arr_set_heap(&arr, 1, &nested);
      THEN("it takes on the value we expect") {
        auto nes = dart_heap_arr_get(&arr, 1);
        auto str = dart_heap_obj_get(&nes, "stop");
        auto guard = make_scope_guard([&] {
          dart_heap_destroy(&str);
          dart_heap_destroy(&nes);
        });
        REQUIRE(dart_heap_is_obj(&nes));
        REQUIRE(dart_heap_is_obj(&nested));
        REQUIRE(dart_heap_size(&nes) == 1U);
        REQUIRE(dart_heap_size(&nested) == 1U);
        REQUIRE(dart_heap_equal(&nested, &nes));
        REQUIRE(dart_heap_str_get(&str) == "go"s);
        REQUIRE(dart_heap_size(&arr) == 6U);
      }
    }

    WHEN("the nested object is move assigned to") {
      auto nested = dart_heap_obj_init_va_rc(DART_RC_UNSAFE, "s", "stop", "go");
      auto guard = make_scope_guard([&] { dart_heap_destroy(&nested); });
      dart_heap_arr_set_take_heap(&arr, 1, &nested);
      THEN("it takes on the value we expect") {
        auto nes = dart_heap_arr_get(&arr, 1);
        auto str = dart_heap_obj_get(&nes, "stop");
        auto guard = make_scope_guard([&] {
          dart_heap_destroy(&str);
          dart_heap_destroy(&nes);
        });
        REQUIRE(dart_heap_is_obj(&nes));
        REQUIRE(dart_heap_is_null(&nested));
        REQUIRE(dart_heap_size(&nes) == 1U);
        REQUIRE(!dart_heap_equal(&nested, &nes));
        REQUIRE(dart_heap_str_get(&str) == "go"s);
        REQUIRE(dart_heap_size(&arr) == 6U);
      }
    }

    WHEN("the nested object is assigned to from a disparate type") {
      dart_heap_arr_set_null(&arr, 1);
      THEN("it takes on the value we expect") {
        auto prevobj = dart_heap_arr_get(&arr, 1);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&prevobj); });
        REQUIRE(dart_heap_is_null(&prevobj));
        REQUIRE(dart_heap_size(&arr) == 6U);
      }
    }

    WHEN("the string value is assigned to") {
      dart_heap_arr_set_str(&arr, 0, "goodbye");
      THEN("it takes on the value we expect") {
        auto str = dart_heap_arr_get(&arr, 0);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&str); });
        REQUIRE(dart_heap_is_str(&str));
        REQUIRE(dart_heap_size(&str) == strlen("goodbye"));
        REQUIRE(dart_heap_str_get(&str) == "goodbye"s);
      }
    }

    WHEN("the string value is assigned from a disparate type") {
      dart_heap_arr_set_bool(&arr, 0, true);
      THEN("it takes on the value we expect") {
        auto prevstr = dart_heap_arr_get(&arr, 0);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&prevstr); });

        REQUIRE(dart_heap_is_bool(&prevstr));
        REQUIRE(dart_heap_bool_get(&prevstr));
      }
    }

    WHEN("the integer value is assigned to") {
      dart_heap_arr_set_int(&arr, 2, 72);
      THEN("it takes on the value we expect") {
        auto integer = dart_heap_arr_get(&arr, 2);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&arr); });
        REQUIRE(dart_heap_is_int(&integer));
        REQUIRE(dart_heap_int_get(&integer) == 72);
      }
    }

    WHEN("the integer value is assigned from a disparate type") {
      dart_heap_arr_set_dcm(&arr, 2, 27.5);
      THEN("it takes on the value we expect") {
        auto prevint = dart_heap_arr_get(&arr, 2);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&prevint); });
        REQUIRE(dart_heap_is_dcm(&prevint));
        REQUIRE(dart_heap_dcm_get(&prevint) == 27.5);
      }
    }

    WHEN("the decimal value is assigned to") {
      dart_heap_arr_set_dcm(&arr, 3, 3.0);
      THEN("it takes on the value we expect") {
        auto dcm = dart_heap_arr_get(&arr, 3);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&dcm); });
        REQUIRE(dart_heap_is_dcm(&dcm));
        REQUIRE(dart_heap_dcm_get(&dcm) == 3.0);
      }
    }

    WHEN("the decimal is assigned to from a disparate type") {
      dart_heap_arr_set_int(&arr, 3, 3);
      THEN("it takes on the value we expect") {
        auto prevdcm = dart_heap_arr_get(&arr, 3);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&prevdcm); });
        REQUIRE(dart_heap_is_int(&prevdcm));
        REQUIRE(dart_heap_int_get(&prevdcm) == 3);
      }
    }

    WHEN("the boolean value is assigned to") {
      dart_heap_arr_set_bool(&arr, 4, true);
      THEN("it takes on the value we expect") {
        auto boolean = dart_heap_arr_get(&arr, 4);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&boolean); });
        REQUIRE(dart_heap_is_bool(&boolean));
        REQUIRE(dart_heap_bool_get(&boolean));
      }
    }

    WHEN("the boolean is assigned to from a disparate type") {
      dart_heap_arr_set_str(&arr, 4, "true");
      THEN("it takes on the value we expect") {
        auto prevbool = dart_heap_arr_get(&arr, 4);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&prevbool); });
        REQUIRE(dart_heap_is_str(&prevbool));
        REQUIRE(dart_heap_str_get(&prevbool) == "true"s);
      }
    }

    WHEN("the null is assigned to") {
      dart_heap_arr_set_null(&arr, 5);
      THEN("it retains the value we expect") {
        auto null = dart_heap_arr_get(&arr, 5);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&null); });
        REQUIRE(dart_heap_is_null(&null));
      }
    }

    WHEN("the null is assigned to from a disparate type") {
      auto nested = dart_heap_obj_init_va_rc(DART_RC_UNSAFE, "sss", "hello", "world", "yes", "no", "stop", "go");
      dart_heap_arr_set_take_heap(&arr, 5, &nested);
      dart_heap_destroy(&nested);
      THEN("it takes on the value we expect") {
        auto nes = dart_heap_arr_get(&arr, 5);
        auto guard = make_scope_guard([&] { dart_heap_destroy(&nes); });
        REQUIRE(dart_heap_is_obj(&nes));
        REQUIRE(dart_heap_size(&nes) == 3U);
      }
    }
  }
}

SCENARIO("heap arrays can erase existing indices", "[heap abi unit]") {
  GIVEN("an array full of stuff") {
    auto arr = dart_heap_arr_init_va("sidb", "hello", 27, 2.99792, true);
    auto guard = make_scope_guard([&] { dart_heap_destroy(&arr); });

    WHEN("the string is erased") {
      dart_heap_arr_erase(&arr, 0);
      THEN("all other indices shift up") {
        auto first = dart_heap_arr_get(&arr, 0);
        auto last = dart_heap_arr_get(&arr, 2);
        REQUIRE(dart_heap_is_int(&first));
        REQUIRE(dart_heap_is_bool(&last));
        REQUIRE(dart_heap_int_get(&first) == 27);
        REQUIRE(dart_heap_bool_get(&last));
      }
    }

    WHEN("the integer is erased") {
      dart_heap_arr_erase(&arr, 1);
      THEN("all later indices shift up") {
        auto first = dart_heap_arr_get(&arr, 0);
        auto last = dart_heap_arr_get(&arr, 2);
        REQUIRE(dart_heap_is_str(&first));
        REQUIRE(dart_heap_is_bool(&last));
        REQUIRE(dart_heap_str_get(&first) == "hello"s);
        REQUIRE(dart_heap_bool_get(&last));
        REQUIRE(dart_heap_size(&arr) == 3U);
      }
    }

    WHEN("the decimal is erased") {
      dart_heap_arr_erase(&arr, 2);
      THEN("the last index shifts up") {
        auto first = dart_heap_arr_get(&arr, 0);
        auto last = dart_heap_arr_get(&arr, 2);
        REQUIRE(dart_heap_is_str(&first));
        REQUIRE(dart_heap_is_bool(&last));
        REQUIRE(dart_heap_str_get(&first) == "hello"s);
        REQUIRE(dart_heap_bool_get(&last));
        REQUIRE(dart_heap_size(&arr) == 3U);
      }
    }

    WHEN("the boolean is erased") {
      dart_heap_arr_erase(&arr, 3);
      THEN("no other indexes are affected") {
        auto first = dart_heap_arr_get(&arr, 0);
        auto last = dart_heap_arr_get(&arr, 2);
        REQUIRE(dart_heap_is_str(&first));
        REQUIRE(dart_heap_is_dcm(&last));
        REQUIRE(dart_heap_str_get(&first) == "hello"s);
        REQUIRE(dart_heap_dcm_get(&last) == 2.99792);
        REQUIRE(dart_heap_size(&arr) == 3U);
      }
    }
  }
}

SCENARIO("heap arrays with unsafe refcounting can erase existing indices", "[heap abi unit]") {
  GIVEN("an array full of stuff") {
    auto arr = dart_heap_arr_init_va_rc(DART_RC_UNSAFE, "sidb", "hello", 27, 2.99792, true);
    auto guard = make_scope_guard([&] { dart_heap_destroy(&arr); });

    WHEN("the string is erased") {
      dart_heap_arr_erase(&arr, 0);
      THEN("all other indices shift up") {
        auto first = dart_heap_arr_get(&arr, 0);
        auto last = dart_heap_arr_get(&arr, 2);
        REQUIRE(dart_heap_is_int(&first));
        REQUIRE(dart_heap_is_bool(&last));
        REQUIRE(dart_heap_int_get(&first) == 27);
        REQUIRE(dart_heap_bool_get(&last));
      }
    }

    WHEN("the integer is erased") {
      dart_heap_arr_erase(&arr, 1);
      THEN("all later indices shift up") {
        auto first = dart_heap_arr_get(&arr, 0);
        auto last = dart_heap_arr_get(&arr, 2);
        REQUIRE(dart_heap_is_str(&first));
        REQUIRE(dart_heap_is_bool(&last));
        REQUIRE(dart_heap_str_get(&first) == "hello"s);
        REQUIRE(dart_heap_bool_get(&last));
        REQUIRE(dart_heap_size(&arr) == 3U);
      }
    }

    WHEN("the decimal is erased") {
      dart_heap_arr_erase(&arr, 2);
      THEN("the last index shifts up") {
        auto first = dart_heap_arr_get(&arr, 0);
        auto last = dart_heap_arr_get(&arr, 2);
        REQUIRE(dart_heap_is_str(&first));
        REQUIRE(dart_heap_is_bool(&last));
        REQUIRE(dart_heap_str_get(&first) == "hello"s);
        REQUIRE(dart_heap_bool_get(&last));
        REQUIRE(dart_heap_size(&arr) == 3U);
      }
    }

    WHEN("the boolean is erased") {
      dart_heap_arr_erase(&arr, 3);
      THEN("no other indexes are affected") {
        auto first = dart_heap_arr_get(&arr, 0);
        auto last = dart_heap_arr_get(&arr, 2);
        REQUIRE(dart_heap_is_str(&first));
        REQUIRE(dart_heap_is_dcm(&last));
        REQUIRE(dart_heap_str_get(&first) == "hello"s);
        REQUIRE(dart_heap_dcm_get(&last) == 2.99792);
        REQUIRE(dart_heap_size(&arr) == 3U);
      }
    }
  }
}

SCENARIO("heap arrays can be iterated over", "[heap abi unit]") {
  GIVEN("an array with contents") {
    auto* dyn = "dynamic";
    auto arr = dart_heap_arr_init_va("idbsS", 1, 3.14159, 0, "fixed", dyn, strlen(dyn));
    auto guard = make_scope_guard([&] { dart_heap_destroy(&arr); });

    WHEN("we create an iterator") {
      // Initialize an iterator for our array.
      dart_iterator_t it;
      dart_iterator_init_from_err(&it, &arr);
      auto guard = make_scope_guard([&] { dart_iterator_destroy(&it); });

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

        REQUIRE(dart_is_int(&one));
        REQUIRE(dart_int_get(&one) == 1);
        REQUIRE(dart_is_dcm(&two));
        REQUIRE(dart_dcm_get(&two) == Approx(3.14159));
        REQUIRE(dart_is_bool(&three));
        REQUIRE(dart_bool_get(&three) == 0);
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
          auto verify = dart_heap_arr_get(&arr, idx++);
          auto guard = make_scope_guard([&] { dart_heap_destroy(&verify); });

          // Check it
          REQUIRE(!dart_is_null(&val));
          REQUIRE(!dart_heap_is_null(&verify));
          REQUIRE(dart_get_type(&val) == dart_get_type(&verify));
          REQUIRE(dart_equal(&val, &verify));
        }
      }
    }
  }
}

SCENARIO("heap arrays with unsafe refcounting can be iterated over", "[heap abi unit]") {
  GIVEN("an array with contents") {
    auto* dyn = "dynamic";
    auto arr = dart_heap_arr_init_va_rc(DART_RC_UNSAFE, "idbsS", 1, 3.14159, 0, "fixed", dyn, strlen(dyn));
    auto guard = make_scope_guard([&] { dart_heap_destroy(&arr); });

    WHEN("we create an iterator") {
      // Initialize an iterator for our array.
      dart_iterator_t it;
      dart_iterator_init_from_err(&it, &arr);
      auto guard = make_scope_guard([&] { dart_iterator_destroy(&it); });

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

        REQUIRE(dart_is_int(&one));
        REQUIRE(dart_int_get(&one) == 1);
        REQUIRE(dart_is_dcm(&two));
        REQUIRE(dart_dcm_get(&two) == Approx(3.14159));
        REQUIRE(dart_is_bool(&three));
        REQUIRE(dart_bool_get(&three) == 0);
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
          auto verify = dart_heap_arr_get(&arr, idx++);
          auto guard = make_scope_guard([&] { dart_heap_destroy(&verify); });

          // Check it
          REQUIRE(!dart_is_null(&val));
          REQUIRE(!dart_heap_is_null(&verify));
          REQUIRE(dart_get_type(&val) == dart_get_type(&verify));
          REQUIRE(dart_equal(&val, &verify));
        }
      }
    }
  }
}

SCENARIO("heap arrays are positional data structures", "[heap abi unit]") {
  GIVEN("an empty array") {
    auto arr = dart_heap_arr_init();
    auto guard = make_scope_guard([&] { dart_heap_destroy(&arr); });

    THEN("it contains no elements") {
      REQUIRE(dart_heap_size(&arr) == 0U);
    }

    WHEN("the array is resized") {
      dart_heap_arr_resize(&arr, 3);
      THEN("it contains empty slots") {
        auto one = dart_heap_arr_get(&arr, 0);
        auto two = dart_heap_arr_get(&arr, 1);
        auto three = dart_heap_arr_get(&arr, 2);
        auto guard = make_scope_guard([&] {
          dart_heap_destroy(&three);
          dart_heap_destroy(&two);
          dart_heap_destroy(&one);
        });
        REQUIRE(dart_heap_size(&arr) == 3U);
        REQUIRE(dart_heap_is_null(&one));
        REQUIRE(dart_heap_is_null(&two));
        REQUIRE(dart_heap_is_null(&three));
      }

      WHEN("elements are inserted in the middle") {
        dart_heap_arr_insert_str(&arr, 1, "middle");
        THEN("later elements shift down") {
          auto lhs = dart_heap_arr_get(&arr, 0);
          auto rhs = dart_heap_arr_get(&arr, 2);
          auto mid = dart_heap_arr_get(&arr, 1);
          auto guard = make_scope_guard([&] {
            dart_heap_destroy(&mid);
            dart_heap_destroy(&rhs);
            dart_heap_destroy(&lhs);
          });
          REQUIRE(dart_heap_is_null(&lhs));
          REQUIRE(dart_heap_is_null(&rhs));
          REQUIRE(dart_heap_is_str(&mid));
          REQUIRE(dart_heap_str_get(&mid) == "middle"s);
          REQUIRE(dart_heap_size(&arr) == 4U);
        }
      }

      WHEN("elements are assigned to in the middle") {
        auto nested = dart_heap_obj_init_va("sss", "hello", "goodbye", "yes", "no", "stop", "go");
        dart_heap_arr_set_take_heap(&arr, 1, &nested);
        dart_heap_destroy(&nested);
        THEN("the index is assigned in place, without affecting neighbors") {
          auto lhs = dart_heap_arr_get(&arr, 0);
          auto rhs = dart_heap_arr_get(&arr, 2);
          auto mid = dart_heap_arr_get(&arr, 1);
          auto guard = make_scope_guard([&] {
            dart_heap_destroy(&mid);
            dart_heap_destroy(&rhs);
            dart_heap_destroy(&lhs);
          });
          REQUIRE(dart_heap_is_null(&lhs));
          REQUIRE(dart_heap_is_null(&rhs));
          REQUIRE(dart_heap_is_obj(&mid));
          REQUIRE(dart_heap_size(&mid) == 3U);
          REQUIRE(dart_heap_obj_has_key(&mid, "hello"));
        }
      }

      WHEN("elements are deleted in the middle") {
        dart_heap_arr_erase(&arr, 1);
        THEN("later elements shift up") {
          auto first = dart_heap_arr_get(&arr, 0);
          auto last = dart_heap_arr_get(&arr, 1);
          auto guard = make_scope_guard([&] {
            dart_heap_destroy(&last);
            dart_heap_destroy(&first);
          });
          REQUIRE(dart_heap_is_null(&first));
          REQUIRE(dart_heap_is_null(&last));
          REQUIRE(dart_heap_size(&arr) == 2U);
        }
      }
    }

    WHEN("the array has space reserved") {
      dart_heap_arr_reserve(&arr, 3);
      THEN("its advertised contents do not change") {
        REQUIRE(dart_heap_size(&arr) == 0U);
      }
    }
  }
}

SCENARIO("heap arrays with unsafe refcounting are positional data structures", "[heap abi unit]") {
  GIVEN("an empty array") {
    auto arr = dart_heap_arr_init_rc(DART_RC_UNSAFE);
    auto guard = make_scope_guard([&] { dart_heap_destroy(&arr); });

    THEN("it contains no elements") {
      REQUIRE(dart_heap_size(&arr) == 0U);
    }

    WHEN("the array is resized") {
      dart_heap_arr_resize(&arr, 3);
      THEN("it contains empty slots") {
        auto one = dart_heap_arr_get(&arr, 0);
        auto two = dart_heap_arr_get(&arr, 1);
        auto three = dart_heap_arr_get(&arr, 2);
        auto guard = make_scope_guard([&] {
          dart_heap_destroy(&three);
          dart_heap_destroy(&two);
          dart_heap_destroy(&one);
        });
        REQUIRE(dart_heap_size(&arr) == 3U);
        REQUIRE(dart_heap_is_null(&one));
        REQUIRE(dart_heap_is_null(&two));
        REQUIRE(dart_heap_is_null(&three));
      }

      WHEN("elements are inserted in the middle") {
        dart_heap_arr_insert_str(&arr, 1, "middle");
        THEN("later elements shift down") {
          auto lhs = dart_heap_arr_get(&arr, 0);
          auto rhs = dart_heap_arr_get(&arr, 2);
          auto mid = dart_heap_arr_get(&arr, 1);
          auto guard = make_scope_guard([&] {
            dart_heap_destroy(&mid);
            dart_heap_destroy(&rhs);
            dart_heap_destroy(&lhs);
          });
          REQUIRE(dart_heap_is_null(&lhs));
          REQUIRE(dart_heap_is_null(&rhs));
          REQUIRE(dart_heap_is_str(&mid));
          REQUIRE(dart_heap_str_get(&mid) == "middle"s);
          REQUIRE(dart_heap_size(&arr) == 4U);
        }
      }

      WHEN("elements are assigned to in the middle") {
        auto nested = dart_heap_obj_init_va_rc(DART_RC_UNSAFE, "sss", "hello", "goodbye", "yes", "no", "stop", "go");
        dart_heap_arr_set_take_heap(&arr, 1, &nested);
        dart_heap_destroy(&nested);
        THEN("the index is assigned in place, without affecting neighbors") {
          auto lhs = dart_heap_arr_get(&arr, 0);
          auto rhs = dart_heap_arr_get(&arr, 2);
          auto mid = dart_heap_arr_get(&arr, 1);
          auto guard = make_scope_guard([&] {
            dart_heap_destroy(&mid);
            dart_heap_destroy(&rhs);
            dart_heap_destroy(&lhs);
          });
          REQUIRE(dart_heap_is_null(&lhs));
          REQUIRE(dart_heap_is_null(&rhs));
          REQUIRE(dart_heap_is_obj(&mid));
          REQUIRE(dart_heap_size(&mid) == 3U);
          REQUIRE(dart_heap_obj_has_key(&mid, "hello"));
        }
      }

      WHEN("elements are deleted in the middle") {
        dart_heap_arr_erase(&arr, 1);
        THEN("later elements shift up") {
          auto first = dart_heap_arr_get(&arr, 0);
          auto last = dart_heap_arr_get(&arr, 1);
          auto guard = make_scope_guard([&] {
            dart_heap_destroy(&last);
            dart_heap_destroy(&first);
          });
          REQUIRE(dart_heap_is_null(&first));
          REQUIRE(dart_heap_is_null(&last));
          REQUIRE(dart_heap_size(&arr) == 2U);
        }
      }
    }

    WHEN("the array has space reserved") {
      dart_heap_arr_reserve(&arr, 3);
      THEN("its advertised contents do not change") {
        REQUIRE(dart_heap_size(&arr) == 0U);
      }
    }
  }
}

#if DART_USING_MSVC
#pragma warning(pop)
#endif
