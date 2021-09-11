// XXX: Bool to int conversion warnings are too noisy otherwise
#if DART_USING_MSVC
#pragma warning(push)
#pragma warning(disable: 4805)
#endif

/*----- System Includes -----*/

#include <cstring>
#include <iostream>

/*----- Local Includes -----*/

#include "../include/extern/catch.h"

#include "../include/dart.h"
#include "../include/dart/abi.h"
#include "../include/dart/api_swap.h"

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

SCENARIO("dart can efficiently swap between APIs", "[api swap unit]") {
  GIVEN("a mutable c++ object with some data in it") {
    // A semi-complicated object
    dart::object obj {
      "hello", "world",
      "data", dart::array {
        1, 1.0, 2, 3.0, 5, 8.0
      },
      "lies", false
    };

    WHEN("that object is converted to the pure-C API") {
      dart_packet_t cobj;
      dart::unsafe_api_swap(&cobj, obj);
      auto guard = make_scope_guard([&] { dart_destroy(&cobj); });

      THEN("the C object behaves as a copy of the original C++ object") {
        REQUIRE(dart_size(&cobj) == 3);
        REQUIRE(static_cast<bool>(dart_is_obj(&cobj)));
        REQUIRE(dart_refcount(&cobj) == obj.refcount());

        // Pull out all individual fields
        auto key_one = dart_obj_get(&cobj, "hello");
        auto key_two = dart_obj_get(&cobj, "data");
        auto key_three = dart_obj_get(&cobj, "lies");
        auto guard = make_scope_guard([&] {
          dart_destroy(&key_three);
          dart_destroy(&key_two);
          dart_destroy(&key_one);
        });

        // Check each of the easy elements
        REQUIRE(static_cast<bool>(dart_is_str(&key_one)));
        REQUIRE(dart_str_get(&key_one) == "world"s);
        REQUIRE(static_cast<bool>(dart_is_bool(&key_three)));
        REQUIRE(dart_bool_get(&key_three) == false);

        // Iterate over the aggregate and check.
        // No scope guard is necessary because
        // dart_for_each manages the lifetime of the loop variable.
        int idx = 0;
        dart_packet_t celem;
        REQUIRE(static_cast<bool>(dart_is_arr(&key_two)));
        dart_for_each(&key_two, &celem) {
          auto elem = obj["data"][idx++];
          switch (dart_get_type(&celem)) {
            case DART_INTEGER:
              REQUIRE(dart_int_get(&celem) == elem);
              break;
            case DART_DECIMAL:
              REQUIRE(dart_dcm_get(&celem) == elem);
              break;
            default:
              FAIL("Unexpected type for dart number");
          }
        }
      }

      WHEN("that converted object is finalized and converted back") {
        dart::packet rebuilt;
        auto cfin = dart_finalize(&cobj);
        dart::unsafe_api_swap(rebuilt, &cfin);
        auto guard = make_scope_guard([&] { dart_destroy(&cfin); });

        THEN("it still compares equal with the original object") {
          REQUIRE(rebuilt.refcount() == dart_refcount(&cfin));
          REQUIRE(rebuilt.get_bytes().data() == dart_get_bytes(&cfin, nullptr));
          REQUIRE(rebuilt == obj);
        }
      }
    }
  }
}

SCENARIO("dart can efficiently swap between APIs for finalized buffers", "[api swap unit]") {
  GIVEN("an immutable c++ object with some data in it") {
    // A semi-complicated object
    dart::buffer::object obj {
      "hello", "world",
      "data", dart::array {
        1, 1.0, 2, 3.0, 5, 8.0
      },
      "lies", false
    };

    WHEN("that object is converted to the pure-C API") {
      dart_buffer_t cobj;
      dart::unsafe_api_swap(&cobj, obj);
      auto guard = make_scope_guard([&] { dart_destroy(&cobj); });

      THEN("the C object behaves as a copy of the original C++ object") {
        REQUIRE(dart_size(&cobj) == 3);
        REQUIRE(static_cast<bool>(dart_is_obj(&cobj)));
        REQUIRE(dart_refcount(&cobj) == obj.refcount());

        // Pull out all individual fields
        auto key_one = dart_obj_get(&cobj, "hello");
        auto key_two = dart_obj_get(&cobj, "data");
        auto key_three = dart_obj_get(&cobj, "lies");
        auto guard = make_scope_guard([&] {
          dart_destroy(&key_three);
          dart_destroy(&key_two);
          dart_destroy(&key_one);
        });

        // Check each of the easy elements
        REQUIRE(static_cast<bool>(dart_is_str(&key_one)));
        REQUIRE(dart_str_get(&key_one) == "world"s);
        REQUIRE(static_cast<bool>(dart_is_bool(&key_three)));
        REQUIRE(dart_bool_get(&key_three) == false);

        // Iterate over the aggregate and check.
        // No scope guard is necessary because
        // dart_for_each manages the lifetime of the loop variable.
        int idx = 0;
        dart_packet_t celem;
        REQUIRE(static_cast<bool>(dart_is_arr(&key_two)));
        dart_for_each(&key_two, &celem) {
          auto elem = obj["data"][idx++];
          switch (dart_get_type(&celem)) {
            case DART_INTEGER:
              REQUIRE(dart_int_get(&celem) == elem);
              break;
            case DART_DECIMAL:
              REQUIRE(dart_dcm_get(&celem) == elem);
              break;
            default:
              FAIL("Unexpected type for dart number");
          }
        }
      }

      WHEN("that converted object is finalized and converted back") {
        dart::buffer rebuilt;
        dart::unsafe_api_swap(rebuilt, &cobj);

        THEN("it still compares equal with the original object") {
          REQUIRE(rebuilt.refcount() == dart_refcount(&cobj));
          REQUIRE(rebuilt.get_bytes().data() == dart_get_bytes(&cobj, nullptr));
          REQUIRE(rebuilt == obj);
        }
      }
    }
  }
}

SCENARIO("dart can efficiently swap between APIs for mutable heaps", "[api swap unit]") {
  GIVEN("an explicitly mutable c++ object with some data in it") {
    // A semi-complicated object
    dart::heap::object obj {
      "hello", "world",
      "data", dart::array {
        1, 1.0, 2, 3.0, 5, 8.0
      },
      "lies", false
    };

    WHEN("that object is converted to the pure-C API") {
      dart_heap_t cobj;
      dart::unsafe_api_swap(&cobj, obj);
      auto guard = make_scope_guard([&] { dart_destroy(&cobj); });

      THEN("the C object behaves as a copy of the original C++ object") {
        REQUIRE(dart_size(&cobj) == 3);
        REQUIRE(static_cast<bool>(dart_is_obj(&cobj)));
        REQUIRE(dart_refcount(&cobj) == obj.refcount());

        // Pull out all individual fields
        auto key_one = dart_obj_get(&cobj, "hello");
        auto key_two = dart_obj_get(&cobj, "data");
        auto key_three = dart_obj_get(&cobj, "lies");
        auto guard = make_scope_guard([&] {
          dart_destroy(&key_three);
          dart_destroy(&key_two);
          dart_destroy(&key_one);
        });

        // Check each of the easy elements
        REQUIRE(static_cast<bool>(dart_is_str(&key_one)));
        REQUIRE(dart_str_get(&key_one) == "world"s);
        REQUIRE(static_cast<bool>(dart_is_bool(&key_three)));
        REQUIRE(dart_bool_get(&key_three) == false);

        // Iterate over the aggregate and check.
        // No scope guard is necessary because
        // dart_for_each manages the lifetime of the loop variable.
        int idx = 0;
        dart_packet_t celem;
        REQUIRE(static_cast<bool>(dart_is_arr(&key_two)));
        dart_for_each(&key_two, &celem) {
          auto elem = obj["data"][idx++];
          switch (dart_get_type(&celem)) {
            case DART_INTEGER:
              REQUIRE(dart_int_get(&celem) == elem);
              break;
            case DART_DECIMAL:
              REQUIRE(dart_dcm_get(&celem) == elem);
              break;
            default:
              FAIL("Unexpected type for dart number");
          }
        }
      }

      WHEN("that converted object is finalized and converted back") {
        dart::heap rebuilt;
        dart::unsafe_api_swap(rebuilt, &cobj);

        THEN("it still compares equal with the original object") {
          REQUIRE(rebuilt.refcount() == dart_refcount(&cobj));
          REQUIRE(rebuilt == obj);
        }
      }
    }
  }
}
