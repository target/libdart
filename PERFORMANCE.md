## Dart Performance

As a library, **Dart** tries to walk a fine line between providing the
highest possible performance, while also enabling expressive usability
across a wide variety of domains.

As mentioned in the [readme](README.md#explicit-api-lifecycle), **Dart**
publicly exposes several different classes for interacting with its API
at various levels of abstraction and peformance: `dart::packet`,
`dart::heap`, and `dart::buffer`.

Additionally, as mentioned in the [expert usage](ADVANCED.md) document,
**Dart** exposes a variety of customization points to modify the reference
counter implementation used by the library.

## Performance Characteristics of Dart Types
`dart::packet` is the most flexible of the aforementioned classes, erasing
implementation details related to type, mutability, and layout, and
providing a uniform, and reasonably performant, API for all use-cases.
For the basic case of using **Dart** as a dead-simple `JSON`/`YAML`
parser, this is likely to be the class you want.
This flexibility is not without cost, however, and `dart::packet` should
typically be assumed to be the slowest of the types exported by the
**Dart** library.

`dart::heap` is used to represent a dynamic tree of objects, trading
performance for mutability, and is intended for use-cases where mutation
is mandatory and common. It implements a nearly identical API compared
with `dart::packet`, but will be marginally faster in all cases where
`dart::packet::is_finalized` returns false.

`dart::buffer` is the highest performance class exposed by the library,
and is used to enable read-only interaction with an arbitrary object tree
with the least overhead possible. Exposing a subset of the `dart::packet`
API, a `dart::buffer` instance is guaranteed to _**never**_ allocate
memory in any member function (with the exception of functions that return
STL containers and conversion operators to other **Dart** types), and
is guaranteed to be stored contiguously as a flattened buffer of bytes
(accessible via `dart::buffer::get_bytes` or `dart::buffer::dup_bytes`)
immediately ready for distribution over a socket/file/shared memory queue/etc.

## Measuring Dart Performance
Given the API topology mentioned above, all performance validation cases
for **Dart** first consider whether the operation being tested requires
mutation (in which case `dart::heap` is used) or not (in which case
`dart::buffer` is used).

Additionally, all **Dart** performance test cases override the default
reference counter implementation, replacing it with `dart::unsafe_ptr`,
which implements thread-_unsafe_ reference counting.

## Building Dart Performance Tests
The included benchmark driver depends on
[Google Benchmark](https://github.com/google/benchmark), which must be
installed prior to attempting to build the benchmark driver.
Assuming GBench has been installed:
```bash
# Clone it down.
git clone git@github.com:target/libdart.git
cd libdart/

# Create the cmake build directory and prepare a build
# of the benchmark driver without tests.
mkdir build
cd build
cmake .. -Dtest=OFF -Dbenchmark=ON

# Build the benchmark driver.
make

# Run the benchmark driver.
benchmark/static_bench
```
On my middle-end 2018 MacBook Pro, this outputs something like the following:
```
Christophers-Macbook-Pro:build z002022$ benchmark/static_bench
2019-05-17 03:45:38
Running benchmark/static_bench
Run on (8 X 2700 MHz CPU s)
CPU Caches:
  L1 Data 32K (x4)
  L1 Instruction 32K (x4)
  L2 Unified 262K (x4)
  L3 Unified 8388K (x1)
--------------------------------------------------------------------------------------------------------------------
Benchmark                                                             Time           CPU Iterations UserCounters...
--------------------------------------------------------------------------------------------------------------------
benchmark_helper/parse_dynamic_flat_packet                         5048 ns       5045 ns     125143 parsed flat packets=198.233k/s
benchmark_helper/parse_dynamic_nested_packet                       3878 ns       3877 ns     175637 parsed nested packets=257.921k/s
benchmark_helper/parse_finalized_flat_packet                       3169 ns       3169 ns     214563 parsed flat packets=315.604k/s
benchmark_helper/parse_finalized_nested_packet                     2276 ns       2276 ns     300329 parsed nested packets=439.355k/s
benchmark_helper/lookup_finalized_fields                            252 ns        252 ns    2694577 finalized field lookups=59.5896M/s
benchmark_helper/lookup_finalized_random_fields/1/4                  11 ns         11 ns   61350769 finalized random field lookups=91.3506M/s
benchmark_helper/lookup_finalized_random_fields/8/4                 100 ns        100 ns    6698821 finalized random field lookups=80.131M/s
benchmark_helper/lookup_finalized_random_fields/64/4               1076 ns       1076 ns     618500 finalized random field lookups=59.4998M/s
benchmark_helper/lookup_finalized_random_fields/256/4              7934 ns       7931 ns      85135 finalized random field lookups=32.2775M/s
benchmark_helper/lookup_finalized_random_fields/1/8                  13 ns         13 ns   56890218 finalized random field lookups=79.5126M/s
benchmark_helper/lookup_finalized_random_fields/8/8                 106 ns        106 ns    6471176 finalized random field lookups=75.7222M/s
benchmark_helper/lookup_finalized_random_fields/64/8               1135 ns       1135 ns     559978 finalized random field lookups=56.3966M/s
benchmark_helper/lookup_finalized_random_fields/256/8              8313 ns       8311 ns      76428 finalized random field lookups=30.8035M/s
benchmark_helper/lookup_finalized_random_fields/1/64                 12 ns         12 ns   56405215 finalized random field lookups=84.8426M/s
benchmark_helper/lookup_finalized_random_fields/8/64                103 ns        103 ns    6237970 finalized random field lookups=77.3428M/s
benchmark_helper/lookup_finalized_random_fields/64/64              1105 ns       1104 ns     618462 finalized random field lookups=57.9479M/s
benchmark_helper/lookup_finalized_random_fields/256/64             7790 ns       7787 ns      82966 finalized random field lookups=32.8755M/s
benchmark_helper/lookup_finalized_random_fields/1/256                17 ns         17 ns   37896629 finalized random field lookups=60.2012M/s
benchmark_helper/lookup_finalized_random_fields/8/256               146 ns        146 ns    4725675 finalized random field lookups=54.8849M/s
benchmark_helper/lookup_finalized_random_fields/64/256             1671 ns       1670 ns     412155 finalized random field lookups=38.3215M/s
benchmark_helper/lookup_finalized_random_fields/256/256           10144 ns      10137 ns      64578 finalized random field lookups=25.255M/s
benchmark_helper/lookup_dynamic_fields                              577 ns        577 ns    1203183 dynamic field lookups=25.9929M/s
benchmark_helper/lookup_dynamic_random_fields/1/4                    18 ns         18 ns   36184313 dynamic random field lookups=55.0258M/s
benchmark_helper/lookup_dynamic_random_fields/8/4                   241 ns        240 ns    2856583 dynamic random field lookups=33.318M/s
benchmark_helper/lookup_dynamic_random_fields/64/4                 3591 ns       3582 ns     194295 dynamic random field lookups=17.8687M/s
benchmark_helper/lookup_dynamic_random_fields/256/4               18648 ns      18622 ns      36274 dynamic random field lookups=13.7474M/s
benchmark_helper/lookup_dynamic_random_fields/1/8                    19 ns         19 ns   36752545 dynamic random field lookups=52.0239M/s
benchmark_helper/lookup_dynamic_random_fields/8/8                   249 ns        249 ns    2661749 dynamic random field lookups=32.1194M/s
benchmark_helper/lookup_dynamic_random_fields/64/8                 3710 ns       3691 ns     190910 dynamic random field lookups=17.3387M/s
benchmark_helper/lookup_dynamic_random_fields/256/8               19846 ns      19804 ns      35016 dynamic random field lookups=12.9268M/s
benchmark_helper/lookup_dynamic_random_fields/1/64                   29 ns         29 ns   24173274 dynamic random field lookups=34.4751M/s
benchmark_helper/lookup_dynamic_random_fields/8/64                  328 ns        327 ns    2055293 dynamic random field lookups=24.44M/s
benchmark_helper/lookup_dynamic_random_fields/64/64                4847 ns       4837 ns     139262 dynamic random field lookups=13.2325M/s
benchmark_helper/lookup_dynamic_random_fields/256/64              26835 ns      26781 ns      25985 dynamic random field lookups=9.55914M/s
benchmark_helper/lookup_dynamic_random_fields/1/256                  37 ns         37 ns   18322209 dynamic random field lookups=26.7142M/s
benchmark_helper/lookup_dynamic_random_fields/8/256                 486 ns        485 ns    1493639 dynamic random field lookups=16.5026M/s
benchmark_helper/lookup_dynamic_random_fields/64/256               5898 ns       5883 ns     117024 dynamic random field lookups=10.8791M/s
benchmark_helper/lookup_dynamic_random_fields/256/256             30265 ns      30191 ns      24776 dynamic random field lookups=8.47941M/s
benchmark_helper/insert_into_exclusive_dynamic_packet               157 ns        157 ns    4358438 exclusive dynamic packet field modifications=6.38728M/s
benchmark_helper/insert_into_shared_dynamic_packet                 1208 ns       1208 ns     559499 shared dynamic packet field modifications=828.067k/s
benchmark_helper/finalize_dynamic_packet                            871 ns        871 ns     804108 finalized packets=1.14862M/s
benchmark_helper/serialize_finalized_packet_into_json              1751 ns       1750 ns     383976 serialized finalized packets=571.283k/s
benchmark_helper/serialize_finalized_nested_packet_into_json       1304 ns       1304 ns     564098 serialized finalized packets=767.106k/s
benchmark_helper/serialize_dynamic_packet_into_json                2157 ns       2157 ns     310596 serialized dynamic packets=463.703k/s
benchmark_helper/serialize_dynamic_nested_packet_into_json         1436 ns       1435 ns     490763 serialized dynamic nested packets=696.718k/s
```
The output from the previous command is generated by running each test case a
single time, which can produce somewhat noisy results, especially for the fastest
test cases.

Averaging this out across many independent runs gives more stable results, and
can be performed with the following command:
`benchmark/static_bench --benchmark_repetitions={number of repetitions} --benchmark_report_aggregates_only=true`

This will produce output for the mean test case performance, along with its
standard deviation, which is what was used to produce the graph on the main page
(16 repetitions in that particular case).

## Head-to-Head Performance Comparisons
The graph on the main page plots **Dart** performance
vs [SAJSON](https://github.com/chadaustin/sajson)
vs [Google Flexbuffers](https://github.com/google/flatbuffers)
and if those libraries are installed when building the benchmark driver, it will
automatically output an apples-to-apples comparison of similar operations with
those libraries.

## Understanding Benchmark Output
As you can see, **Dart** measures performance across a wide-variety of categories,
descriptions of those categories are listed below.

### Parse Flat/Nested Dynamic Packet
These benchmark cases are testing the whole stack of dynamic **Dart** `JSON` parsing,
from a `std::string`, parsed by the **RapidJSON** SAX parser, raised into a dynamic
`dart::heap` representation.

The distinction between the **nested** and **flat** test cases are precisely what
their names would suggest: the **flat** test case exists all at one level, whereas
the **nested** test benchmarks the additional overhead from creating nested objects.

### Parse Flat/Nested Finalized Packet
These benchmark cases are testing the whole stack of finalized **Dart** `JSON` parsing,
from a `std::string`, parsed by the **RapidJSON** DOM parser, lowered into a finalized,
contiguous, `dart::buffer` representation.

### Lookup Finalized Random Fields
This is a family of benchmark cases that test the performance of key-lookups on
a finalized packet (using `dart::buffer`).

The test cases are generated from the following parameter matrix:

| Number of Keys in Packet | Number of Characters per Key |
|:------------------------:|:----------------------------:|
|             1            |               4              |
|             8            |               4              |
|            64            |               4              |
|           256            |               4              |
|             1            |               8              |
|             8            |               8              |
|            64            |               8              |
|           256            |               8              |
|             1            |              64              |
|             8            |              64              |
|            64            |              64              |
|           256            |              64              |
|             1            |             256              |
|             8            |             256              |
|            64            |             256              |
|           256            |             256              |

This is considered to be the most important test-case from a performance perspective,
and is performed by generating a random set of keys of the specified size and length,
generating a finalized **Dart** object that contains all of those keys, and then
iterating over the set of keys and looking them up in the packet that was just
generated.

Each separate _repetition_ of each test case performed operates on an
_independently generated_ set of random keys.

### Lookup Dynamic Random Fields
This is a family of benchmark cases that test the performance of key-lookups on
a dynamic packet (using `dart::heap`).

This benchmark case is identical to the previous benchmark from a methodological
perspective, the crucial difference being that this test measures the performance
of dynamic key lookups instead of finalized key lookups.

### Insertions into an Exclusive Dynamic Packet
This benchmark case measures the cost of inserting a new key-value pair into a
dynamic object (`dart::heap`) _that is not shared_.

**Dart** uses a copy-on-write strategy to allow frictionless mutation of shared
data in a multi-threaded environment, and so the relative cost of inserting
something into a packet changes if the associated packet state is shared across
multiple instances.

### Insertions into a Shared Dynamic Packet
This benchmark case measures the cost of inserting a new key-value pair into a
dynamic object (`dart::heap`) _that is shared_.

This benchmark case is identical to the previous benchmark from a methodological
perspective, the crucial difference being that this test measures the performance
of dynamic key-value pair insertions into an object that shares its representation
across multiple packet instances, and therefore must perform a shallow copy-out
before performing the insertion.

### Finalize a Dynamic Packet
This benchmark case measures the cost of *finalizing* a dynamic packet
(the cost of casting a `dart::heap` into a `dart::buffer`).

This operation consists of recursively climbing through the dynamic representation
once to calculate an upper bound on the required size of the contiguous buffer,
performing a single allocation of the calculated size, and then recursively walking
across the dynamic representation a second time and lowering it into the allocated buffer.

### Serialize Flat/Nested Finalized Packet
These benchmark cases are testing the whole stack of **Dart** `JSON` generation,
from a finalized representation (`dart::buffer`), fed into a **RapidJSON** writer,
and output as a `std::string`.

The distinction between the **nested** and **flat** test cases are precisely what
their names would suggest: the **flat** test case exists all at one level, whereas
the **nested** test benchmarks the additional overhead of nested objects.

### Serialize Flat/Nested Dynamic Packet
These benchmark cases are testing the whole stack of **Dart** `JSON` generation,
from a dynamic representation (`dart::heap`), fed into a **RapidJSON** writer,
and output as a `std::string`.

This benchmark is identical to the previous benchmark from a methodological
perspective, the crucial difference being that this test measures the performance
of serializing a _dynamic_ packet into `JSON`.
