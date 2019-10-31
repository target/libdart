## C Bindings Layer
**C++** is great, and header-only libraries are super convenient as
zero-configuration, drag-and-drop, dependencies, but both present significant
challenges for any use case that requires long-term **ABI** stability (_any_
modification can present as an **ABI** break for header-only libraries),
or cross-language **FFI** bindings (symbol mangling is a never-ending nightmare).

To support these, and many other, use cases, **Dart** provides a secondary interface
(available in the `<dart/abi.h>` header) that any **C89** compliant
toolchain/**FFI** library can bind against without issue. Additionally, the **ABI**
header is self contained and can be shipped around without the rest of the
surrounding project.

It is worth noting that the **Dart** **C** layer is _not_ a re-implementation of
the library, and simply wraps the **C++** layer such that implementation details
beyond the public **C** interface are not visible to clients. It does _not_
provide a performance improvement over the **C++** interface.

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
# After install, the Dart ABI can be linked against using
# -ldart_abi, or -ldart_abi_static if static linking is
# desired.
cmake .. -Dbuild_abi=ON
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

While the **C++** layer can _optionally_ leverage [RapidJSON](https://github.com/Tencent/rapidjson)
for **JSON** integration, silently removing parsing/serializing from/to **JSON** from the project
if [RapidJSON](https://github.com/Tencent/rapidjson) isn't installed, the **C** layer _requires_
that [RapidJSON](https://github.com/Tencent/rapidjson) be installed and in the include path.
This is both for simplicity's sake, and to ensure that clients of the **ABI** don't have to worry
about whether their installation was compiled with **JSON** support.

## Usage
To illustrate usage for the bindings layer, this document will refer back to **C++** examples
from the primary [readme](README.md), showing how equivalent operations would be performed
using the C layer.

From the first example in the [quick start](README.md#quick-start) section:
```c
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <dart/abi.h>

int main() {
  // Parse the JSON
  // If an error occurs during parsing, a dart null instance is returned.
  dart_packet_t json = dart_from_json("{\"msg\":\"hello from dart!\"}");
  assert(dart_is_obj(&json));
  assert(dart_size(&json) == 1);

  // Grab the nested string.
  // If the key isn't present, a dart null instance is returned.
  dart_packet_t msg = dart_obj_get(&json, "msg");
  assert(dart_is_str(&msg));
  assert(strcmp(dart_str_get(&msg), "hello from dart!") == 0);

  // Stringify it and print.
  size_t len;
  char* str = dart_to_json(&msg, &len);
  assert(len == strlen("\"hello from dart!\""));
  puts(str);

  // Cleanup
  free(str);
  dart_destroy(&msg);
  dart_destroy(&json);
}

// => "hello from dart!"
```

From the second example in the [quick start](README.md#quick-start) section:
```c
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <dart/abi.h>

int main() {
  // Variadic initializers allow for syntax similar to that
  // of the C++ layer, but you have to get the format string correct...
  // If initialization fails for any reason, a dart null instance is returned.
  dart_packet_t arr = dart_arr_init_va("isdbn", 1, "two", 3.14159, true);
  assert(dart_is_arr(&arr));
  assert(dart_size(&arr) == 5);

  // Print our array, then free the string.
  char* arrjson = dart_to_json(&arr, NULL);
  puts(arrjson);
  free(arrjson);

  // Nothing like std::map/std::vector in C,
  // but we'll show how to build an equivalent packet manually.
  dart_packet_t nested = dart_obj_init_va("addss", "args", 3.14159, 2.99792, "top", "secret");
  assert(dart_is_obj(&nested));
  assert(dart_size(&nested) == 1);

  // Insert the object we just created at the end of the array.
  // Functions with "take" in their name "steal" the contents of the
  // instances they operate on.
  // nested is still a valid, queryable, object, but owns no resources.
  dart_arr_insert_take_dart(&arr, 5, &nested);
  assert(dart_is_null(&nested));

  // Print the final packet
  arrjson = dart_to_json(&arr, NULL);
  puts(arrjson);

  // Cleanup
  free(arrjson);
  dart_destroy(&arr);
}

// => [1,"two",3.14159,true,null]
// => [1,"two",3.14159,true,null,{"args":[3.14159,2.99792,"top","secret"]}]
```

From the third example in the [quick start](README.md#quick-start) section:
```c
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <dart/abi.h>

int main() {
  // Initialize an empty array.
  // Could've also used dart_arr_init, which returns its result
  // directly, but this shows the more explicit error handling options.
  // All Dart ABI functions that return a packet instance directly have an
  // _err "overload" that takes an out parameter and returns an error code.
  dart_packet_t arr;
  dart_err_t err = dart_arr_init_err(&arr);
  assert(err == DART_NO_ERROR);

  // Insert each of the necessary values.
  // All Dart ABI functions that don't return a packet instance return an
  // error code.
  err = dart_arr_insert_str(&arr, 0, "two");
  assert(err == DART_NO_ERROR);
  err = dart_arr_insert_int(&arr, 0, 1);
  assert(err == DART_NO_ERROR);
  err = dart_arr_insert_dcm(&arr, 2, 3.14159);
  assert(err == DART_NO_ERROR);

  // When calling resize, new indices are initialized to null.
  assert(dart_size(&arr) == 3);
  err = dart_arr_resize(&arr, 5);
  assert(err == DART_NO_ERROR);
  assert(dart_size(&arr) == 5);
  err = dart_arr_set_bool(&arr, 3, true);
  assert(err == DART_NO_ERROR);

  // Print our array, then free the string.
  char* arrjson = dart_to_json(&arr, NULL);
  assert(arrjson != NULL);
  puts(arrjson);
  free(arrjson);

  // Erasure during iteration isn't possible with the C API
  // So instead let's just erase the stuff we don't want.
  dart_arr_erase(&arr, 2);
  dart_arr_erase(&arr, 2);
  dart_arr_erase(&arr, 2);

  // And iterate.
  dart_packet_t val;
  dart_for_each(&arr, &val) {
    // dart_for_each manages its own resources,
    // we do NOT call dart_destroy on val.
    char* json = dart_to_json(&val, NULL);
    puts(json);
    free(json);
  }

  // Cleanup.
  dart_destroy(&arr);
}

// => [1,"two",3.14159,true,null]
// => 1
// => "two"
```

## Reference Counting Customization
The **C++** layer of **Dart** allows full customization of its reference counting
policies, the **C** layer is a bit more static and currently allows two separate
reference counter implementations.
To be specific, `DART_RC_SAFE` and `DART_RC_UNSAFE`
(which correspond to thread-safe, and thread-unsafe, reference counting respectively).

An example of a program that explicitly specifies the reference counter to use:
```c
#include <stdio.h>
#include <assert.h>
#include <dart/abi.h>

int main() {
  // All functions that construct a Dart instance have an _rc "overload"
  // that allows the user to specify the reference counter implementation to use.
  dart_packet_t json = dart_from_json_rc(DART_RC_UNSAFE, "{\"msg\":\"hello from dart!\"}");
  assert(json.rtti.rc_id == DART_RC_UNSAFE);

  // The reference counter will automatically be inherited from its parent object.
  dart_packet_t msg = dart_obj_get(&json, "msg");
  assert(msg.rtti.rc_id == DART_RC_UNSAFE);

  // Stringification works the same
  char* str = dart_to_json(&msg, NULL);
  puts(str);
  free(str);

  // Disparate reference counters are not interoperable.
  // An attempt to insert an instance with a different reference counter will fail.
  dart_packet_t safe = dart_obj_init_rc(DART_RC_SAFE);
  dart_err_t err = dart_obj_insert_dart(&json, "fails", &safe);
  assert(err == DART_TYPE_ERROR);
  puts(dart_get_error());

  // Cleanup works the same
  dart_destroy(&safe);
  dart_destroy(&msg);
  dart_destroy(&json);
}

// => "hello from dart!"
// => Unsupported packet type insertion requested
```

## Further Documentation/Examples
Extremely detailed inline documentation is available for all functions in the
**Dart ABI**, and can be generated via `doxygen`.
Examples of usage for all functions can also be found in the **ABI** tests under
the `test/` directory.
