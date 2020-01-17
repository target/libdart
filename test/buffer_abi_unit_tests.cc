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

SCENARIO("dart buffers are regular types", "[buffer abi unit]") {
  GIVEN("a default constructed object") {
    // Get an object, make sure it's cleaned up.
    auto mut = dart_obj_init();
    auto guard = make_scope_guard([&] { dart_destroy(&mut); });

    WHEN("the object is queried") {
      auto fin = dart_to_buffer(&mut);
      auto guard = make_scope_guard([&] { dart_buffer_destroy(&fin); });
      THEN("its basic properties make sense") {
        REQUIRE(dart_buffer_size(&fin) == 0);
        REQUIRE(static_cast<bool>(dart_is_obj(&fin)));
        REQUIRE(fin.rtti.p_id == DART_BUFFER);
        REQUIRE(fin.rtti.rc_id == DART_RC_SAFE);
        REQUIRE(dart_buffer_get_type(&fin) == DART_OBJECT);
      }
    }

    WHEN("keys are inserted") {
      // Insert some values into our object.
      dart_obj_insert_str(&mut, "hello", "world");
      dart_obj_insert_int(&mut, "int", 5);
      dart_obj_insert_dcm(&mut, "pi", 3.14159);
      dart_obj_insert_bool(&mut, "bool", true);

      auto fin = dart_to_buffer(&mut);
      auto guard = make_scope_guard([&] { dart_buffer_destroy(&fin); });
      THEN("the keys are accessible") {
        // Grab each key for validation, make sure it's cleaned up.
        REQUIRE(dart_obj_has_key(&fin, "hello"));
        REQUIRE(dart_obj_has_key(&fin, "int"));
        REQUIRE(dart_obj_has_key(&fin, "pi"));
        REQUIRE(dart_obj_has_key(&fin, "bool"));
        REQUIRE(dart_buffer_size(&fin) == 4U);
        auto key_one = dart_buffer_obj_get(&fin, "hello");
        auto key_two = dart_buffer_obj_get_len(&fin, "int", strlen("int"));
        auto key_three = dart_buffer_obj_get(&fin, "pi");
        auto key_four = dart_buffer_obj_get_len(&fin, "bool", strlen("bool"));
        auto guard = make_scope_guard([&] {
          dart_buffer_destroy(&key_one);
          dart_buffer_destroy(&key_two);
          dart_buffer_destroy(&key_three);
          dart_buffer_destroy(&key_four);
        });
        REQUIRE(dart_buffer_is_str(&key_one));
        REQUIRE(dart_buffer_str_get(&key_one) == "world"s);
        REQUIRE(dart_buffer_is_int(&key_two));
        REQUIRE(dart_buffer_int_get(&key_two) == 5);
        REQUIRE(dart_buffer_is_dcm(&key_three));
        REQUIRE(dart_buffer_dcm_get(&key_three) == 3.14159);
        REQUIRE(dart_is_bool(&key_four));
        REQUIRE(static_cast<bool>(dart_buffer_bool_get(&key_four)) == true);
      }
    }

    WHEN("aggregates are inserted") {
      auto nested = dart_obj_init_rc(DART_RC_SAFE);
      dart_obj_insert_str(&nested, "a nested", "string");
      dart_obj_insert_dart(&mut, "nested", &nested);

      auto fin = dart_to_buffer(&mut);
      auto guard = make_scope_guard([&] {
        dart_buffer_destroy(&fin);
        dart_destroy(&nested);
      });
      THEN("it's recursively queryable") {
        auto nested_copy = dart_buffer_obj_get(&fin, "nested");
        auto nested_str = dart_buffer_obj_get(&nested_copy, "a nested");
        auto guard = make_scope_guard([&] {
          dart_buffer_destroy(&nested_str);
          dart_buffer_destroy(&nested_copy);
        });
        REQUIRE(dart_buffer_is_str(&nested_str));
        REQUIRE(dart_buffer_str_get(&nested_str) == "string"s);
        REQUIRE(dart_size(&mut) == 1U);
        REQUIRE(dart_is_obj(&nested_copy));
        REQUIRE(dart_buffer_size(&nested_copy) == 1U);
        REQUIRE(dart_equal(&nested_copy, &nested));
      }
    }

    WHEN("objects are copied") {
      auto fin = dart_to_buffer(&mut);
      auto copy = dart_buffer_copy(&fin);
      auto guard = make_scope_guard([&] {
        dart_buffer_destroy(&copy);
        dart_buffer_destroy(&fin);
      });

      THEN("it is indistinguishable from the original") {
        REQUIRE(dart_buffer_equal(&copy, &fin));
        REQUIRE(dart_equal(&copy, &mut));
        REQUIRE(dart_buffer_size(&copy) == dart_buffer_size(&fin));
        REQUIRE(dart_buffer_get_type(&copy) == dart_buffer_get_type(&fin));
      }
    }

    WHEN("objects are moved") {
      auto fin = dart_to_buffer(&mut);
      auto moved = dart_buffer_move(&fin);
      auto guard = make_scope_guard([&] {
        dart_buffer_destroy(&moved);
        dart_buffer_destroy(&fin);
      });
      THEN("the new object steals the contents of the old") {
        REQUIRE(dart_size(&moved) == 0);
        REQUIRE(dart_is_obj(&moved));
        REQUIRE(dart_buffer_get_type(&moved) == DART_OBJECT);
        REQUIRE(!dart_is_obj(&fin));
        REQUIRE(dart_buffer_is_null(&fin));
        REQUIRE(dart_buffer_get_type(&fin) == DART_NULL);
      }
    }
  }

  GIVEN("a default constructed null") {
    // Get an array, make sure it's cleaned up.
    auto pkt = dart_buffer_init();
    auto guard = make_scope_guard([&] { dart_buffer_destroy(&pkt); });

    WHEN("the null is queried") {
      THEN("its basic properties make sense") {
        REQUIRE(dart_buffer_is_null(&pkt));
        REQUIRE(dart_bool_get(&pkt) == false);
        REQUIRE(pkt.rtti.p_id == DART_BUFFER);
        REQUIRE(pkt.rtti.rc_id == DART_RC_SAFE);
        REQUIRE(dart_buffer_get_type(&pkt) == DART_NULL);
      }
    }

    WHEN("the null is copied") {
      auto copy = dart_buffer_copy(&pkt);
      auto guard = make_scope_guard([&] { dart_buffer_destroy(&copy); });

      THEN("it is indistinguishable from the original") {
        REQUIRE(dart_equal(&copy, &pkt));
        REQUIRE(dart_buffer_get_type(&copy) == dart_buffer_get_type(&pkt));
      }
    }

    WHEN("the null is moved") {
      auto moved = dart_buffer_move(&pkt);
      auto guard = make_scope_guard([&] { dart_buffer_destroy(&moved); });
      THEN("null instances are indistinguishable") {
        auto third = dart_buffer_init();
        auto guard = make_scope_guard([&] { dart_buffer_destroy(&third); });

        REQUIRE(dart_buffer_is_null(&moved));
        REQUIRE(dart_buffer_is_null(&pkt));
        REQUIRE(dart_equal(&moved, &pkt));
        REQUIRE(dart_equal(&third, &pkt));
        REQUIRE(dart_equal(&third, &moved));
      }
    }
  }
}

SCENARIO("buffer objects can be constructed with many values", "[buffer abi unit]") {
  GIVEN("many test cases to run") {
    WHEN("an object is constructed with many values") {
      auto* str = "runtime";
      auto mut = dart_obj_init_va("Ssbdi", "Str", str, strlen(str),
          "str", "string", "bool", true, "decimal", 2.99792, "integer", 1337);
      auto fin = dart_to_buffer(&mut);
      auto guard = make_scope_guard([&] {
        dart_buffer_destroy(&fin);
        dart_destroy(&mut);
      });

      THEN("everything winds up where it's supposed to") {
        auto sized_str = dart_buffer_obj_get(&fin, "Str");
        auto str = dart_buffer_obj_get(&fin, "str");
        auto boolean = dart_buffer_obj_get(&fin, "bool");
        auto decimal = dart_buffer_obj_get(&fin, "decimal");
        auto integer = dart_buffer_obj_get(&fin, "integer");
        auto guard = make_scope_guard([&] {
          dart_buffer_destroy(&integer);
          dart_buffer_destroy(&decimal);
          dart_buffer_destroy(&boolean);
          dart_buffer_destroy(&str);
          dart_buffer_destroy(&sized_str);
        });

        REQUIRE(dart_buffer_str_get(&sized_str) == "runtime"s);
        REQUIRE(dart_buffer_str_get(&str) == "string"s);
        REQUIRE(static_cast<bool>(dart_buffer_bool_get(&boolean)) == true);
        REQUIRE(dart_buffer_dcm_get(&decimal) == Approx(2.99792));
        REQUIRE(dart_buffer_int_get(&integer) == 1337);
      }
    }

    WHEN("an object is constructed with many nested objects") {
      auto* str = "runtime";
      auto mut = dart_obj_init_va("Soos,i,as", "str", str, strlen(str),
          "nested", "double_nested", "double_nested_str", "deep", "integer", 10, "arr", "last");
      auto fin = dart_to_buffer(&mut);
      auto guard = make_scope_guard([&] {
        dart_buffer_destroy(&fin);
        dart_destroy(&mut);
      });

      THEN("everything winds up where it's supposed to") {
        auto str = dart_buffer_obj_get(&fin, "str");
        auto nested = dart_buffer_obj_get(&fin, "nested");
        auto double_nested = dart_buffer_obj_get(&nested, "double_nested");
        auto double_nested_str = dart_buffer_obj_get(&double_nested, "double_nested_str");
        auto integer = dart_buffer_obj_get(&nested, "integer");
        auto arr = dart_buffer_obj_get(&fin, "arr");
        auto last = dart_buffer_arr_get(&arr, 0);
        auto guard = make_scope_guard([&] {
          dart_buffer_destroy(&last);
          dart_buffer_destroy(&arr);
          dart_buffer_destroy(&integer);
          dart_buffer_destroy(&double_nested_str);
          dart_buffer_destroy(&double_nested);
          dart_buffer_destroy(&nested);
          dart_buffer_destroy(&str);
        });

        REQUIRE(dart_buffer_str_get(&str) == "runtime"s);
        REQUIRE(dart_is_obj(&nested));
        REQUIRE(dart_buffer_size(&nested) == 2U);
        REQUIRE(dart_is_obj(&double_nested));
        REQUIRE(dart_buffer_size(&double_nested) == 1U);
        REQUIRE(dart_buffer_str_get(&double_nested_str) == "deep"s);
        REQUIRE(dart_buffer_int_get(&integer) == 10);
        REQUIRE(dart_is_arr(&arr));
        REQUIRE(dart_buffer_size(&arr) == 1U);
        REQUIRE(dart_buffer_str_get(&last) == "last"s);
      }
    }
  }
}

SCENARIO("buffer objects can be iterated over", "[buffer abi unit]") {
  GIVEN("an object with contents") {
    auto* dyn = "dynamic";
    auto mut = dart_obj_init_va("idbsS", "int", 1,
        "decimal", 3.14159, "bool", 0, "str", "fixed", "Str", dyn, strlen(dyn));
    auto fin = dart_to_buffer(&mut);
    auto guard = make_scope_guard([&] {
      dart_buffer_destroy(&fin);
      dart_destroy(&mut);
    });

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
      dart_iterator_init_from_err(&it, &fin);
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
        REQUIRE(dart_bool_get(&four) == false);
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
      dart_iterator_init_key_from_err(&it, &fin);
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
      auto arr = dart_arr_init_va("Sisbd", dyn, strlen(dyn), 1, "fixed", 0, 3.14159);
      auto guard = make_scope_guard([&] { dart_destroy(&arr); });
      THEN("it visits all values in the expected order") {
        dart_for_each(&fin, &val) {
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

    WHEN("we use automatic key iteration") {
      int idx = 0;
      dart_packet_t val;
      auto arr = dart_arr_init_va_rc(DART_RC_SAFE, "sssss", "Str", "int", "str", "bool", "decimal");
      auto guard = make_scope_guard([&] { dart_destroy(&arr); });
      THEN("it visits all keys in the expected order") {
        dart_for_each_key(&fin, &val) {
          // Get the key manually.
          auto verify = dart_arr_get(&arr, idx++);
          auto guard = make_scope_guard([&] { dart_destroy(&verify); });

          // Check it.
          REQUIRE(dart_equal(&val, &verify));
        }
      }
    }
  }
}

SCENARIO("buffer objects can switch between finalized and non-finalized representations", "[buffer abi unit]") {
  GIVEN("an object with lots of contents") {
    auto mut = dart_obj_init_va("sass,oidb,sidbn",
        "hello", "world", "arr", "one", "two",
        "obj", "nest_int", 1337, "nest_dcm", 3.14159, "nest_bool", true,
        "yes", "no", "int", 1337, "dcm", 3.14159, "bool", true, "none");
    auto fin = dart_to_buffer(&mut);
    auto guard = make_scope_guard([&] {
      dart_buffer_destroy(&fin);
      dart_destroy(&mut);
    });

    WHEN("the object is definalized") {
      // These functions are equivalent
      auto defin = dart_buffer_definalize(&fin);
      auto liftd = dart_buffer_lift(&fin);
      auto guard = make_scope_guard([&] {
        dart_heap_destroy(&liftd);
        dart_heap_destroy(&defin);
      });

      THEN("it still compares equal with its original representation") {
        REQUIRE(!dart_is_finalized(&defin));
        REQUIRE(!dart_is_finalized(&liftd));
        REQUIRE(dart_equal(&defin, &liftd));
        REQUIRE(dart_equal(&fin, &defin));
        REQUIRE(dart_equal(&defin, &fin));
        REQUIRE(dart_equal(&fin, &liftd));
        REQUIRE(dart_equal(&liftd, &fin));
      }
    }
  }
}

SCENARIO("finalized buffer objects have unique object representations") {
  GIVEN("two independent, but equivalent, objects") {
    auto objone = dart_obj_init_va("sass,oidb,sidbn",
        "hello", "world", "arr", "one", "two",
        "obj", "nest_int", 1337, "nest_dcm", 3.14159, "nest_bool", true,
        "yes", "no", "int", 1337, "dcm", 3.14159, "bool", true, "none");
    auto objtwo = dart_obj_init_va("sass,oidb,sidbn",
        "hello", "world", "arr", "one", "two",
        "obj", "nest_int", 1337, "nest_dcm", 3.14159, "nest_bool", true,
        "yes", "no", "int", 1337, "dcm", 3.14159, "bool", true, "none");
    auto guard = make_scope_guard([&] {
      dart_destroy(&objtwo);
      dart_destroy(&objone);
    });

    WHEN("the objects are finalized") {
      auto finone = dart_to_buffer(&objone);
      auto fintwo = dart_to_buffer(&objtwo);
      auto guard = make_scope_guard([&] {
        dart_buffer_destroy(&fintwo);
        dart_buffer_destroy(&finone);
      });

      THEN("they produce the same byte representation") {
        size_t lenone, lentwo;
        auto* bytesone = dart_buffer_get_bytes(&finone, &lenone);
        auto* bytestwo = dart_buffer_get_bytes(&fintwo, &lentwo);
        REQUIRE(lenone == lentwo);
        REQUIRE(std::memcmp(bytesone, bytestwo, lenone) == 0);

        auto* ownone = dart_buffer_dup_bytes(&finone, &lenone);
        auto* owntwo = dart_buffer_dup_bytes(&fintwo, nullptr);
        auto guard = make_scope_guard([&] {
          free(owntwo);
          free(ownone);
        });
        REQUIRE(lenone == lentwo);
        REQUIRE(std::memcmp(ownone, owntwo, lenone) == 0);
      }

      THEN("they can be reconstituted") {
        size_t lenone;
        auto* bytes = dart_buffer_get_bytes(&finone, &lenone);
        auto recone = dart_buffer_from_bytes(bytes, lenone);
        auto rectwo = dart_buffer_from_bytes_rc(bytes, DART_RC_SAFE, lenone);
        auto guard = make_scope_guard([&] {
          dart_buffer_destroy(&rectwo);
          dart_buffer_destroy(&recone);
        });

        REQUIRE(dart_equal(&recone, &objone));
        REQUIRE(dart_equal(&rectwo, &objone));
        REQUIRE(dart_buffer_equal(&recone, &finone));
        REQUIRE(dart_buffer_equal(&rectwo, &finone));
        REQUIRE(std::memcmp(dart_buffer_get_bytes(&recone, nullptr), bytes, lenone) == 0);
        REQUIRE(std::memcmp(dart_buffer_get_bytes(&rectwo, nullptr), bytes, lenone) == 0);
      }
    }
  }
}

#if DART_USING_MSVC
#pragma warning(pop)
#endif
