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
Christophers-MacBook-Pro-2:build christopherfretz$ benchmark/static_bench
2019-05-10 14:51:04                                                                                                                                                                                                                                                      [25/58]
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
benchmark_helper/parse_flat_packet                                 5005 ns       5003 ns     123466 parsed flat packets=199.885k/s
benchmark_helper/parse_nested_packet                               3950 ns       3948 ns     171170 parsed nested packets=253.3k/s
benchmark_helper/lookup_finalized_fields                            256 ns        255 ns    2811956 finalized field lookups=58.9306M/s
benchmark_helper/lookup_finalized_random_fields/1/4                  11 ns         11 ns   64348881 finalized random field lookups=94.2035M/s
benchmark_helper/lookup_finalized_random_fields/8/4                  92 ns         92 ns    7186637 finalized random field lookups=86.5766M/s
benchmark_helper/lookup_finalized_random_fields/64/4               1002 ns       1002 ns     680113 finalized random field lookups=63.8931M/s
benchmark_helper/lookup_finalized_random_fields/256/4              9133 ns       9126 ns      72762 finalized random field lookups=28.051M/s
benchmark_helper/lookup_finalized_random_fields/1/8                  12 ns         12 ns   59496490 finalized random field lookups=83.3267M/s
benchmark_helper/lookup_finalized_random_fields/8/8                 107 ns        107 ns    6662289 finalized random field lookups=75.0079M/s
benchmark_helper/lookup_finalized_random_fields/64/8               1119 ns       1118 ns     616925 finalized random field lookups=57.2417M/s
benchmark_helper/lookup_finalized_random_fields/256/8             10105 ns      10100 ns      68101 finalized random field lookups=25.3463M/s
benchmark_helper/lookup_finalized_random_fields/1/64                 12 ns         12 ns   59365300 finalized random field lookups=80.6732M/s
benchmark_helper/lookup_finalized_random_fields/8/64                104 ns        104 ns    7002101 finalized random field lookups=77.2479M/s
benchmark_helper/lookup_finalized_random_fields/64/64              1113 ns       1112 ns     629808 finalized random field lookups=57.5349M/s
benchmark_helper/lookup_finalized_random_fields/256/64             9379 ns       9374 ns      70153 finalized random field lookups=27.3096M/s
benchmark_helper/lookup_finalized_random_fields/1/256                24 ns         24 ns   42037135 finalized random field lookups=42.3001M/s
benchmark_helper/lookup_finalized_random_fields/8/256               141 ns        141 ns    4838598 finalized random field lookups=56.9238M/s
benchmark_helper/lookup_finalized_random_fields/64/256             1649 ns       1648 ns     411956 finalized random field lookups=38.8412M/s
benchmark_helper/lookup_finalized_random_fields/256/256           11623 ns      11617 ns      59116 finalized random field lookups=22.0368M/s
benchmark_helper/lookup_dynamic_fields                              540 ns        540 ns    1238763 dynamic field lookups=27.7971M/s
benchmark_helper/lookup_dynamic_random_fields/1/4                    16 ns         16 ns   41404928 dynamic random field lookups=61.137M/s
benchmark_helper/lookup_dynamic_random_fields/8/4                   208 ns        207 ns    3265550 dynamic random field lookups=38.6262M/s
benchmark_helper/lookup_dynamic_random_fields/64/4                 3094 ns       3093 ns     228926 dynamic random field lookups=20.6936M/s
benchmark_helper/lookup_dynamic_random_fields/256/4               17066 ns      17060 ns      41108 dynamic random field lookups=15.0059M/s
benchmark_helper/lookup_dynamic_random_fields/1/8                    18 ns         18 ns   38149010 dynamic random field lookups=55.1387M/s
benchmark_helper/lookup_dynamic_random_fields/8/8                   259 ns        259 ns    3056115 dynamic random field lookups=30.8507M/s
benchmark_helper/lookup_dynamic_random_fields/64/8                 3409 ns       3407 ns     210649 dynamic random field lookups=18.7859M/s
benchmark_helper/lookup_dynamic_random_fields/256/8               17484 ns      17475 ns      40096 dynamic random field lookups=14.6494M/s
benchmark_helper/lookup_dynamic_random_fields/1/64                   26 ns         26 ns   26746754 dynamic random field lookups=38.1993M/s
benchmark_helper/lookup_dynamic_random_fields/8/64                  297 ns        297 ns    2226711 dynamic random field lookups=26.9792M/s
benchmark_helper/lookup_dynamic_random_fields/64/64                4342 ns       4340 ns     158728 dynamic random field lookups=14.746M/s
benchmark_helper/lookup_dynamic_random_fields/256/64              23768 ns      23758 ns      30051 dynamic random field lookups=10.7754M/s
benchmark_helper/lookup_dynamic_random_fields/1/256                  36 ns         36 ns   18869196 dynamic random field lookups=27.713M/s
benchmark_helper/lookup_dynamic_random_fields/8/256                 427 ns        427 ns    1670509 dynamic random field lookups=18.7254M/s
benchmark_helper/lookup_dynamic_random_fields/64/256               5769 ns       5766 ns     121775 dynamic random field lookups=11.1M/s
benchmark_helper/lookup_dynamic_random_fields/256/256             29609 ns      29602 ns      23458 dynamic random field lookups=8.64811M/s
benchmark_helper/insert_into_exclusive_dynamic_packet               196 ns        196 ns    4025765 exclusive dynamic packet field modifications=5.10644M/s
benchmark_helper/insert_into_shared_dynamic_packet                 1263 ns       1259 ns     531903 shared dynamic packet field modifications=793.993k/s
benchmark_helper/finalize_dynamic_packet                            958 ns        955 ns     727696 finalized packets=1047.41k/s
benchmark_helper/serialize_finalized_packet_into_json              1813 ns       1809 ns     370496 serialized finalized packets=552.872k/s
benchmark_helper/serialize_finalized_nested_packet_into_json       1289 ns       1283 ns     526462 serialized finalized packets=779.452k/s
benchmark_helper/serialize_dynamic_packet_into_json                2046 ns       2039 ns     348484 serialized dynamic packets=490.329k/s
benchmark_helper/serialize_dynamic_nested_packet_into_json         1471 ns       1467 ns     484513 serialized dynamic nested packets=681.54k/s
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

### Parse Flat/Nested Packet
These benchmark cases are testing the whole stack of **Dart** `JSON` parsing, from
a `std::string`, parsed by the **RapidJSON** SAX parser, raised into a dynamic
`dart::heap` representation.

The distinction between the **nested** and **flat** test cases are precisely what
their names would suggest: the **flat** test case exists all at one level, whereas
the **nested** test benchmarks the additional overhead from creating nested objects.

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
