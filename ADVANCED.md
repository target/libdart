## Customization Points/Expert Usage

A guiding design principal in C++ is "don't pay for what you don't use", and so, for
those of you who don't want/need thread-safe reference counting, or have a custom reference
counter which is totally awesome for some specific definition of awesome, **Dart** has an answer.

All of the types discussed so far are actually type aliases from templates parameterizable
by the reference counter implementation to be used, and will work out of the box with any type
that implements a specific subset of the `std::shared_ptr` API.

For convenience, **Dart** provides a couple basic implementations of alternative reference
counters, like `dart::unsafe_ptr`, which implements thread-unsafe reference counting.
```c++
#include <dart.h>

using unsafe_packet = dart::basic_packet<dart::unsafe_ptr>;

int main() {
  // A thread-unsafe object.
  unsafe_packet::object unsafe {"not", "safe"};
  unsafe.add_field("bad", "idea");

  // Our program will collapse at the slightest perturbation,
  // but copies are SO CHEAP!
  auto copy = unsafe;
  auto another = copy;
  auto just_because_we_can = another;
}
```

Generally speaking this is a bad idea as `unsafe_packet` is now a thread-hostile type,
with a thread-unsafe copy-constructor and copy-assignment operator, but for those very special
use cases, it's possible.

For those with _truly_ unique use cases, or, perhaps more likely, for those who need to
interact with legacy reference counter implementations, we can go even a step further.

Consider the following "reference counter":
```c++
#include <dart.h>

template <class T>
struct obtuse_ptr {
  obtuse_ptr() = delete;
  obtuse_ptr(obtuse_ptr const&) = delete;
  obtuse_ptr(obtuse_ptr&&) = delete;
  std::shared_ptr<T> impl;
};

using obtuse_packet = dart::basic_packet<obtuse_ptr>;

int main() {
  obtuse_packet::object asdf {"won't", "work"};
}
```

The precise error message will depend on the toolchain we're using, but if we attempt
to compile this, we'll receiving _something_ like the following:
```c
error: static_assert failed due to requirement
'refcount::has_element_type<obtuse_ptr<const byte>>::value'

"Reference counter type must either conform to the std::shared_ptr API
or specialize the necessary implementation types in the dart::refcount namespace"
```

We can fix this in two ways:
  * Implement the subset of the `std::shared_ptr` API that **Dart** utilizes
  * Leverage the trait types in the `dart::refcount` namespace to explain to **Dart**
    how to work with our custom type.

Option one:
```c++
#include <dart.h>

template <class T>
struct obtuse_ptr {

  /*----- Types -----*/

  using element_type = T;

  /*----- Lifecycle Functions -----*/

  explicit obtuse_ptr(T* owner) : impl(owner) {}
  template <class Del>
  explicit obtuse_ptr(T* owner, Del&& del) :
    impl(owner, std::forward<Del>(del))
  {}

  obtuse_ptr(obtuse_ptr const&) = default;
  obtuse_ptr(obtuse_ptr&&) = default;

  /*----- API -----*/

  T* get() const {
    return impl.get();
  }

  size_t use_count() const {
    return impl.use_count();
  }

  void reset() {
    impl.reset();
  }

  /*----- Members -----*/

  std::shared_ptr<T> impl;
};

using obtuse_packet = dart::basic_packet<obtuse_ptr>;

int main() {
  obtuse_packet::object asdf;
}
```

Option two:
```c++
#include <dart.h>

template <class T>
struct obtuse_ptr {
  obtuse_ptr() = delete;
  obtuse_ptr(obtuse_ptr const&) = delete;
  obtuse_ptr(obtuse_ptr&&) = delete;
  std::shared_ptr<T> impl;
};

namespace dart::refcount {
  
  // Provide a mapping from our custom reference counter
  // to the type it wraps.
  template <class T>
  struct element_type<obtuse_ptr<T>> {
    using type = T;
  };

  // Provide a way to create a custom reference counter
  // to wrap T, given a sequence of arguments with which
  // to construct T.
  template <class T>
  struct construct<obtuse_ptr<T>> {
    template <class... Args>
    static void perform(obtuse_ptr<T>* that, Args&&... the_args) {
      new(that) obtuse_ptr<T> {
        std::make_shared<T>(std::forward<Args>(the_args)...)
      };
    }
  };

  // Provide a way to create a custom reference counter
  // to take ownership of an existing pointer to T and
  // a deleter function object.
  template <class T>
  struct take<obtuse_ptr<T>> {
    template <class D>
    static void perform(obtuse_ptr<T>* that, std::nullptr_t, D&&) {
      new(that) obtuse_ptr<T> {
        std::shared_ptr<T>(nullptr)
      };
    }
    template <class D>
    static void perform(obtuse_ptr<T>* that, T* owned, D&& del) {
      new(that) obtuse_ptr<T> {
        std::shared_ptr<T>(owned, std::forward<D>(del))
      };
    }
  };

  // Provide a way to create a custom reference counter
  // from an existing instance, such that the shared reference
  // count is incremented.
  template <class T>
  struct copy<obtuse_ptr<T>> {
    static void perform(obtuse_ptr<T>* that, obtuse_ptr<T> const& other) {
      new(that) obtuse_ptr<T> {other.impl};
    }
  };

  // Provide a way to create a custom reference counter
  // from an existing instance, such that the reference
  // count is NOT incremented, and the original instance
  // will not decrement the reference count upon destruction.
  template <class T>
  struct move<obtuse_ptr<T>> {
    static void perform(obtuse_ptr<T>* that, obtuse_ptr<T>&& other) {
      new(that) obtuse_ptr<T> {std::move(other.impl)};
    }
  };

  // Provide a way to unwrap a custom reference counter,
  // returning the managed pointer.
  template <class T>
  struct unwrap<obtuse_ptr<T>> {
    static T* perform(obtuse_ptr<T> const& rc) {
      return rc.impl.get();
    }
  };

  // Provide a way to check the current reference count
  // of a custom reference counter.
  template <class T>
  struct use_count<obtuse_ptr<T>> {
    static size_t perform(obtuse_ptr<T> const& rc) {
      return rc.impl.use_count();
    }
  };

  // Provide a way to "reset" a custom reference counter
  // such that the reference count is immediately decremented,
  // or the managed object is destroyed.
  template <class T>
  struct reset<obtuse_ptr<T>> {
    static void perform(obtuse_ptr<T>& rc) {
      rc.impl.reset();
    }
  };

}

using obtuse_packet = dart::basic_packet<obtuse_ptr>;

int main() {
  obtuse_packet::object {"works", "fine"};
}
```

We've supplied you the ingredients and recipe, what happens with it is on you.

Finally, if you are unfortunate enough to need to work with multiple reference counter
implementations in one application (please don't), there's a helper function to allow for that:
```c++
#include <dart.h>

using unsafe_packet = dart::basic_packet<dart::unsafe_ptr>;

int main() {
  // A thread-unsafe object.
  unsafe_packet::object unsafe {"not", "safe"};
  unsafe.add_field("bad", "idea");

  // Lift up the entire tree referenced by "unsafe", and reseat it on std::shared_ptr.
  // Very expensive, and hard to type, but it is possible if it's necessary.
  auto safe_again = unsafe_packet::transmogrify<std::shared_ptr>(unsafe);
}
```
Otherwise, **Dart** types are not interoperable across reference counter implementations.
