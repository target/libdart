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
2019-05-26 17:51:30
Running benchmark/static_bench
Run on (8 X 2300 MHz CPU s)
CPU Caches:
  L1 Data 32K (x4)
  L1 Instruction 32K (x4)
  L2 Unified 262K (x4)
  L3 Unified 6291K (x1)
--------------------------------------------------------------------------------------------------------------------------
Benchmark                                                                   Time           CPU Iterations UserCounters...
--------------------------------------------------------------------------------------------------------------------------
benchmark_helper/parse_dynamic_flat_packet                               6105 ns       6068 ns     106909 parsed flat packets=164.793k/s
benchmark_helper/parse_dynamic_nested_packet                             4734 ns       4711 ns     138225 parsed nested packets=212.274k/s
benchmark_helper/parse_finalized_flat_packet                             3300 ns       3284 ns     211021 parsed flat packets=304.475k/s
benchmark_helper/parse_finalized_nested_packet                           2275 ns       2265 ns     305881 parsed nested packets=441.554k/s
benchmark_helper/lookup_finalized_fields                                  313 ns        312 ns    2257038 finalized field lookups=48.0687M/s
benchmark_helper/lookup_finalized_random_fields/1/4                        12 ns         12 ns   56758291 finalized random field lookups=81.5933M/s
benchmark_helper/lookup_finalized_random_fields/8/4                       114 ns        113 ns    6029597 finalized random field lookups=70.5681M/s
benchmark_helper/lookup_finalized_random_fields/64/4                     1225 ns       1218 ns     561010 finalized random field lookups=52.5428M/s
benchmark_helper/lookup_finalized_random_fields/256/4                   10851 ns      10799 ns      62121 finalized random field lookups=23.7068M/s
benchmark_helper/lookup_finalized_random_fields/1/8                        14 ns         14 ns   51050547 finalized random field lookups=73.4215M/s
benchmark_helper/lookup_finalized_random_fields/8/8                       126 ns        125 ns    5706878 finalized random field lookups=63.8593M/s
benchmark_helper/lookup_finalized_random_fields/64/8                     1354 ns       1347 ns     512093 finalized random field lookups=47.5002M/s
benchmark_helper/lookup_finalized_random_fields/256/8                   12139 ns      12088 ns      58641 finalized random field lookups=21.1782M/s
benchmark_helper/lookup_finalized_random_fields/1/64                       13 ns         13 ns   52063577 finalized random field lookups=74.529M/s
benchmark_helper/lookup_finalized_random_fields/8/64                      122 ns        122 ns    5693881 finalized random field lookups=65.6067M/s
benchmark_helper/lookup_finalized_random_fields/64/64                    1360 ns       1355 ns     533614 finalized random field lookups=47.2316M/s
benchmark_helper/lookup_finalized_random_fields/256/64                  11263 ns      11212 ns      62593 finalized random field lookups=22.8335M/s
benchmark_helper/lookup_finalized_random_fields/1/256                      28 ns         28 ns   36364959 finalized random field lookups=36.0883M/s
benchmark_helper/lookup_finalized_random_fields/8/256                     180 ns        179 ns    3851550 finalized random field lookups=44.6335M/s
benchmark_helper/lookup_finalized_random_fields/64/256                   2010 ns       2001 ns     347429 finalized random field lookups=31.9829M/s
benchmark_helper/lookup_finalized_random_fields/256/256                 13853 ns      13795 ns      50028 finalized random field lookups=18.5577M/s
benchmark_helper/lookup_finalized_colliding_fields/0/16/8                 271 ns        269 ns    2586433 field collisions=0 finalized random field lookups=59.4129M/s
benchmark_helper/lookup_finalized_colliding_fields/8/16/8                 276 ns        274 ns    2492744 field collisions=2 finalized random field lookups=58.3096M/s
benchmark_helper/lookup_finalized_colliding_fields/32/16/8                292 ns        291 ns    2304231 field collisions=6 finalized random field lookups=54.9586M/s
benchmark_helper/lookup_finalized_colliding_fields/64/16/8                345 ns        343 ns    2031246 field collisions=11 finalized random field lookups=46.6107M/s
benchmark_helper/lookup_finalized_colliding_fields/100/16/8               432 ns        429 ns    1651793 field collisions=16 finalized random field lookups=37.2761M/s
benchmark_helper/lookup_finalized_colliding_fields/0/64/8                1352 ns       1347 ns     513223 field collisions=0 finalized random field lookups=47.5255M/s
benchmark_helper/lookup_finalized_colliding_fields/8/64/8                1356 ns       1350 ns     510037 field collisions=6 finalized random field lookups=47.4078M/s
benchmark_helper/lookup_finalized_colliding_fields/32/64/8               1448 ns       1442 ns     484785 field collisions=21 finalized random field lookups=44.3738M/s
benchmark_helper/lookup_finalized_colliding_fields/64/64/8               1591 ns       1585 ns     424513 field collisions=41 finalized random field lookups=40.3765M/s
benchmark_helper/lookup_finalized_colliding_fields/100/64/8              2523 ns       2513 ns     284134 field collisions=64 finalized random field lookups=25.4651M/s
benchmark_helper/lookup_finalized_colliding_fields/0/256/8              11906 ns      11855 ns      59935 field collisions=0 finalized random field lookups=21.5948M/s
benchmark_helper/lookup_finalized_colliding_fields/8/256/8              12075 ns      12021 ns      58699 field collisions=21 finalized random field lookups=21.2962M/s
benchmark_helper/lookup_finalized_colliding_fields/32/256/8             12388 ns      12345 ns      56578 field collisions=82 finalized random field lookups=20.7377M/s
benchmark_helper/lookup_finalized_colliding_fields/64/256/8             12264 ns      12216 ns      56022 field collisions=164 finalized random field lookups=20.9565M/s
benchmark_helper/lookup_finalized_colliding_fields/100/256/8            18975 ns      18894 ns      36722 field collisions=256 finalized random field lookups=13.5491M/s
benchmark_helper/lookup_dynamic_fields                                    634 ns        631 ns    1099920 dynamic field lookups=23.7654M/s
benchmark_helper/lookup_dynamic_random_fields/1/4                          19 ns         19 ns   35500738 dynamic random field lookups=51.589M/s
benchmark_helper/lookup_dynamic_random_fields/8/4                         252 ns        250 ns    2543133 dynamic random field lookups=31.9393M/s
benchmark_helper/lookup_dynamic_random_fields/64/4                       3632 ns       3612 ns     193343 dynamic random field lookups=17.7166M/s
benchmark_helper/lookup_dynamic_random_fields/256/4                     19501 ns      19412 ns      35589 dynamic random field lookups=13.1875M/s
benchmark_helper/lookup_dynamic_random_fields/1/8                          22 ns         21 ns   32006731 dynamic random field lookups=46.5888M/s
benchmark_helper/lookup_dynamic_random_fields/8/8                         300 ns        299 ns    2584848 dynamic random field lookups=26.7503M/s
benchmark_helper/lookup_dynamic_random_fields/64/8                       3964 ns       3943 ns     180605 dynamic random field lookups=16.2328M/s
benchmark_helper/lookup_dynamic_random_fields/256/8                     20766 ns      20666 ns      34049 dynamic random field lookups=12.3874M/s
benchmark_helper/lookup_dynamic_random_fields/1/64                         32 ns         32 ns   22227373 dynamic random field lookups=31.4766M/s
benchmark_helper/lookup_dynamic_random_fields/8/64                        393 ns        391 ns    1966358 dynamic random field lookups=20.4712M/s
benchmark_helper/lookup_dynamic_random_fields/64/64                      5581 ns       5555 ns     125103 dynamic random field lookups=11.5222M/s
benchmark_helper/lookup_dynamic_random_fields/256/64                    29146 ns      29001 ns      23864 dynamic random field lookups=8.82729M/s
benchmark_helper/lookup_dynamic_random_fields/1/256                        45 ns         45 ns   15519000 dynamic random field lookups=22.2487M/s
benchmark_helper/lookup_dynamic_random_fields/8/256                       491 ns        489 ns    1339021 dynamic random field lookups=16.3709M/s
benchmark_helper/lookup_dynamic_random_fields/64/256                     6524 ns       6495 ns     107920 dynamic random field lookups=9.85423M/s
benchmark_helper/lookup_dynamic_random_fields/256/256                   33718 ns      33579 ns      20743 dynamic random field lookups=7.62392M/s
benchmark_helper/iterate_finalized_random_fields/1                          7 ns          7 ns   99265436 finalized random field iterations=144.137M/s
benchmark_helper/iterate_finalized_random_fields/8                         30 ns         29 ns   23412311 finalized random field iterations=271.592M/s
benchmark_helper/iterate_finalized_random_fields/64                       387 ns        385 ns    3248501 finalized random field iterations=166.118M/s
benchmark_helper/iterate_finalized_random_fields/256                      830 ns        826 ns     830328 finalized random field iterations=309.789M/s
benchmark_helper/iterate_finalized_random_elements/1                        7 ns          7 ns   96643702 finalized random element iterations=140.281M/s
benchmark_helper/iterate_finalized_random_elements/8                       28 ns         28 ns   25208510 finalized random element iterations=287.553M/s
benchmark_helper/iterate_finalized_random_elements/64                     190 ns        189 ns    3630178 finalized random element iterations=337.755M/s
benchmark_helper/iterate_finalized_random_elements/256                    756 ns        752 ns     936517 finalized random element iterations=340.264M/s
benchmark_helper/iterate_dynamic_random_fields/1                           15 ns         15 ns   45300406 dynamic random field iterations=65.5481M/s
benchmark_helper/iterate_dynamic_random_fields/8                           87 ns         86 ns    8014655 dynamic random field iterations=92.7464M/s
benchmark_helper/iterate_dynamic_random_fields/64                         708 ns        705 ns    1008340 dynamic random field iterations=90.8304M/s
benchmark_helper/iterate_dynamic_random_fields/256                       3152 ns       3135 ns     220794 dynamic random field iterations=81.6684M/s
benchmark_helper/iterate_dynamic_random_elements/1                         16 ns         16 ns   42796965 dynamic random element iterations=62.4425M/s
benchmark_helper/iterate_dynamic_random_elements/8                         94 ns         93 ns    7475917 dynamic random element iterations=85.8154M/s
benchmark_helper/iterate_dynamic_random_elements/64                       626 ns        624 ns    1106474 dynamic random element iterations=102.628M/s
benchmark_helper/iterate_dynamic_random_elements/256                     2447 ns       2433 ns     284680 dynamic random element iterations=105.209M/s
benchmark_helper/access_sequential_finalized_strings                      221 ns        220 ns    3172632 finalized sequential element accesses=291.409M/s
benchmark_helper/access_sequential_dynamic_strings                        433 ns        431 ns    1620179 dynamic sequential element accesses=148.606M/s
benchmark_helper/access_random_finalized_strings                         3369 ns       3352 ns     205073 finalized random element accesses=305.465M/s
benchmark_helper/access_random_dynamic_strings                           7188 ns       7150 ns      97776 finalized random element accesses=143.216M/s
benchmark_helper/insert_into_exclusive_dynamic_object                     303 ns        301 ns    2297583 exclusive dynamic object field modifications=3.31352M/s
benchmark_helper/insert_into_exclusive_dynamic_array                       36 ns         36 ns   19456280 exclusive dynamic array element modifications=27.5286M/s
benchmark_helper/insert_into_shared_dynamic_object                       1449 ns       1443 ns     477024 shared dynamic object field modifications=692.887k/s
benchmark_helper/insert_into_shared_dynamic_array                         460 ns        459 ns    1529971 shared dynamic array element modifications=2.17982M/s
benchmark_helper/finalize_dynamic_packet                                 1018 ns       1014 ns     686510 finalized packets=986.626k/s
benchmark_helper/serialize_finalized_packet_into_json                    2081 ns       2072 ns     327429 serialized finalized packets=482.667k/s
benchmark_helper/serialize_finalized_nested_packet_into_json             1411 ns       1405 ns     483980 serialized finalized packets=711.926k/s
benchmark_helper/serialize_dynamic_packet_into_json                      2312 ns       2300 ns     303459 serialized dynamic packets=434.85k/s
benchmark_helper/serialize_dynamic_nested_packet_into_json               1598 ns       1589 ns     436412 serialized dynamic nested packets=629.252k/s
benchmark_helper/unwrap_finalized_string                                 2280 ns       2270 ns     308526 finalized string value accesses=451.046M/s
benchmark_helper/unwrap_dynamic_string                                    857 ns        853 ns     784973 dynamic string value accesses=1.20047G/s
benchmark_helper/unwrap_finalized_integer                                 955 ns        950 ns     747855 finalized integer value accesses=1077.55M/s
benchmark_helper/unwrap_dynamic_integer                                   566 ns        563 ns    1184834 dynamic integer value accesses=1.81998G/s
benchmark_helper/unwrap_finalized_decimal                                 915 ns        911 ns     744056 finalized decimal value accesses=1.12422G/s
benchmark_helper/unwrap_dynamic_decimal                                   569 ns        566 ns    1222046 dynamic decimal value accesses=1.80852G/s
benchmark_helper/unwrap_finalized_boolean                                 927 ns        922 ns     752001 finalized boolean value accesses=1.11007G/s
benchmark_helper/unwrap_dynamic_boolean                                   580 ns        577 ns    1161614 dynamic boolean value accesses=1.77433G/s
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

### Lookup Finalized Colliding Fields
This is a family of benchmark cases that test the performance of key-lookups when
the keyspace of the packet being tested is intentionally filled with prefix collisions
(uses `dart::buffer`).

The test cases are generated from the following parameter matrix:

| Percentage of Collisions in Packet | Number of Keys in Packet | Number of Characters per Key |
|:----------------------------------:|:------------------------:|:----------------------------:|
|                 0                  |            16            |               8              |
|                 8                  |            16            |               8              |
|                32                  |            16            |               8              |
|                64                  |            16            |               8              |
|               100                  |            16            |               8              |
|                 0                  |            64            |               8              |
|                 8                  |            64            |               8              |
|                32                  |            64            |               8              |
|                64                  |            64            |               8              |
|               100                  |            64            |               8              |
|                 0                  |           256            |               8              |
|                 8                  |           256            |               8              |
|                32                  |           256            |               8              |
|                64                  |           256            |               8              |
|               100                  |           256            |               8              |

This is considered to be the second most important test-case from a performance perspective,
and serves to validate the real-world applicability of **Dart**'s prefix optimizations.

It is performed by first generating a random set of keys that _do not_ have any collisions
in their prefix bytes (best case scenario for **Dart**), and then intentionally inserting
additional, randomly colliding, keys until the requested collision percentage is reached
(will always generate _at least_ the percentage requested, may actually generate a higher
percentage depending on the total number of keys requested).

Each separate _repetition_ of each test case performed operates on an
_independently generated_ set of randomly colliding keys.

### Lookup Dynamic Random Fields
This is a family of benchmark cases that test the performance of key-lookups on
a dynamic packet (using `dart::heap`).

This benchmark case is identical to the previous benchmark from a methodological
perspective, the crucial difference being that this test measures the performance
of dynamic key lookups instead of finalized key lookups.

### Iterate over Random, Finalized, Fields
This is a family of benchmark cases that test the peformance of finalized object
iteration, iterating over a set of randomly generated strings.

The test cases are generated from the following parameter matrix:

| Number of Fields in Packet |
|:--------------------------:|
|             1              |
|             8              |
|            64              |
|           256              |

Each separate _repetition_ of each test case performed operates on an
_independently generated_ set of random fields.

### Iterate over Random, Dynamic, Fields
This is a family of benchmark cases that test the peformance of dynamic object
iteration, iterating over a set of randomly generated strings.

The test cases are generated from the following parameter matrix:

| Number of Fields in Packet |
|:--------------------------:|
|             1              |
|             8              |
|            64              |
|           256              |

Each separate _repetition_ of each test case performed operates on an
_independently generated_ set of random fields.

### Sequentially Access Finalized Strings
This benchmark case measures the performance of sequentially pulling strings
out of a finalized array (`dart::buffer`).

This benchmark is intended to benchmark the performance of array lookup, and
simply iterates from 0 -> [array size] and loads a value for each index.

### Sequentially Access Dynamic Strings
This benchmark case measures the performance of sequentially pulling strings
out of a dynamic array (`dart::heap`).

This benchmark is intended to benchmark the performance of array lookup, and
simply iterates from 0 -> [array size] and loads a value for each index.

### Randomly Access Finalized Strings
This benchmark case measures the performance of randomly pulling strings
out of a finalized array (`dart::buffer`).

This benchmark is intended to benchmark the performance of randomized array lookup,
and generates a set of random indices and loads a value for each index.

### Randomly Access Dynamic Strings
This benchmark case measures the performance of randomly pulling strings
out of a dynamic array (`dart::heap`).

This benchmark is intended to benchmark the performance of randomized array lookup,
and generates a set of random indices and loads a value for each index.

### Insertions into an Exclusive Dynamic Object
This benchmark case measures the cost of inserting a new key-value pair into a
dynamic object (`dart::heap`) _that is not shared_.

**Dart** uses a copy-on-write strategy to allow frictionless mutation of shared
data in a multi-threaded environment, and so the relative cost of inserting
something into a packet changes if the associated packet state is shared across
multiple instances.

### Insertions into an Exclusive Dynamic Array
This benchmark case measures the cost of pushing a new element onto the end of a
dynamic array (`dart::heap`) _that is not shared_.

**Dart** uses a copy-on-write strategy to allow frictionless mutation of shared
data in a multi-threaded environment, and so the relative cost of inserting
something into a packet changes if the associated packet state is shared across
multiple instances.

### Insertions into a Shared Dynamic Object
This benchmark case measures the cost of inserting a new key-value pair into a
dynamic object (`dart::heap`) _that is shared_.

This benchmark case is identical to the previous benchmark from a methodological
perspective, the crucial difference being that this test measures the performance
of dynamic key-value pair insertions into an object that shares its representation
across multiple packet instances, and therefore must perform a shallow copy-out
before performing the insertion.

### Insertions into a Shared Dynamic Array
This benchmark case measures the cost of pushing a new element onto the end of a
dynamic array (`dart::heap`) _that is shared_.

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

### Unwrap Finalized String
This benchmark case measurements the cost of unwrapping a finalized **Dart** string
object (`dart::buffer::string`) to an underlying machine type.

### Unwrap Dynamic String
This benchmark case measurements the cost of unwrapping a dynamic **Dart** string
object (`dart::heap::string`) to an underlying machine type.

### Unwrap Finalized Integer
This benchmark case measurements the cost of unwrapping a finalized **Dart** integer
object (`dart::buffer::number`) to an underlying machine type.

### Unwrap Dynamic Integer
This benchmark case measurements the cost of unwrapping a dynamic **Dart** integer
object (`dart::heap::number`) to an underlying machine type.

### Unwrap Finalized Decimal
This benchmark case measurements the cost of unwrapping a finalized **Dart** decimal
object (`dart::buffer::number`) to an underlying machine type.

### Unwrap Dynamic Decimal
This benchmark case measurements the cost of unwrapping a dynamic **Dart** decimal
object (`dart::heap::number`) to an underlying machine type.

### Unwrap Finalized Boolean
This benchmark case measurements the cost of unwrapping a finalized **Dart** boolean
object (`dart::buffer::flag`) to an underlying machine type.

### Unwrap Dynamic Boolean
This benchmark case measurements the cost of unwrapping a dynamic **Dart** boolean
object (`dart::heap::flag`) to an underlying machine type.

## Conclusions
The resulting curves for some of the benchmark cases are included below.
These graphs were generated with 16 repetitions, outputting to csv, and using the
standard deviation of each test case as its error.

### Lookup Finalized Random Fields
![Dart vs Google Flexbuffers vs SAJSON](benchmark/dart.png)
The purpose of this graph is to plot **Dart** key lookup performance against that
of similar libraries, to measure the benefit for its prefix optimizations.

### Lookup Finalized Colliding Fields
![Dart Field Collision Performance](benchmark/prefix_collisions.png)
The purpose of this graph is to validate the usefulness of **Dart**'s prefix
optimizations with less than ideal (collision heavy) data.
