/*----- System Includes -----*/

#include <dart.h>
#include <dart/ptrs.h>
#include <extern/catch.h>

/*----- Function Implementations -----*/

SCENARIO("unsafe pointers can be created", "[pointer unit]") {
  GIVEN("an empty unsafe pointer") {
    dart::unsafe_ptr<dart::packet> ptr;

    THEN("its basic properties make sense") {
      REQUIRE(ptr.get() == nullptr);
      REQUIRE(ptr.use_count() == 0);
      REQUIRE_FALSE(static_cast<bool>(ptr));
    }
  }
}

SCENARIO("skinny pointers can be created", "[pointer unit]") {
  GIVEN("an empty skinny pointer") {
    dart::skinny_ptr<dart::packet> ptr;

    THEN("its basic properties make sense") {
      REQUIRE(ptr.get() == nullptr);
      REQUIRE(ptr.use_count() == 0);
      REQUIRE_FALSE(static_cast<bool>(ptr));
    }
  }
}

SCENARIO("unsafe array pointers can be created", "[pointer unit]") {
  GIVEN("an empty unsafe pointer") {
    dart::unsafe_ptr<dart::packet[]> ptr;

    THEN("its basic properties make sense") {
      REQUIRE(ptr.get() == nullptr);
      REQUIRE(ptr.use_count() == 0);
      REQUIRE_FALSE(static_cast<bool>(ptr));
    }
  }
}

SCENARIO("skinny array pointers can be created", "[pointer unit]") {
  GIVEN("an empty skinny pointer") {
    dart::skinny_ptr<dart::packet[]> ptr;

    THEN("its basic properties make sense") {
      REQUIRE(ptr.get() == nullptr);
      REQUIRE(ptr.use_count() == 0);
      REQUIRE_FALSE(static_cast<bool>(ptr));
    }
  }
}

SCENARIO("empty unsafe pointers can be copied", "[pointer unit]") {
  GIVEN("an empty unsafe pointer") {
    dart::unsafe_ptr<dart::packet> ptr;

    WHEN("the pointer is copied") {
      auto copy = ptr;
      THEN("both resulting pointers are empty") {
        REQUIRE(ptr.get() == nullptr);
        REQUIRE(copy.get() == nullptr);
        REQUIRE(ptr.use_count() == 0);
        REQUIRE(copy.use_count() == 0);
        REQUIRE_FALSE(static_cast<bool>(ptr));
        REQUIRE_FALSE(static_cast<bool>(copy));
      }
    }
  }
}

SCENARIO("empty skinny pointers can be copied", "[pointer unit]") {
  GIVEN("an empty skinny pointer") {
    dart::skinny_ptr<dart::packet> ptr;

    WHEN("the pointer is copied") {
      auto copy = ptr;
      THEN("both resulting pointers are empty") {
        REQUIRE(ptr.get() == nullptr);
        REQUIRE(copy.get() == nullptr);
        REQUIRE(ptr.use_count() == 0);
        REQUIRE(copy.use_count() == 0);
        REQUIRE_FALSE(static_cast<bool>(ptr));
        REQUIRE_FALSE(static_cast<bool>(copy));
      }
    }
  }
}

SCENARIO("empty unsafe array pointers can be copied", "[pointer unit]") {
  GIVEN("an empty unsafe pointer") {
    dart::unsafe_ptr<dart::packet[]> ptr;

    WHEN("the pointer is copied") {
      auto copy = ptr;
      THEN("both resulting pointers are empty") {
        REQUIRE(ptr.get() == nullptr);
        REQUIRE(copy.get() == nullptr);
        REQUIRE(ptr.use_count() == 0);
        REQUIRE(copy.use_count() == 0);
        REQUIRE_FALSE(static_cast<bool>(ptr));
        REQUIRE_FALSE(static_cast<bool>(copy));
      }
    }
  }
}

SCENARIO("empty skinny array pointers can be copied", "[pointer unit]") {
  GIVEN("an empty skinny pointer") {
    dart::skinny_ptr<dart::packet[]> ptr;

    WHEN("the pointer is copied") {
      auto copy = ptr;
      THEN("both resulting pointers are empty") {
        REQUIRE(ptr.get() == nullptr);
        REQUIRE(copy.get() == nullptr);
        REQUIRE(ptr.use_count() == 0);
        REQUIRE(copy.use_count() == 0);
        REQUIRE_FALSE(static_cast<bool>(ptr));
        REQUIRE_FALSE(static_cast<bool>(copy));
      }
    }
  }
}

SCENARIO("unsafe pointers can be initialized with contents", "[pointer unit]") {
  GIVEN("an object to be managed") {
    auto obj = dart::packet::object("hello", "world");

    WHEN("we create a unsafe pointer to that object") {
      auto ptr = dart::make_unsafe<dart::packet>(std::move(obj));
      THEN("the resulting pointer is non-empty and exclusive") {
        REQUIRE(ptr.get() != nullptr);
        REQUIRE(ptr->is_object());
        REQUIRE(static_cast<bool>(ptr));
        REQUIRE(ptr->get("hello") == "world");
        REQUIRE(ptr.use_count() == 1);
        REQUIRE(ptr.unique());
      }
    }
  }
}

SCENARIO("skinny pointers can be initialized with contents", "[pointer unit]") {
  GIVEN("an object to be managed") {
    auto obj = dart::packet::object("hello", "world");

    WHEN("we create a skinny pointer to that object") {
      auto ptr = dart::make_skinny<dart::packet>(std::move(obj));
      THEN("the resulting pointer is non-empty and exclusive") {
        REQUIRE(ptr.get() != nullptr);
        REQUIRE(ptr->is_object());
        REQUIRE(static_cast<bool>(ptr));
        REQUIRE(ptr->get("hello") == "world");
        REQUIRE(ptr.use_count() == 1);
        REQUIRE(ptr.unique());
      }
    }
  }
}

SCENARIO("unsafe array pointers can be initialized with contents", "[pointer unit]") {
  GIVEN("an object to be managed") {
    auto obj = dart::packet::object("hello", "world");

    WHEN("we create a unsafe pointer to that object") {
      auto ptr = dart::make_unsafe<dart::packet[]>(1);
      ptr[0] = std::move(obj);
      THEN("the resulting pointer is non-empty and exclusive") {
        REQUIRE(ptr.get() != nullptr);
        REQUIRE(ptr[0].is_object());
        REQUIRE(static_cast<bool>(ptr));
        REQUIRE(ptr[0].get("hello") == "world");
        REQUIRE(ptr.use_count() == 1);
        REQUIRE(ptr.unique());
      }
    }
  }
}

SCENARIO("skinny array pointers can be initialized with contents", "[pointer unit]") {
  GIVEN("an object to be managed") {
    auto obj = dart::packet::object("hello", "world");

    WHEN("we create a skinny pointer to that object") {
      auto ptr = dart::make_skinny<dart::packet[]>(1);
      ptr[0] = std::move(obj);
      THEN("the resulting pointer is non-empty and exclusive") {
        REQUIRE(ptr.get() != nullptr);
        REQUIRE(ptr[0].is_object());
        REQUIRE(static_cast<bool>(ptr));
        REQUIRE(ptr[0].get("hello") == "world");
        REQUIRE(ptr.use_count() == 1);
        REQUIRE(ptr.unique());
      }
    }
  }
}

SCENARIO("unsafe pointers with contents can be reset", "[pointer unit]") {
  GIVEN("an unsafe pointer with some contents") {
    auto ptr = dart::make_unsafe<dart::packet>();

    WHEN("the pointer is reset") {
      ptr.reset();
      THEN("its properties reset") {
        REQUIRE(ptr.get() == nullptr);
        REQUIRE(ptr.use_count() == 0);
        REQUIRE_FALSE(static_cast<bool>(ptr));
      }
    }
  }
}

SCENARIO("skinny pointers with contents can be reset", "[pointer unit]") {
  GIVEN("a skinny pointer with some contents") {
    auto ptr = dart::make_skinny<dart::packet>();

    WHEN("the pointer is reset") {
      ptr.reset();
      THEN("its properties reset") {
        REQUIRE(ptr.get() == nullptr);
        REQUIRE(ptr.use_count() == 0);
        REQUIRE_FALSE(static_cast<bool>(ptr));
      }
    }
  }
}

SCENARIO("unsafe array pointers with contents can be reset", "[pointer unit]") {
  GIVEN("an unsafe pointer with some contents") {
    auto ptr = dart::make_unsafe<dart::packet[]>(1);

    WHEN("the pointer is reset") {
      ptr.reset();
      THEN("its properties reset") {
        REQUIRE(ptr.get() == nullptr);
        REQUIRE(ptr.use_count() == 0);
        REQUIRE_FALSE(static_cast<bool>(ptr));
      }
    }
  }
}

SCENARIO("skinny array pointers with contents can be reset", "[pointer unit]") {
  GIVEN("a skinny pointer with some contents") {
    auto ptr = dart::make_skinny<dart::packet[]>(1);

    WHEN("the pointer is reset") {
      ptr.reset();
      THEN("its properties reset") {
        REQUIRE(ptr.get() == nullptr);
        REQUIRE(ptr.use_count() == 0);
        REQUIRE_FALSE(static_cast<bool>(ptr));
      }
    }
  }
}

SCENARIO("unsafe pointers can be re-seated with new contents", "[pointer unit]") {
  GIVEN("an unsafe pointer with some contents") {
    auto ptr = dart::make_unsafe<dart::packet>();

    WHEN("that pointer is given new contents") {
      ptr.reset(new dart::packet(dart::packet::object("yes", "no")));
      THEN("the pointer behaves as if it had always owned those contents") {
        REQUIRE(ptr.get() != nullptr);
        REQUIRE(ptr->is_object());
        REQUIRE(static_cast<bool>(ptr));
        REQUIRE(ptr->get("yes") == "no");
        REQUIRE(ptr.use_count() == 1);
        REQUIRE(ptr.unique());
      }
    }
  }
}

SCENARIO("skinny pointers can be re-seated with new contents", "[pointer unit]") {
  GIVEN("a skinny pointer with some contents") {
    auto ptr = dart::make_skinny<dart::packet>();

    WHEN("that pointer is given new contents") {
      ptr.reset(new dart::packet(dart::packet::object("yes", "no")));
      THEN("the pointer behaves as if it had always owned those contents") {
        REQUIRE(ptr.get() != nullptr);
        REQUIRE(ptr->is_object());
        REQUIRE(static_cast<bool>(ptr));
        REQUIRE(ptr->get("yes") == "no");
        REQUIRE(ptr.use_count() == 1);
        REQUIRE(ptr.unique());
      }
    }
  }
}

SCENARIO("unsafe pointers can share contents across multiple instance", "[pointer unit]") {
  GIVEN("an unsafe pointer with some contents") {
    auto ptr = dart::make_unsafe<dart::packet>();

    WHEN("the pointer is copied") {
      auto copy = ptr;
      THEN("ownership is shared between the instances") {
        REQUIRE(copy.get() == ptr.get());
        REQUIRE(copy.use_count() == ptr.use_count());
        REQUIRE(copy.use_count() == 2);
        REQUIRE(copy.unique() == ptr.unique());
        REQUIRE_FALSE(copy.unique());
      }

      WHEN("the original pointer is reset") {
        ptr.reset();
        THEN("it relinquishes shared ownership") {
          REQUIRE_FALSE(copy.get() == ptr.get());
          REQUIRE_FALSE(copy.use_count() == ptr.use_count());
          REQUIRE(copy.use_count() == 1);
          REQUIRE(copy.unique());
        }
      }
    }
  }
}

SCENARIO("skinny pointers can share contents across multiple instance", "[pointer unit]") {
  GIVEN("a skinny pointer with some contents") {
    auto ptr = dart::make_skinny<dart::packet>();

    WHEN("the pointer is copied") {
      auto copy = ptr;
      THEN("ownership is shared between the instances") {
        REQUIRE(copy.get() == ptr.get());
        REQUIRE(copy.use_count() == ptr.use_count());
        REQUIRE(copy.use_count() == 2);
        REQUIRE(copy.unique() == ptr.unique());
        REQUIRE_FALSE(copy.unique());
      }

      WHEN("the original pointer is reset") {
        ptr.reset();
        THEN("it relinquishes shared ownership") {
          REQUIRE_FALSE(copy.get() == ptr.get());
          REQUIRE_FALSE(copy.use_count() == ptr.use_count());
          REQUIRE(copy.use_count() == 1);
          REQUIRE(copy.unique());
        }
      }
    }
  }
}

SCENARIO("unsafe array pointers can share contents across multiple instance", "[pointer unit]") {
  GIVEN("an unsafe pointer with some contents") {
    auto ptr = dart::make_unsafe<dart::packet[]>(1);

    WHEN("the pointer is copied") {
      auto copy = ptr;
      THEN("ownership is shared between the instances") {
        REQUIRE(copy.get() == ptr.get());
        REQUIRE(copy.use_count() == ptr.use_count());
        REQUIRE(copy.use_count() == 2);
        REQUIRE(copy.unique() == ptr.unique());
        REQUIRE_FALSE(copy.unique());
      }

      WHEN("the original pointer is reset") {
        ptr.reset();
        THEN("it relinquishes shared ownership") {
          REQUIRE_FALSE(copy.get() == ptr.get());
          REQUIRE_FALSE(copy.use_count() == ptr.use_count());
          REQUIRE(copy.use_count() == 1);
          REQUIRE(copy.unique());
        }
      }
    }
  }
}

SCENARIO("skinny array pointers can share contents across multiple instance", "[pointer unit]") {
  GIVEN("a skinny pointer with some contents") {
    auto ptr = dart::make_skinny<dart::packet[]>(1);

    WHEN("the pointer is copied") {
      auto copy = ptr;
      THEN("ownership is shared between the instances") {
        REQUIRE(copy.get() == ptr.get());
        REQUIRE(copy.use_count() == ptr.use_count());
        REQUIRE(copy.use_count() == 2);
        REQUIRE(copy.unique() == ptr.unique());
        REQUIRE_FALSE(copy.unique());
      }

      WHEN("the original pointer is reset") {
        ptr.reset();
        THEN("it relinquishes shared ownership") {
          REQUIRE_FALSE(copy.get() == ptr.get());
          REQUIRE_FALSE(copy.use_count() == ptr.use_count());
          REQUIRE(copy.use_count() == 1);
          REQUIRE(copy.unique());
        }
      }
    }
  }
}

SCENARIO("unsafe pointers can move their contents", "[pointer unit]") {
  GIVEN("an unsafe pointer with some contents") {
    auto ptr = dart::make_unsafe<dart::packet>();

    WHEN("the pointer is moved") {
      auto moved = std::move(ptr);
      THEN("the new pointer takes ownership of the contents") {
        REQUIRE(moved.unique());
        REQUIRE(ptr.use_count() == 0);
        REQUIRE(ptr.get() == nullptr);
        REQUIRE_FALSE(moved.get() == nullptr);
        REQUIRE(static_cast<bool>(moved));
        REQUIRE_FALSE(static_cast<bool>(ptr));
      }
    }
  }
}

SCENARIO("skinny pointers can move their contents", "[pointer unit]") {
  GIVEN("a skinny pointer with some contents") {
    auto ptr = dart::make_skinny<dart::packet>();

    WHEN("the pointer is moved") {
      auto moved = std::move(ptr);
      THEN("the new pointer takes ownership of the contents") {
        REQUIRE(moved.unique());
        REQUIRE(ptr.use_count() == 0);
        REQUIRE(ptr.get() == nullptr);
        REQUIRE_FALSE(moved.get() == nullptr);
        REQUIRE(static_cast<bool>(moved));
        REQUIRE_FALSE(static_cast<bool>(ptr));
      }
    }
  }
}

SCENARIO("unsafe array pointers can move their contents", "[pointer unit]") {
  GIVEN("an unsafe pointer with some contents") {
    auto ptr = dart::make_unsafe<dart::packet[]>(1);

    WHEN("the pointer is moved") {
      auto moved = std::move(ptr);
      THEN("the new pointer takes ownership of the contents") {
        REQUIRE(moved.unique());
        REQUIRE(ptr.use_count() == 0);
        REQUIRE(ptr.get() == nullptr);
        REQUIRE_FALSE(moved.get() == nullptr);
        REQUIRE(static_cast<bool>(moved));
        REQUIRE_FALSE(static_cast<bool>(ptr));
      }
    }
  }
}

SCENARIO("skinny array pointers can move their contents", "[pointer unit]") {
  GIVEN("a skinny pointer with some contents") {
    auto ptr = dart::make_skinny<dart::packet[]>(1);

    WHEN("the pointer is moved") {
      auto moved = std::move(ptr);
      THEN("the new pointer takes ownership of the contents") {
        REQUIRE(moved.unique());
        REQUIRE(ptr.use_count() == 0);
        REQUIRE(ptr.get() == nullptr);
        REQUIRE_FALSE(moved.get() == nullptr);
        REQUIRE(static_cast<bool>(moved));
        REQUIRE_FALSE(static_cast<bool>(ptr));
      }
    }
  }
}

SCENARIO("unsafe pointers support const-correct promotions", "[pointer unit]") {
  GIVEN("an unsafe pointer with some contents") {
    auto ptr = dart::make_unsafe<dart::packet>();

    WHEN("the pointer is copied into a pointer-to-const") {
      dart::unsafe_ptr<dart::packet const> copy = ptr;
      THEN("the conversion works") {
        REQUIRE_FALSE(ptr.get() == nullptr);
        REQUIRE_FALSE(copy.get() == nullptr);
        REQUIRE(ptr.use_count() == 2);
        REQUIRE(copy.use_count() == 2);
        REQUIRE(static_cast<bool>(ptr));
        REQUIRE(static_cast<bool>(copy));
      }
    }

    WHEN("the pointer is moved into a pointer-to-const") {
      dart::unsafe_ptr<dart::packet const> moved = std::move(ptr);
      THEN("the conversion works") {
        REQUIRE(ptr.get() == nullptr);
        REQUIRE_FALSE(moved.get() == nullptr);
        REQUIRE(ptr.use_count() == 0);
        REQUIRE(moved.use_count() == 1);
        REQUIRE(moved.unique());
        REQUIRE_FALSE(static_cast<bool>(ptr));
        REQUIRE(static_cast<bool>(moved));
      }
    }
  }
}

SCENARIO("skinny pointers support const-correct promotions", "[pointer unit]") {
  GIVEN("a skinny pointer with some contents") {
    auto ptr = dart::make_skinny<dart::packet>();

    WHEN("the pointer is copied into a pointer-to-const") {
      dart::skinny_ptr<dart::packet const> copy = ptr;
      THEN("the conversion works") {
        REQUIRE_FALSE(ptr.get() == nullptr);
        REQUIRE_FALSE(copy.get() == nullptr);
        REQUIRE(ptr.use_count() == 2);
        REQUIRE(copy.use_count() == 2);
        REQUIRE(static_cast<bool>(ptr));
        REQUIRE(static_cast<bool>(copy));
      }
    }

    WHEN("the pointer is moved into a pointer-to-const") {
      dart::skinny_ptr<dart::packet const> moved = std::move(ptr);
      THEN("the conversion works") {
        REQUIRE(ptr.get() == nullptr);
        REQUIRE_FALSE(moved.get() == nullptr);
        REQUIRE(ptr.use_count() == 0);
        REQUIRE(moved.use_count() == 1);
        REQUIRE(moved.unique());
        REQUIRE_FALSE(static_cast<bool>(ptr));
        REQUIRE(static_cast<bool>(moved));
      }
    }
  }
}

SCENARIO("unsafe array pointers can use subscript indexing", "[pointer unit]") {
  GIVEN("an unsafe array pointer with some contents") {
    auto ptr = dart::make_unsafe<dart::packet[]>(4);

    WHEN("reading from indices") {
      THEN("they work as expected") {
        REQUIRE(ptr[0].get_type() == dart::packet::type::null);
        REQUIRE(ptr[1].get_type() == dart::packet::type::null);
        REQUIRE(ptr[2].get_type() == dart::packet::type::null);
        REQUIRE(ptr[3].get_type() == dart::packet::type::null);
      }
    }

    WHEN("writing to indices") {
      ptr[0] = dart::packet::object("hello", "world");
      THEN("it works as expected") {
        REQUIRE(ptr[0].get_type() == dart::packet::type::object);
        REQUIRE(ptr[0]["hello"] == "world");
      }
    }
  }
}

SCENARIO("skinny array pointers can use subscript indexing", "[pointer unit]") {
  GIVEN("an skinny array pointer with some contents") {
    auto ptr = dart::make_skinny<dart::packet[]>(4);

    WHEN("reading from indices") {
      THEN("they work as expected") {
        REQUIRE(ptr[0].get_type() == dart::packet::type::null);
        REQUIRE(ptr[1].get_type() == dart::packet::type::null);
        REQUIRE(ptr[2].get_type() == dart::packet::type::null);
        REQUIRE(ptr[3].get_type() == dart::packet::type::null);
      }
    }

    WHEN("writing to indices") {
      ptr[0] = dart::packet::object("hello", "world");
      THEN("it works as expected") {
        REQUIRE(ptr[0].get_type() == dart::packet::type::object);
        REQUIRE(ptr[0]["hello"] == "world");
      }
    }
  }
}
