![Dart Logo](benchmark/logo.png)
==============
[![Build Status](https://travis-ci.com/target/libdart.svg?branch=master)](https://travis-ci.com/target/libdart)
[![Build status](https://ci.appveyor.com/api/projects/status/fji5sgka5toa7ieq/branch/master?svg=true)](https://ci.appveyor.com/project/Cfretz244/libdart-lud7s/branch/master)
[![Coverage Status](https://coveralls.io/repos/github/target/libdart/badge.svg?branch=master)](https://coveralls.io/github/target/libdart?branch=master)

## A High Performance, Easy to Use, Network Optimized, JSON Library
**Dart** is both a wire-level binary **JSON** protocol, along with a high performance,
and surprisingly high level, **C++** API to interact with that **JSON**.
It is primarily optimized for efficiency of receiver interaction and on-the-wire
representation size, however, it also allows for dynamic modification when necessary.

**Dart** can be used in any application as a dead-simple, fast, and lightweight
**JSON** parser, but it first and foremost targets real-time stream processing engines
in a schema-less environment.

It offers logarithmic complexity of object key-lookup, guarantees stability of object
iteration, scales [extremely well](PERFORMANCE.md#lookup-finalized-random-fields-1)
as packet sizes increase, requires **zero** receiver-side memory allocations/parsing/unpacking
for read-only interactions, and exposes two interfaces, a header-only **C++14** interface for
typical use, and an **ABI** stable **C89** interface for binding against.

Although not a **JSON** parser itself, **Dart** leverages the fastest general purpose
**JSON** parsers available ([source](https://github.com/miloyip/nativejson-benchmark)),
[RapidJSON](https://github.com/Tencent/rapidjson)
and [sajson](https://github.com/chadaustin/sajson), for format conversion both into,
and out of, **JSON**.

Finally, as **Dart** can also be useful when working with config files, it also
supports parsing **YAML** via [libyaml](https://github.com/yaml/libyaml.git).

## Quick Start
This readme covers a wide variety of information for the library, but for the impatient among us,
here are some at-a-glance examples. For examples of how to use the **C** binding layer,
see our [bindings](BINDINGS.md) document.

**Dart** makes parsing a **JSON** string dead-simple, and crazy [fast](PARSING.md):
```c++
#include <dart.h>
#include <iostream>

int main() {
  // Fancy string literal is a raw literal.
  auto json = dart::parse(R"({"msg":"hello from dart!"})");
  std::cout << json["msg"].to_json() << std::endl;
}

// => "hello from dart!"
```

**Dart** automatically understands most built-in and `std` types
(and can be extended to work with any type), making it extremely
easy and natural to build **JSON**.
```c++
#include <dart.h>
#include <iostream>

int main() {
  // A simple example with some built-in types.
  dart::array arr {1, "two", 3.14159, true, nullptr};
  std::cout << arr << std::endl;

  // A more complex example with some complex STL types.
  using value = std::variant<double, std::string>;
  using sequence = std::vector<value>;
  using map = std::map<std::string, sequence>;

  // Dart recursively decomposes the type and figures it out.
  arr.push_back(map {{"args", {3.14159, 2.99792, "top", "secret"}}});
  std::cout << arr << std::endl;
}

// => [1,"two",3.14159,true,null]
// => [1,"two",3.14159,true,null,{"args":[3.14159,2.99792,"top","secret"]}]
```

The **Dart** container types (`dart::object` and `dart::array`) model the API
of the `std` equivalents (`std::map` and `std::vector` respectively), allowing
for extremely expressive, idiomatic, and type-safe interaction with a dynamically
typed notation language from a statically typed programming language:
```c++
#include <dart.h>
#include <iostream>
#include <algorithm>

int main() {
  // Build the same array as in the last example, but incrementally.
  dart::array arr;
  arr.push_back("two").push_front(1);
  arr.insert(2, 3.14159);
  arr.resize(5, true);
  arr.set(4, nullptr);
  std::cout << arr << std::endl;

  // Search for a particular element and erase the rest.
  auto it = std::find(std::begin(arr), std::end(arr), 3.14159);
  while (it != std::end(arr)) {
    it = arr.erase(it);
  }
  std::cout << arr << std::endl;
}

// => [1,"two",3.14159,true,null]
// => [1, "two"]
```

**Dart** also makes it extremely easy to efficiently share data across machines/processes,
without the receiver needing to reparse the data.
```c++
#include <dart.h>

// Function sends data over the network,
// through shared memory,
// into a file,
// wherever.
size_t send_data(void const* bytes, size_t len);

int main() {
  // Suppose we have some very important data
  dart::object data {"albums", dart::array {"dark side", "meddle", "animals"}};

  // It can be finalized into a contiguous, architecture-independent, representation.
  data.finalize();

  // We can then pass the raw bytes along.
  // On the receiver end, a new packet instance can be constructed from this buffer
  // without any additional work or parsing.
  auto bytes = data.get_bytes();
  auto sent = send_data(bytes.data(), bytes.size());
  assert(sent == bytes.size());
}
```

## Compilation and Installation
**Dart** is implemented using modern **C++**, and requires both Microsoft's Guidelines
Support Library [GSL](https://github.com/Microsoft/GSL), and a **C++14** enabled toolchain
(`clang` >= **3.8**, `gcc` >= **5.0**, apple's `clang` >= **8.3**, `MSVC` ~> **19.22**).
Support for **C++11**  may be added in the future, but is not currently being pursued.

**Dart** utilizes `cmake` for its build process and currently primarily targets
Linux/macOS, but has experimental (and improving) support for Windows.
On Linux/macOS, it can be built in the following way:
```bash
# Clone it down.
git clone https://github.com/target/libdart.git
cd libdart/

# Create the cmake build directory and prepare a build
# with tests enabled.
mkdir build
cd build

# Build, test, and install (assuming a 4 core machine).
# Running tests isn't mandatory, but is highly recommended.
# Dart is primarily a header-only library, but also includes
# an ABI-stable pure C binding layer which can be built and
# installed with -Dbuild_abi=ON
cmake .. # -Dbuild_abi=ON
make -j 4
ctest
make install

# Generate documentation (if desired).
# Doxygen must have already been installed.
# Generates documentation inside the directory "docs"
cd ..
doxygen
```
For instructions on building for windows, see our [windows](WINDOWS.md) build instructions.

**Dart** can optionally leverage [RapidJSON](https://github.com/Tencent/rapidjson),
[sajson](https://github.com/chadaustin/sajson), 
and [libyaml](https://github.com/yaml/libyaml.git), and will attempt to detect installations
automatically while building, but can be independently specified with `-DDART_HAS_RAPIDJSON`,
`-DDART_USE_SAJSON`, and `-DDART_HAS_YAML` preprocessor flags.


## Performance
**TL;DR**: **Dart**'s performance is excellent, but to see detailed breakdowns for different
workflows, see our [performance](PERFORMANCE.md) document.
For those interested in where this performance comes from, see our
[implementation](IMPLEMENTATION.md) document.

**JSON** parsing performance is a big enough topic to be given its own document, which
can be found here: [parsing performance](PARSING.md).

## Basic Usage
Overly detailed usage examples can be obtained from the `test/` directory, or by building the
included documentation, but some examples of basic usage are included below. For examples of
how to use the **C** binding layer, see our [bindings](BINDINGS.md) document.
Parsing a **JSON** string with **Dart**:
```c++
// Get some JSON from somewhere.
std::string json = input.read();

// Get a packet.
dart::packet pkt = dart::packet::from_json(json);

// Do stuff with it.
dart::packet header = pkt["header"];
std::cout << header["User-Agent"] << std::endl;
std::cout << header["Content-Type"] << std::endl;

// When machine types are needed.
auto resp = pkt["response"];
switch (resp["code"].integer()) {
  case 200:
    {
      std::string_view body {resp["body"]};
      double elapsed = resp["elapsed_millis"].decimal();
      process_success(body, elapsed);
      break;
    }
  case 300:
    for (auto res : resp["resources"]) {
      resource_moved(res["path"].strv(), res["mods"][0]);
    }
    break;
  case 400:
    {
      char const* err = resp["error"].str();
      std::string_view warn = resp["warning"].strv();
      process_failure(err, warn);
      break;
    }
  default:
    printf("Encountered internal error %s\n", resp["error"].str());
}
```

Build a **JSON** object from scratch:
```c++
// Create a base object.
// dart::packet::make_object can take arbitrarily many pairs of arguments.
dart::packet obj = dart::packet::make_object("hello", "goodbye", "answer", 42);

// Put some more stuff in it.
obj.add_field("c", 2.99792);
obj.add_field("none", nullptr);
obj.add_field("truth", true);
obj.add_field("lies", false);
obj.add_field("fib", dart::packet::make_array(1, 1, 2, 3, 5, 8, 13));

// Send it somewhere.
do_something(obj.to_json());
```

Preparing a **JSON** object to be sent over the network:
```c++
// Assuming the packet from the previous example:
auto buffer = obj.finalize().get_bytes();

// Write the buffer to some output:
socket.write(buffer.data(), buffer.size());
```

## Conversion API
As you've seen by now, **Dart** ships with a large number of type conversions predefined,
allowing for expressive, and safe, interaction with a dynamically typed notation language
from a statically typed programming language.
Out-of-the-box, **Dart** can work with any machine type, and with most STL containers:
```c++
// Get a packet to test with.
auto obj = dart::packet::make_object();

// Machine types.
obj.add_field("bool", true);
obj.add_field("int", 5);
obj.add_field("uint", 5U);
obj.add_field("long", 5L);
obj.add_field("ulong", 5UL);
obj.add_field("long long", 5LL);
obj.add_field("ulong long", 5ULL);
obj.add_field("float", 5.0f);
obj.add_field("double", 5.0);
obj.add_field("str", "asdf");
obj.add_field("null", nullptr);

// STL types
obj.add_field("arr", std::vector {1, 1, 2, 3, 5, 8});
obj.add_field("maybe", std::optional {"present"});
obj.add_field("oneof", std::variant<int, double> {5});
obj.add_field("many", std::tuple {3.14159, 2.99792})
```

However, in addition to the pre-defined conversions, **Dart** exposes an API to allow for
user defined conversions like so:
```c++
#include <dart.h>
#include <string>
#include <iostream>

// A simple custom string class that Dart doesn't know about.
struct my_string {
  std::string str;
};

// We define an explicit specialization of the struct dart::convert::to_dart,
// which Dart will call to perform our conversion
namespace dart::convert {
  template <>
  struct to_dart<my_string> {
    template <class Packet>
    static Packet cast(my_string const& s) {
      return Packet::make_string(s.str);
    }
  };
}

// Example usage
int main() {
  // If dart::convert::to_dart had not been specialized, this line would fail to
  // compile with an error about an undefined overload of dart::packet::push_back.
  auto arr = dart::packet::make_array(my_string {"one"}, my_string {"two"});

  // The conversions are applied recursively, so we can also use STL containers
  // of our custom type.
  arr.push_back(std::optional<my_string> {});
  arr.push_back(std::vector {my_string {"three"}, my_string {"four"}});

  // Will print:
  // ["one", "two", null, ["three", "four"]]
  std::cout << arr << std::endl;
}
```
For those who are meta-programming inclined, you can test if a **Dart** conversion is
defined by using the type trait `dart::convert::is_castable`.

## Mutability and Copy-on-Write
By default, to keep memory in scope safely, **Dart** uses thread safe reference counting
based on `std::shared_ptr`. This behavior is a configuration point for the library which
will be covered later.

To allow easy embedding in threaded applications, **Dart** follows a copy-on-write 
data model in non-finalized mode, allowing for frictionless mutation of shared data.
This "just works" for the most part without the user being very involved, but it can have
some surprising implications:

Example of it "just working":
```c++
// Create a base object.
auto orig = dart::packet::make_object("hello", "world");
assert(orig.refcount() == 1U);

// Copy it.
// At this point, copy and orig share the same underlying representation, only an atomic
// increment was performed.
auto copy = orig;
assert(orig.refcount() == 2U);
assert(copy.refcount() == orig.refcount());

// Mutate the copy.
// At this point, copy performs a shallow copy-out, copying only the immediate
// level of an arbitrarily deep tree maintained by orig.
copy.add_field("hello", "life");
assert(orig.refcount() == 1U);
assert(copy.refcount() == 1U);
assert(copy != orig);

// Will output:
// {"hello":"world"}
// {"hello":"life"}
std::cout << orig << std::endl;
std::cout << copy << std::endl;
```

Example of a surprising implication:
```c++
// Get an object with a nested array.
auto arr = dart::packet::make_array("surprise");
auto obj = dart::packet::make_object("nested", std::move(arr));

// Attempt to remove something from the array.
// Subscript operator returns a temporary packet instance, which is then mutated, copying out
// in the processes, and is then destroyed at the semicolon, taking its modifications with it.
// If you're on a supported compiler, this line will yield a warning about a discarded
// result.
obj["nested"].pop_back();

// The original object was not modified, and so the following will abort.
assert(obj["nested"].empty()); // <-- BOOM

// What should have been done is the following:
auto nested = obj["nested"].pop_back();
obj.add_field("nested", std::move(nested));

// This behavior also means that assigning to the subscript operator will not insert
// as the user might expect, instead assigning to a temporary null value.
// To avoid confusion on this point, the case is explicitly called out by the library.
obj["oops"] = dart::packet::make_string("ouch"); // <-- WILL NOT COMPILE
```

## API Lifecycle
The type `dart::packet` is the primary class of the library, is likely to be the type most
commonly interacted with, and can be in two distinct states: "finalized" and non-"finalized".

Finalized packets are represented internally as a contiguous buffer of bytes, are immutable,
and are immediately ready to be sent over a network connection. In exchange for mutability,
finalized packets come with **_significant_** performance improvements, with object key-lookups
in particular seeing a 200%-300% performance increase.

```c++
// Get an object, starts out mutable.
auto data = dart::packet::make_object("status", 200, "err", nullptr);
assert(!data.is_finalized());
data.add_field("message", "OK");

// Transition it into the finalized state.
// Finalizing a packet requires making a single allocation, walking across the object tree,
// and then freeing the original tree.
data.finalize();
assert(data.is_finalized());

// Key lookups are now much faster, but we can no longer mutate the packet.
// The following line would throw:
data.add_field("can't", "do it"); // <-- BOOM

// We can also now get access to a contiguous buffer of bytes representing the packet,
// ready to be written out to a file/over the network/etc.
gsl::span<gsl::byte const> bytes = data.get_bytes();
file.write(bytes.data(), bytes.size());

// We can transition back to being a mutable packet, ableit expensively, by calling:
data.lift();
assert(!data.is_finalized());
data.add_field("can", "do it");
```

## Explicit API Lifecycle
The fact that `dart::packet` can interoperably juggle data across two very different
internal representations is convenient for many use-cases, but this necessarily comes
with an inability to reason at compile-time as to whether a particular packet can be
mutated. For this reason, **Dart** exposes two other types that explicitly document
this division at compile-time: `dart::buffer` and `dart::heap`.

`dart::packet` is internally implemented in terms of `dart::buffer` and `dart::heap`,
and is implicitly covertible with either, which allows for expressive, safe, and
performant usage across a wide variety of domains.

```c++
// Function takes a dart object that is definitely mutable
void a_high_level_function(dart::heap obj);

// Function takes a dart object that is definitely immutable and can be
// used as a buffer of bytes.
void a_lower_level_network_function(dart::buffer buf);

// Get a packet using the mutable api.
auto pkt = dart::heap::make_object("hello", "world");

// Do stuff with it.
pkt.add_field("missing", nullptr);
a_high_level_function(pkt);

// Say that we now know that we want an immutable representation, we can
// describe that in code using dart::buffer.
// dart::buffer exposes a smaller subset of the API presented by dart::packet,
// but it still behaves like any other dart object.
dart::buffer finalized = pkt.finalize();
assert(finalized.size() == 1U);
assert(finalized["hello"] == "world");
a_lower_level_network_function(finalized);

// We can also convert back and forth as we choose.
// In general:
// *  dart::heap|dart::buffer -> dart::packet is essentially free and does not
//    require a cast.
// *  dart::heap -> dart::buffer requires a single allocation and walking across
//    the object tree, and is still relatively cheap, but requires a cast.
// *  dart::buffer -> dart::heap requires allocations for as at least as many nodes
//    in the tree and is very expensive; it also requires a cast.
dart::packet copy = pkt;
copy = finalized;
dart::heap unfinalized {finalized};
dart::buffer refinalized {unfinalized};
```

## Reference Counting, When You Want It
Thread-safe reference counting is ideal for many applications. It strikes a nice
balance between performance (predictable memory usage, no GC-pauses, etc) and safety,
while also making it trivially easy to share data across threads. Nothing is for free,
however, and there will always be use-cases that demand every last ounce of available
performance; **C++** has never been a "one size fits all" kind of language.

As stated previously, **Dart** has a dedicated reference counter API that allows the
user to generically customize every facet of how **Dart** implements reference counting
(covered in-depth in the [advanced](ADVANCED.md) section), however, sometimes it's even
useful to be able to selectively _disable_ reference counting for just a particular section
of code; the **Dart** `view` API allows for this.

All of the types mentioned previously (`dart::heap`, `dart::buffer`, and `dart::packet`)
contain the nested type `view`, which is respectively interoperable, and exports a read-only
subset of the associated API.

As a motivating example:
```c++
#include <dart.h>
#include <stdlib.h>

// A short function that samples a couple values
int64_t work_hard(dart::packet points, int samples = 10) {
  // Sample some packets "randomly"
  int64_t total = 0;
  auto len = points.size();
  for (int i = 0; i < samples; ++i) {
    total += points[rand() % len].numeric();
  }
  return total;
}

// A computationally heavy function that frequently accesses keys
int64_t work_really_hard(dart::packet::view points) {
  int64_t total = 0;
  auto end = std::end(points);
  auto start = std::begin(points);
  while (start != end) {
    for (auto curr = start; curr != end; ++curr) {
      total += curr->numeric_or(0);
    }
    ++start;
  }
  return total;
}

// Decides how important the data is and dispatches accordingly
int64_t process_data(dart::packet data) {
  auto points = data["points"];
  if (data["really_important"]) {
    return work_really_hard(points);
  } else {
    return work_hard(points);
  }
}
```
As you can see, we've added a new type, `dart::packet::view`, which is implicitly
constructible from any instance of `dart::packet`.
On the surface, `view` types appear to behave like any other **Dart** type, however,
internally they do _**not**_ participate in reference counting, which, in the
right circumstances, can be a huge performance win.

Because we're using a `view` type, `work_really_hard` has become a pure function,
spinning over its input without mutation and accumulating into a local (probably register backed)
before returning; `work_hard`, by contrast, does a lot of unnecessary refcount manipulation
for little clear benefit.

Judicious use of `view` types can result in significant performance wins, however, as they
do _**not**_ participate in reference counting, they can also dangle and corrupt memory very
easily if not handled appropriately (`work_really_hard` caching `points` in a static/global
variable would be trouble).
A fairly safe pattern with `view` types is to exclusively pass them down the stack (returning
a `view` requires thought) and think _very_ carefully before putting them in containers.

Finally, a `view` type can be transitioned back into its fully-fledge counterpart at any point
in time by calling `dart::packet::as_owner` like so:
```c++
#include <dart.h>
#include <unordered_map>

std::unordered_map<std::string, dart::packet> cache;

dart::packet update_cache(dart::packet::view data) {
  bool dummy;
  auto found = cache.find(data["name"].str());
  if (found == cache.end()) {
    std::tie(found, dummy) = cache.emplace(data["name"].str(), data["values"].as_owner());
  } else {
    found->second = data["values"].as_owner();
  }
  return found->second;
}
```

## Type-Safe API
**Dart** is first and foremost a dynamically typed library, for interacting with a
dynamically typed notation language, but as **C++** is a statically typed language, it
can be useful to statically know the type of a variable at compile-time.

To enable this use-case, **Dart** exposes a secondary interface, fully interoperable
with the first, that enables static type enforcement.
```c++
// Definitely an object, might be mutable.
// dart::object is an alias for dart::packet::object.
dart::object dynamic {"pi", 3.14159, "c", 2.99792};
dart::packet::object copyobj = dynamic;

// Definitely a mutable object.
dart::heap::object mut {"pi", 3.14159, "c", 2.99792};

// Definitely an immutable object.
dart::buffer::object immut {"pi", 3.14159, "c", 2.99792};

// Definitely an array, might be mutable.
// dart::array is an alias for dart::packet::array
dart::array dynarr {1, "fish", 2, "fish"};
dart::packet::array copyarr = dynarr;

// Definitely a mutable array.
dart::heap::array mutarr {"red", "fish", "blue", "fish"};

assert(dynamic == mut);
assert(mut == immut);
assert(immut == dynamic);
assert(dynamic != dynarr);
assert(mut != mutarr);
```

A variety of conversions between the APIs are defined, but generally speaking, if
the conversion can be performed safely and inexpensively, without throwing an
exception or allocating memory, it's allowed implicitly, otherwise it requires a cast.
```c++
using namespace dart::literals;

// Examples of some implicit conversions.
dart::buffer::object fin {"rick", "sanchez", "morty", "smith"};
dart::buffer untyped = fin;
dart::packet::object dynamic = fin;
dart::packet untyped_dynamic = dynamic;

// Mixing the type-safe/non API.
dart::packet str = "world"_dart;
dart::packet::object obj {"hello", str};
obj.insert(dart::heap::string {"yes"}, dart::heap::make_string("no"));
```
It is worth noting that while this API is included as a convenience, to allow **Dart** to
naturally express data across a variety of different domains and development workflows,
it does _not_ come with any performance improvement.

The type-safe API is implemented on top of the API described up to this point, should exhibit
identical performance characteristics, and exists solely for the purposes of
code-organization/developer quality of life improvements.

A consequence of all this conversion logic, and becoming especially compelling with
class template argument deduction in **C++17**, is that code like this is actually valid:
```c++
#include <map>
#include <dart.h>
#include <vector>
#include <variant>
#include <iostream>

int main() {
  std::cout << dart::packet::object {
    "a key", "a value",
    "a homogenous array", std::vector {1, 1, 2, 3, 5, 8, 13},
    "a heterogenous array", std::tuple {false, 0.0, "hello"},
    "an object", dart::packet::object {
      "oneof", std::variant<int, float, bool> {false},
      "maybe_null", std::optional {"present"}
    }
  } << std::endl;
}
```
Which outputs:
```
{"a heterogenous array":[false,0.0,"hello"],"a homogenous array":[1,1,2,3,5,8,13],"a key":"a value","an object":{"maybe_null":"present","oneof":false}}
```
Deciding as to whether it's actually sensible/ethical/_practically useful_ to do so
is left as an exercise to the reader.

## Customization Points/Expert Usage
For a guide on the customization points the library exposes, see our
[Advanced Usage](ADVANCED.md) document.
