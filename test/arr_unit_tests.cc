/*----- System Includes -----*/

#include <vector>
#include <string>
#include <unordered_set>

/*----- Local Includes -----*/

#include "dart_tests.h"

/*----- Function Implementations -----*/

SCENARIO("arrays can be created", "[array unit]") {
  GIVEN("an array inside an object") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("arr", dart::heap::make_array());
      auto obj = dart::conversion_helper<pkt>(tmp);

      // Check to make sure the type and size agrees.
      auto arr = obj["arr"];
      REQUIRE(arr.is_array());
      REQUIRE(arr.get_type() == dart::packet::type::array);

      // Make sure the array is empty.
      REQUIRE(arr.size() == 0ULL);

      DYNAMIC_WHEN("the object is finalized", idx) {
        obj.finalize();
        DYNAMIC_THEN("basic properties remain the same", idx) {
          REQUIRE(arr == obj["arr"]);
          arr = obj["arr"];

          REQUIRE(arr.is_array());
          REQUIRE(arr.get_type() == dart::packet::type::array);
          REQUIRE(arr.size() == 0ULL);
        }
      }
    });
  }
}

SCENARIO("arrays can be copied", "[array unit]") {
  GIVEN("an array") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("arr", dart::heap::make_array("hello", "goodbye"));
      auto arr = dart::conversion_helper<pkt>(tmp)["arr"];

      // Check the initial refcount.
      REQUIRE(arr.refcount() == 1ULL);
      REQUIRE(arr[0] == "hello");
      REQUIRE(arr[1] == "goodbye");

      DYNAMIC_WHEN("the array is copied", idx) {
        auto copy = arr;
        DYNAMIC_THEN("its reference count goes up", idx) {
          REQUIRE(copy[0] == "hello");
          REQUIRE(copy[1] == "goodbye");
          REQUIRE(arr.refcount() == 2ULL);
          REQUIRE(copy.refcount() == 2ULL);
        }
      }

      DYNAMIC_WHEN("the array is finalized and copied", idx) {
        auto new_arr = pkt::make_object("arr", arr).finalize()["arr"];
        auto copy = new_arr;
        DYNAMIC_THEN("its reference count goes up", idx) {
          REQUIRE(new_arr.refcount() == 2ULL);
          REQUIRE(copy.refcount() == 2ULL);
        }
      }
    });
  }
}

SCENARIO("arrays can be moved", "[array unit]") {
  GIVEN("an array with some contents") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("arr", dart::heap::make_array("hello", "goodbye"));
      auto arr = dart::conversion_helper<pkt>(tmp)["arr"];

      // Check the initial refcount.
      REQUIRE(arr.refcount() == 1ULL);

      DYNAMIC_WHEN("the array is moved", idx) {
        auto new_arr = std::move(arr);
        DYNAMIC_THEN("its reference count does not change", idx) {
          REQUIRE(arr.refcount() == 0ULL);
          REQUIRE(new_arr.refcount() == 1ULL);
          REQUIRE(arr.get_type() == dart::packet::type::null);
          REQUIRE(new_arr.get_type() == dart::packet::type::array);
        }
      }

      DYNAMIC_WHEN("the array is finalized and then moved", idx) {
        auto new_arr = pkt::make_object("arr", arr).finalize()["arr"];
        auto moved_arr = std::move(arr);
        DYNAMIC_THEN("its reference count does not change", idx) {
          REQUIRE(new_arr.refcount() == 1ULL);
          REQUIRE(new_arr.get_type() == dart::packet::type::array);
        }
      }
    });
  }
}

SCENARIO("aliased arrays lazily copy data when mutated", "[array unit]") {
  GIVEN("an array") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto arr = pkt::make_array();
      DYNAMIC_WHEN("an array is nested inside itself", idx) {
        auto copy = arr;
        arr.push_back(copy.push_front(arr));
        DYNAMIC_THEN("it automatically breaks the cycle", idx) {
          REQUIRE(arr.refcount() == 1ULL);
          REQUIRE(copy.refcount() == 2ULL);
        }

        DYNAMIC_WHEN("the array is cleared", idx) {
          copy.pop_front();
          arr.pop_back();
          THEN("refcounts return to normal") {
            REQUIRE(arr.refcount() == 1ULL);
            REQUIRE(copy.refcount() == 1ULL);
          }
        }

        DYNAMIC_WHEN("that array is finalized", idx) {
          auto new_arr = pkt::make_object("arr", arr).finalize()["arr"];
          DYNAMIC_THEN("it behaves as expected", idx) {
            auto new_copy = new_arr[0];
            auto last = copy[0];
            REQUIRE(new_arr.get_type() == dart::packet::type::array);
            REQUIRE(new_arr.size() == 1ULL);
            REQUIRE(new_arr.refcount() == 2ULL);
            REQUIRE(new_copy.get_type() == dart::packet::type::array);
            REQUIRE(new_copy.size() == 1ULL);
            REQUIRE(new_copy.refcount() == 2ULL);
            REQUIRE(last.get_type() == dart::packet::type::array);
            REQUIRE(last.size() == 0ULL);
            REQUIRE(last.refcount() == 2ULL);
          }
        }
      }
    });
  }
}

SCENARIO("arrays can be initialized with contents", "[array unit]") {
  GIVEN("a set of values") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      bool val_twelve = true, val_thirteen = false;
      int val_five = 1, val_six = 1, val_seven = 2, val_eight = 3;
      double val_nine = 3.14159, val_ten = 2.99792, val_eleven = 2.71828;
      std::string val_one = "hello", val_two = "goodbye", val_three = "yes", val_four = "no";

      DYNAMIC_WHEN("arrays are created from them", idx) {
        auto tmp = dart::heap::make_array(val_one, val_two, val_three, val_four);
        auto arr_one = dart::conversion_helper<pkt>(dart::heap::make_object("arr", tmp))["arr"];
        tmp = dart::heap::make_array(val_five, val_six, val_seven, val_eight);
        auto arr_two = dart::conversion_helper<pkt>(dart::heap::make_object("arr", tmp))["arr"];
        tmp = dart::heap::make_array(val_nine, val_ten, val_eleven);
        auto arr_three = dart::conversion_helper<pkt>(dart::heap::make_object("arr", tmp))["arr"];
        tmp = dart::heap::make_array(val_twelve, val_thirteen);
        auto arr_four = dart::conversion_helper<pkt>(dart::heap::make_object("arr", tmp))["arr"];

        DYNAMIC_THEN("they check out", idx) {
          REQUIRE(arr_one[0] == "hello");
          REQUIRE(arr_one[1] == "goodbye");
          REQUIRE(arr_one[2] == "yes");
          REQUIRE(arr_one[3] == "no");
          REQUIRE(arr_two.front() == 1);
          REQUIRE(arr_two[1] == 1);
          REQUIRE(arr_two[2] == 2);
          REQUIRE(arr_two.back() == 3);
          REQUIRE(arr_three[0].decimal() == Approx(3.14159));
          REQUIRE(arr_three[1].decimal() == Approx(2.99792));
          REQUIRE(arr_three[2].decimal() == Approx(2.71828));
          REQUIRE(arr_four[0].boolean());
          REQUIRE_FALSE(arr_four[1].boolean());
        }

        DYNAMIC_WHEN("they're finalized", idx) {
          auto obj = pkt::make_object("one", arr_one, "two", arr_two, "three", arr_three, "four", arr_four);
          obj.finalize();
          arr_one = obj["one"];
          arr_two = obj["two"];
          arr_three = obj["three"];
          arr_four = obj["four"];

          DYNAMIC_THEN("they still check out", idx) {
            REQUIRE(arr_one[0] == "hello");
            REQUIRE(arr_one[1] == "goodbye");
            REQUIRE(arr_one[2] == "yes");
            REQUIRE(arr_one[3] == "no");
            REQUIRE(arr_two.front() == 1);
            REQUIRE(arr_two[1] == 1);
            REQUIRE(arr_two[2] == 2);
            REQUIRE(arr_two.back() == 3);
            REQUIRE(arr_three[0].decimal() == Approx(3.14159));
            REQUIRE(arr_three[1].decimal() == Approx(2.99792));
            REQUIRE(arr_three[2].decimal() == Approx(2.71828));
            REQUIRE(arr_four[0].boolean());
            REQUIRE_FALSE(arr_four[1].boolean());
          }
        }
      }
    });
  }
}

SCENARIO("arrays can add all types of values", "[array unit]") {
  GIVEN("a base array") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto arr = pkt::make_array();
      DYNAMIC_WHEN("we add basically ever type of value under the sun", idx) {
        // Try adding some strings.
        arr.push_back(pkt::make_string("hello"));
        arr.push_back("goodbye");
        arr.push_front(pkt::make_string("yes"));
        arr.push_front("no");

        // Run the gamut to ensure our overloads behave.
        arr.push_back(42);
        arr.push_back(365U);
        arr.push_back(86400L);
        arr.push_back(3600UL);
        arr.push_back(7200LL);
        arr.push_back(93000000ULL);
        arr.push_back(3.14);
        arr.push_back(2.99792);
        arr.push_back(true);
        arr.push_back(false);
        arr.push_back(nullptr);

        DYNAMIC_THEN("it all checks out", idx) {
          // Check everything.
          REQUIRE(arr[0] == "no");
          REQUIRE(arr[1] == "yes");
          REQUIRE(arr[2] == "hello");
          REQUIRE(arr[3] == "goodbye");
          REQUIRE(arr[4].integer() == 42);
          REQUIRE(arr[5].integer() == 365);
          REQUIRE(arr[6].integer() == 86400);
          REQUIRE(arr[7].integer() == 3600);
          REQUIRE(arr[8].integer() == 7200);
          REQUIRE(arr[9].integer() == 93000000);
          REQUIRE(arr[10].decimal() == 3.14);
          REQUIRE(arr[11].decimal() == 2.99792);
          REQUIRE(arr[12].boolean());
          REQUIRE_FALSE(arr[13].boolean());
          REQUIRE(arr[14].get_type() == dart::packet::type::null);
        }

        DYNAMIC_WHEN("the packet is finalized", idx) {
          auto new_arr = pkt::make_object("arr", arr).finalize()["arr"];
          DYNAMIC_THEN("things still check out", idx) {
            REQUIRE(new_arr[0] == "no");
            REQUIRE(new_arr[1] == "yes");
            REQUIRE(new_arr[2] == "hello");
            REQUIRE(new_arr[3] == "goodbye");
            REQUIRE(new_arr[4].integer() == 42);
            REQUIRE(new_arr[5].integer() == 365);
            REQUIRE(new_arr[6].integer() == 86400);
            REQUIRE(new_arr[7].integer() == 3600);
            REQUIRE(new_arr[8].integer() == 7200);
            REQUIRE(new_arr[9].integer() == 93000000);
            REQUIRE(new_arr[10].decimal() == 3.14);
            REQUIRE(new_arr[11].decimal() == 2.99792);
            REQUIRE(new_arr[12].boolean());
            REQUIRE_FALSE(new_arr[13].boolean());
            REQUIRE(new_arr[14].get_type() == dart::packet::type::null);
          }
        }
      }
    });
  }
}

SCENARIO("arrays can be compared for equality", "[array unit]") {
  GIVEN("two empty arrays") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto arr_one = dart::conversion_helper<pkt>(dart::heap::make_object("arr", dart::heap::make_array()))["arr"];
      auto arr_two = dart::conversion_helper<pkt>(dart::heap::make_object("arr", dart::heap::make_array()))["arr"];
      DYNAMIC_WHEN("an array is compared against itself", idx) {
        DYNAMIC_THEN("it compares equal", idx) {
          REQUIRE(arr_one == arr_one);
        }

        DYNAMIC_WHEN("that array is finalized", idx) {
          auto new_arr_one = pkt::make_object("arr", arr_one).finalize()["arr"];
          DYNAMIC_THEN("it still compares equal to itself", idx) {
            REQUIRE(new_arr_one == new_arr_one);
          }
        }
      }

      DYNAMIC_WHEN("two disparate arrays are compared", idx) {
        DYNAMIC_THEN("they still compare equal", idx) {
          REQUIRE(arr_one == arr_two);
        }

        DYNAMIC_WHEN("they are finalized", idx) {
          auto new_arr_one = pkt::make_object("arr", arr_one).finalize()["arr"];
          auto new_arr_two = pkt::make_object("arr", arr_two).finalize()["arr"];
          DYNAMIC_THEN("they STILL compare equal", idx) {
            REQUIRE(new_arr_one == new_arr_two);
          }
        }
      }

      DYNAMIC_WHEN("one array is assigned to the other", idx) {
        arr_two = arr_one;
        DYNAMIC_THEN("they compare equal", idx) {
          REQUIRE(arr_one == arr_two);
        }

        dart::meta::if_constexpr<
          dart::meta::is_packet<pkt>::value
          ||
          dart::meta::is_heap<pkt>::value,
          true
        >(
          [=] (auto& one, auto& two) {
            DYNAMIC_WHEN("one of the arrays is modified", idx) {
              one.push_back("hello");
              DYNAMIC_THEN("they no longer compare equal", idx) {
                REQUIRE(one != two);
              }
            }
          },
          [] (auto&, auto&) {},
          arr_one,
          arr_two
        );
      }
    });
  }

  GIVEN("two arrays with simple, but identical contents") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("arr", dart::heap::make_array(1, 2.0, 3.14159, true, "yes"));
      auto arr_one = dart::conversion_helper<pkt>(tmp)["arr"];
      auto arr_two = dart::conversion_helper<pkt>(tmp)["arr"];

      DYNAMIC_WHEN("they are compared", idx) {
        DYNAMIC_THEN("they compare equal", idx) {
          REQUIRE(arr_one == arr_two);
        }
      }

      DYNAMIC_WHEN("one array is finalized", idx) {
        auto new_arr_one = pkt::make_object("arr", arr_one).finalize()["arr"];
        DYNAMIC_THEN("they still compare equal", idx) {
          REQUIRE(new_arr_one == arr_two);
        }
      }

      DYNAMIC_WHEN("both objects are finalized", idx) {
        auto new_arr_one = pkt::make_object("arr", arr_one).finalize()["arr"];
        auto new_arr_two = pkt::make_object("arr", arr_two).finalize()["arr"];
        DYNAMIC_THEN("they still compare equal", idx) {
          REQUIRE(new_arr_one == new_arr_two);
        }
      }
    });
  }

  GIVEN("two arrays with simple, but different contents") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("arr", dart::heap::make_array(1, 2.0, 3.14159, true, "no"));
      auto arr_one = dart::conversion_helper<pkt>(tmp)["arr"];
      tmp = dart::heap::make_object("arr", dart::heap::make_array(1, 2.0, 3.14159, true, "yes"));
      auto arr_two = dart::conversion_helper<pkt>(tmp)["arr"];

      DYNAMIC_WHEN("they are compared", idx) {
        DYNAMIC_THEN("they do not compare equal", idx) {
          REQUIRE(arr_one != arr_two);
        }
      }

      DYNAMIC_WHEN("one array is finalized", idx) {
        auto new_arr_one = pkt::make_object("arr", arr_one).finalize()["arr"];
        DYNAMIC_THEN("they still do not compare equal", idx) {
          REQUIRE(new_arr_one != arr_two);
        }
      }

      DYNAMIC_WHEN("both objects are finalized", idx) {
        auto new_arr_one = pkt::make_object("arr", arr_one).finalize()["arr"];
        auto new_arr_two = pkt::make_object("arr", arr_two).finalize()["arr"];
        DYNAMIC_THEN("they still do not compare equal", idx) {
          REQUIRE(new_arr_one != new_arr_two);
        }
      }
    });
  }

  GIVEN("two arrays with nested arrays") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("arr", dart::heap::make_array(1, 2.0, 3.14159, true, "yes"));
      auto arr_one = dart::conversion_helper<pkt>(tmp)["arr"];
      auto arr_two = dart::conversion_helper<pkt>(tmp)["arr"];

      DYNAMIC_WHEN("they are compared", idx) {
        DYNAMIC_THEN("they compare equal", idx) {
          REQUIRE(arr_one == arr_two);
        }
      }

      DYNAMIC_WHEN("one object is finalized", idx) {
        auto new_arr_one = pkt::make_object("arr", arr_one).finalize()["arr"];
        DYNAMIC_THEN("they still compare equal", idx) {
          REQUIRE(new_arr_one == arr_two);
        }
      }

      DYNAMIC_WHEN("both objects are finalized", idx) {
        auto new_arr_one = pkt::make_object("arr", arr_one).finalize()["arr"];
        auto new_arr_two = pkt::make_object("arr", arr_two).finalize()["arr"];
        DYNAMIC_THEN("they still compare equal", idx) {
          REQUIRE(new_arr_one == new_arr_two);
        }
      }
    });
  }

  GIVEN("two arrays with nested objects") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto obj = dart::packet::make_object("yes", "no", "one", 1, "pi", 3.14159, "true", true);
      auto tmp = dart::heap::make_object("arr", dart::packet::make_array(obj));
      auto arr_one = dart::conversion_helper<pkt>(tmp)["arr"];
      auto arr_two = dart::conversion_helper<pkt>(tmp)["arr"];

      DYNAMIC_WHEN("they are compared", idx) {
        DYNAMIC_THEN("they compare equal", idx) {
          REQUIRE(arr_one == arr_two);
        }
      }

      DYNAMIC_WHEN("one object is finalized", idx) {
        auto new_arr_one = pkt::make_object("arr", arr_one).finalize()["arr"];
        DYNAMIC_THEN("they still compare equal", idx) {
          REQUIRE(new_arr_one == arr_two);
        }
      }

      DYNAMIC_WHEN("both objects are finalized", idx) {
        auto new_arr_one = pkt::make_object("arr", arr_one).finalize()["arr"];
        auto new_arr_two = pkt::make_object("arr", arr_two).finalize()["arr"];
        DYNAMIC_THEN("they still compare equal", idx) {
          REQUIRE(new_arr_one == new_arr_two);
        }
      }
    });
  }
}

SCENARIO("arrays protect scope of shared resources", "[array unit]") {
  GIVEN("some arrays at an initial scope") {
    dart::packet_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      pkt fin_out_of_scope, dyn_out_of_scope;
      DYNAMIC_WHEN("those arrays are assigned to another that goes out of scope", idx) {
        {
          auto obj = pkt::make_object(), arr = pkt::make_array();
          arr.push_back(1337);
          dyn_out_of_scope = arr;
          obj.add_field("arr", arr);

          // Finalize the packet to test the code below.
          obj.finalize();

          // Keep a copy outside of this scope.
          fin_out_of_scope = obj["arr"];
        }

        DYNAMIC_THEN("the arrays protect shared resources", idx) {
          REQUIRE(fin_out_of_scope.refcount() == 1ULL);
          REQUIRE(dyn_out_of_scope.refcount() == 1ULL);
        }
      }
    });
  }
}

SCENARIO("arrays do not allow out-of-bound access", "[array unit]") {
  GIVEN("an array with some contents") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_array();
      for (int i = 0; i < 57; ++i) tmp.push_back(i);
      auto arr = dart::conversion_helper<pkt>(dart::heap::make_object("arr", tmp))["arr"];
      DYNAMIC_WHEN("an out of bound access is attempted", idx) {
        DYNAMIC_THEN("it throws", idx) {
          REQUIRE_NOTHROW(arr[57]);
          REQUIRE_THROWS_AS(arr.at(57), std::out_of_range);
        }
      }

      DYNAMIC_WHEN("a negative access is attempted", idx) {
        DYNAMIC_THEN("it throws", idx) {
          REQUIRE_NOTHROW(arr[-1]);
          REQUIRE_THROWS_AS(arr.at(-1), std::out_of_range);
        }
      }

      DYNAMIC_WHEN("the array is finalized", idx) {
        auto new_arr = pkt::make_object("arr", arr).finalize()["arr"];
        DYNAMIC_WHEN("an out of bound access is attempted", idx) {
          DYNAMIC_THEN("it throws", idx) {
            REQUIRE_NOTHROW(new_arr[57]);
            REQUIRE_THROWS_AS(new_arr.at(57), std::out_of_range);
          }
        }
        DYNAMIC_WHEN("a negative access is attempted", idx) {
          DYNAMIC_THEN("it throws", idx) {
            REQUIRE_NOTHROW(new_arr[-1]);
            REQUIRE_THROWS_AS(new_arr.at(-1), std::out_of_range);
          }
        }
      }
    });
  }
}

SCENARIO("arrays can add contents at either end", "[array unit]") {
  GIVEN("an array") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto arr = pkt::make_array();
      DYNAMIC_WHEN("contents are pushed onto it", idx) {
        arr.push_back("in the middle");
        arr.push_back("at the end");
        arr.push_front("at the front");

        DYNAMIC_THEN("their order is maintained", idx) {
          REQUIRE(arr[0] == "at the front");
          REQUIRE(arr[1] == "in the middle");
          REQUIRE(arr[2] == "at the end");
          REQUIRE(arr.size() == 3ULL);
        }

        DYNAMIC_WHEN("the array is finalized", idx) {
          auto new_arr = pkt::make_object("arr", arr).finalize()["arr"];
          DYNAMIC_THEN("order is still maintainted", idx) {
            REQUIRE(new_arr[0] == "at the front");
            REQUIRE(new_arr[1] == "in the middle");
            REQUIRE(new_arr[2] == "at the end");
            REQUIRE(new_arr.size() == 3ULL);
          }
        }
      }
    });
  }
}

SCENARIO("arrays can insert contents anywhere", "[array unit]") {
  GIVEN("an array") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto arr = pkt::make_array();
      DYNAMIC_WHEN("insertion at the front, back, and middle is attempted with iterators", idx) {
        arr.insert(arr.begin(), "at the front");
        arr.insert(arr.end(), "at the back");
        arr.insert(++arr.begin(), "in the middle");
        DYNAMIC_THEN("they end up in the right places", idx) {
          REQUIRE(arr.front() == "at the front");
          REQUIRE(arr[1] == "in the middle");
          REQUIRE(arr.back() == "at the back");
        }
      }

      DYNAMIC_WHEN("insertion at the front, back, and middle is attempted with indexes", idx) {
        arr.insert(0, "at the front");
        arr.insert(1, "at the back");
        arr.insert(1, "in the middle");
        DYNAMIC_THEN("they end up in the right places", idx) {
          REQUIRE(arr.front() == "at the front");
          REQUIRE(arr[1] == "in the middle");
          REQUIRE(arr.back() == "at the back");
        }
      }
    });
  }
}

SCENARIO("arrays can erase contents anywhere", "[array unit]") {
  GIVEN("an array") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto arr = pkt::make_array("at the front", "in the middle", "at the back");
      DYNAMIC_WHEN("erasure at the middle, back, and front is attempted with iterators", idx) {
        arr.erase(++arr.begin());
        REQUIRE(arr[1] == "at the back");
        arr.erase(--arr.end());
        REQUIRE(arr[0] == "at the front");
        arr.erase(arr.begin());
        DYNAMIC_THEN("the array checks out", idx) {
          REQUIRE(arr.empty());
          REQUIRE(arr.size() == 0ULL);
        }
      }

      DYNAMIC_WHEN("erasure at the middle, back, and front is attempted with indexes", idx) {
        arr.erase(1);
        REQUIRE(arr[1] == "at the back");
        arr.erase(1);
        REQUIRE(arr[0] == "at the front");
        arr.erase(0);
        DYNAMIC_THEN("the array checks out", idx) {
          REQUIRE(arr.empty());
          REQUIRE(arr.size() == 0ULL);
        }
      }
    });
  }
}

SCENARIO("arrays can remove content at either end", "[array unit]") {
  GIVEN("an array with some contents") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto arr = pkt::make_array("yes", "no", "stop", "go");
      DYNAMIC_WHEN("the front is popped", idx) {
        arr.pop_front();
        DYNAMIC_THEN("it's no longer present", idx) {
          REQUIRE(arr.size() == 3ULL);
          REQUIRE(arr[0] == "no");
        }
      }

      DYNAMIC_WHEN("the back is popped", idx) {
        arr.pop_back();
        DYNAMIC_THEN("it's no longer present", idx) {
          REQUIRE(arr.size() == 3ULL);
          REQUIRE(arr[2] == "stop");
        }
      }

      DYNAMIC_WHEN("everything is popped", idx) {
        arr.pop_back().pop_front().pop_back().pop_front();
        DYNAMIC_THEN("the array is empty", idx) {
          REQUIRE(arr.empty());
        }
      }
    });
  }
}

SCENARIO("arrays can access contents at either end", "[array unit]") {
  GIVEN("an array with contents") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("arr", dart::heap::make_array("front", "back"));
      auto arr = dart::conversion_helper<pkt>(tmp)["arr"];

      // Unconditionally access front and back.
      DYNAMIC_WHEN("the ends are accessed", idx) {
        auto front = arr.front();
        auto back = arr.back();
        DYNAMIC_THEN("correct values are returned", idx) {
          REQUIRE(front == "front");
          REQUIRE(back == "back");
        }
      }

      // Optionally access front and back if we're a mutable packet.
      dart::meta::if_constexpr<
        dart::meta::is_packet<pkt>::value
        ||
        dart::meta::is_heap<pkt>::value,
        true
      >(
        [=] (auto& a) {
          DYNAMIC_WHEN("the ends are optionally accessed", idx) {
            auto front = a.front_or("wont see me");
            auto back = a.back_or("nor me");
            DYNAMIC_THEN("underlying values are returned", idx) {
              REQUIRE(front == "front");
              REQUIRE(back == "back");
            }
          }
        },
        [] (auto&) {},
        arr
      );
    });
  }

  GIVEN("an array without contents") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("arr", dart::heap::make_array());
      auto arr = dart::conversion_helper<pkt>(tmp)["arr"];

      DYNAMIC_WHEN("the ends are accessed", idx) {
        DYNAMIC_THEN("error behavior depends on access", idx) {
          REQUIRE(arr.front().is_null());
          REQUIRE(arr.back().is_null());
          REQUIRE_THROWS_AS(arr.at_front(), std::out_of_range);
          REQUIRE_THROWS_AS(arr.at_back(), std::out_of_range);
        }
      }

      // Optionally access front and back if we're a mutable packet.
      dart::meta::if_constexpr<
        dart::meta::is_packet<pkt>::value
        ||
        dart::meta::is_heap<pkt>::value,
        true
      >(
        [=] (auto& a) {
          DYNAMIC_WHEN("the ends are optionally accessed", idx) {
            auto front = a.front_or("hello");
            auto back = a.back_or("goodbye");
            DYNAMIC_THEN("optional values are returned", idx) {
              REQUIRE(front == "hello");
              REQUIRE(back == "goodbye");
            }
          }
        },
        [] (auto&) {},
        arr
      );
    });
  }
}

SCENARIO("arrays cannot be used as an object", "[array unit]") {
  GIVEN("an array") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto arr = pkt::make_array();
      DYNAMIC_WHEN("using that array as an object", idx) {
        DYNAMIC_THEN("it refuses to do so", idx) {
          REQUIRE_THROWS_AS(arr.add_field("nope", "nope"), std::logic_error);
          REQUIRE_THROWS_AS(arr["oops"], std::logic_error);
        }
      }
    });
  }
}

SCENARIO("naked arrays cannot be finalized", "[array unit]") {
  GIVEN("an array") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      auto arr = pkt::make_array();
      DYNAMIC_WHEN("it's finalized", idx) {
        DYNAMIC_THEN("it refuses to do so", idx) {
          REQUIRE_THROWS_AS(arr.finalize(), std::logic_error);
        }
      }
    });
  }
}

SCENARIO("arrays can contain objects", "[array unit]") {
  GIVEN("an array containing an object") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("arr", dart::heap::make_array(dart::heap::make_object()));
      auto arr = dart::conversion_helper<pkt>(tmp)["arr"];
      DYNAMIC_WHEN("we check the contents", idx) {
        DYNAMIC_THEN("they check out", idx) {
          REQUIRE(arr[0].is_object());
          REQUIRE(arr[0].get_type() == dart::packet::type::object);
          REQUIRE(arr[0].size() == 0ULL);
        }
      }

      DYNAMIC_WHEN("the array is finalized", idx) {
        auto new_arr = pkt::make_object("arr", arr).finalize()["arr"];
        DYNAMIC_THEN("it still checks out", idx) {
          REQUIRE(new_arr[0].is_object());
          REQUIRE(new_arr[0].get_type() == dart::packet::type::object);
          REQUIRE(new_arr[0].size() == 0ULL);
        }
      }
    });
  }
}

SCENARIO("arrays can contain arrays", "[array unit]") {
  GIVEN("an array containing an array") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("arr", dart::heap::make_array(dart::heap::make_array()));
      auto arr = dart::conversion_helper<pkt>(tmp)["arr"];
      DYNAMIC_WHEN("we check the contents", idx) {
        DYNAMIC_THEN("they check out", idx) {
          REQUIRE(arr[0].is_array());
          REQUIRE(arr[0].get_type() == dart::packet::type::array);
          REQUIRE(arr[0].size() == 0ULL);
        }
      }

      DYNAMIC_WHEN("the array is finalized", idx) {
        auto new_arr = pkt::make_object("arr", arr).finalize()["arr"];
        DYNAMIC_THEN("it still checks out", idx) {
          REQUIRE(new_arr[0].is_array());
          REQUIRE(new_arr[0].get_type() == dart::packet::type::array);
          REQUIRE(new_arr[0].size() == 0ULL);
        }
      }
    });
  }
}

SCENARIO("arrays can contain strings", "[array unit]") {
  GIVEN("an array containing a string") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("arr", dart::heap::make_array("hello world"));
      auto arr = dart::conversion_helper<pkt>(tmp)["arr"];
      DYNAMIC_WHEN("we check the contents", idx) {
        DYNAMIC_THEN("they check out", idx) {
          REQUIRE(arr[0].is_str());
          REQUIRE(arr[0].get_type() == dart::packet::type::string);
          REQUIRE(arr[0] == "hello world");
          REQUIRE(arr[0].size() == strlen("hello world"));
        }
      }

      DYNAMIC_WHEN("the array is finalized", idx) {
        auto new_arr = pkt::make_object("arr", arr).finalize()["arr"];
        DYNAMIC_THEN("they check out", idx) {
          REQUIRE(new_arr[0].is_str());
          REQUIRE(new_arr[0].get_type() == dart::packet::type::string);
          REQUIRE(new_arr[0] == "hello world");
          REQUIRE(new_arr[0].size() == strlen("hello world"));
        }
      }
    });
  }
}

SCENARIO("arrays can contain integers", "[array unit]") {
  GIVEN("an array containing an integer") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      auto tmp = dart::heap::make_object("arr", dart::heap::make_array(1337));
      auto arr = dart::conversion_helper<pkt>(tmp)["arr"];
      DYNAMIC_WHEN("we check the contents", idx) {
        DYNAMIC_THEN("they check out", idx) {
          REQUIRE(arr[0].is_integer());
          REQUIRE(arr[0].get_type() == dart::packet::type::integer);
          REQUIRE(arr[0].integer() == 1337);
        }
      }

      DYNAMIC_WHEN("the array is finalized", idx) {
        auto new_arr = pkt::make_object("arr", arr).finalize()["arr"];
        DYNAMIC_THEN("it still checks out", idx) {
          REQUIRE(new_arr[0].is_integer());
          REQUIRE(new_arr[0].get_type() == dart::packet::type::integer);
          REQUIRE(new_arr[0].integer() == 1337);
        }
      }
    });
  }
}

SCENARIO("arrays can contain floats", "[array unit]") {
  GIVEN("an object containing a float") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;
      auto tmp = dart::heap::make_object("arr", dart::heap::make_array(3.14159));
      auto arr = dart::conversion_helper<pkt>(tmp)["arr"];
      DYNAMIC_WHEN("we check the contents", idx) {
        DYNAMIC_THEN("they check out", idx) {
          REQUIRE(arr[0].is_decimal());
          REQUIRE(arr[0].get_type() == dart::packet::type::decimal);
          REQUIRE(arr[0].decimal() == 3.14159);
        }
      }

      DYNAMIC_WHEN("the array is finalized", idx) {
        auto new_arr = pkt::make_object("arr", arr).finalize()["arr"];
        DYNAMIC_THEN("it still checks out", idx) {
          REQUIRE(new_arr[0].is_decimal());
          REQUIRE(new_arr[0].get_type() == dart::packet::type::decimal);
          REQUIRE(new_arr[0].decimal() == 3.14159);
        }
      }
    });
  }
}

SCENARIO("arrays can contain booleans", "[array unit]") {
  GIVEN("an array containing a boolean") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("arr", dart::heap::make_array(true));
      auto arr = dart::conversion_helper<pkt>(tmp)["arr"];
      DYNAMIC_WHEN("we check the contents", idx) {
        DYNAMIC_THEN("they check out", idx) {
          REQUIRE(arr[0].is_boolean());
          REQUIRE(arr[0].get_type() == dart::packet::type::boolean);
          REQUIRE(arr[0].boolean());
        }
      }

      DYNAMIC_WHEN("the array is finalized", idx) {
        auto new_arr = pkt::make_object("arr", arr).finalize()["arr"];
        DYNAMIC_THEN("it still checks out", idx) {
          REQUIRE(arr[0].is_boolean());
          REQUIRE(arr[0].get_type() == dart::packet::type::boolean);
          REQUIRE(arr[0].boolean());
        }
      }
    });
  }
}

SCENARIO("arrays can contain nulls", "[array unit]") {
  GIVEN("an array containing a null value") {
    dart::api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto tmp = dart::heap::make_object("arr", dart::heap::make_array(dart::heap::null()));
      auto arr = dart::conversion_helper<pkt>(tmp)["arr"];
      DYNAMIC_WHEN("we check the contents", idx) {
        DYNAMIC_THEN("they check out", idx) {
          REQUIRE(arr[0].is_null());
        }
      }

      DYNAMIC_WHEN("the array is finalized", idx) {
        auto new_arr = pkt::make_object("arr", arr).finalize()["arr"];
        DYNAMIC_THEN("it still checks out", idx) {
          REQUIRE(arr[0].is_null());
        }
      }
    });
  }
}

SCENARIO("arrays can optionally access non-existent elements with a fallback", "[array unit]") {
  using namespace dart::literals;

  GIVEN("an empty array") {
    dart::mutable_api_test([] (auto tag, auto idx) {
      using pkt = typename decltype(tag)::type;

      auto arr = pkt::make_array();
      DYNAMIC_WHEN("we attempt to optionally access a non-existent index", idx) {
        auto key = dart::conversion_helper<pkt>(dart::packet::make_integer(0));
        auto opt_one = arr.get_or(0, 1);
        auto opt_two = arr.get_or(key, 1.0);
        auto opt_three = arr.get_or(0, "not here");
        auto opt_four = arr.get_or(key, false);
        auto opt_five = arr.get_or(0, pkt::make_array());

        DYNAMIC_THEN("it returns the optional value", idx) {
          REQUIRE(opt_one == 1);
          REQUIRE(opt_two == 1.0);
          REQUIRE(opt_three == "not here");
          REQUIRE(opt_four == false);
          REQUIRE(opt_five == pkt::make_array());
        }
      }

      DYNAMIC_WHEN("we attempt to optionally access a non-existent index on a temporary", idx) {
        arr.push_back(nullptr);
        auto key = dart::conversion_helper<pkt>(dart::packet::make_integer(0));
        auto opt_one = arr[0].get_or(0, 1);
        auto opt_two = arr[0].get_or(key, 1.0);
        auto opt_three = arr[0].get_or(0, "not here");
        auto opt_four = arr[0].get_or(key, false);
        auto opt_five = arr[0].get_or(0, pkt::make_array());

        DYNAMIC_THEN("it returns the optional value", idx) {
          REQUIRE(opt_one == 1);
          REQUIRE(opt_two == 1.0);
          REQUIRE(opt_three == "not here");
          REQUIRE(opt_four == false);
          REQUIRE(opt_five == pkt::make_array());
        }
      }

      dart::meta::if_constexpr<
        dart::meta::is_packet<pkt>::value,
        true
      >(
        [=] (auto& a) {
          DYNAMIC_WHEN("the array is finalized", idx) {
            a = pkt::make_object("arr", a).finalize()["arr"];

            auto key = dart::conversion_helper<pkt>(dart::packet::make_integer(0));
            auto opt_one = a.get_or(0, 1);
            auto opt_two = a.get_or(key, 1.0);
            auto opt_three = a.get_or(0, "not here");
            auto opt_four = a.get_or(key, false);
            auto opt_five = a.get_or(0, pkt::make_array());

            DYNAMIC_THEN("it still behaves as expcted", idx) {
              REQUIRE(opt_one == 1);
              REQUIRE(opt_two == 1.0);
              REQUIRE(opt_three == "not here");
              REQUIRE(opt_four == false);
              REQUIRE(opt_five == pkt::make_array());
            }
          }
        },
        [] (auto&) {},
        arr
      );
    });
  }
}
