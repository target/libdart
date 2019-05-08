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
git clone git@github.com/target/libdart.git
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
2019-05-07 21:33:30
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
benchmark_helper/parse_flat_packet                                      12263 ns      12254 ns      56622 parsed flat packets=81.6055k/s
benchmark_helper/parse_nested_packet                                     6673 ns       6668 ns     105788 parsed nested packets=149.962k/s
benchmark_helper/lookup_finalized_fields                                  312 ns        312 ns    2155099 finalized field lookups=48.1192M/s
benchmark_helper/lookup_finalized_random_fields/1/4                        13 ns         13 ns   42009746 finalized random field lookups=78.7619M/s
benchmark_helper/lookup_finalized_random_fields/8/4                       123 ns        122 ns    6163111 finalized random field lookups=74.6735M/s
benchmark_helper/lookup_finalized_random_fields/64/4                     1209 ns       1208 ns     449487 finalized random field lookups=53.9942M/s
benchmark_helper/lookup_finalized_random_fields/256/4                   11656 ns      11629 ns      58433 finalized random field lookups=22.0137M/s
benchmark_helper/lookup_finalized_random_fields/1/8                        18 ns         17 ns   44253942 finalized random field lookups=70.3145M/s
benchmark_helper/lookup_finalized_random_fields/8/8                       160 ns        152 ns    4652059 finalized random field lookups=67.7103M/s
benchmark_helper/lookup_finalized_random_fields/64/8                     1340 ns       1337 ns     494490 finalized random field lookups=49.8776M/s
benchmark_helper/lookup_finalized_random_fields/256/8                   11404 ns      11402 ns      59469 finalized random field lookups=22.4521M/s
benchmark_helper/lookup_finalized_random_fields/1/64                       13 ns         13 ns   51620896 finalized random field lookups=74.9993M/s
benchmark_helper/lookup_finalized_random_fields/8/64                      116 ns        116 ns    6013022 finalized random field lookups=68.7961M/s
benchmark_helper/lookup_finalized_random_fields/64/64                    1404 ns       1404 ns     510029 finalized random field lookups=45.5854M/s
benchmark_helper/lookup_finalized_random_fields/256/64                  10836 ns      10834 ns      62048 finalized random field lookups=23.6296M/s
benchmark_helper/lookup_finalized_random_fields/1/256                      19 ns         19 ns   36768375 finalized random field lookups=52.6867M/s
benchmark_helper/lookup_finalized_random_fields/8/256                     163 ns        163 ns    4264210 finalized random field lookups=49.0541M/s
benchmark_helper/lookup_finalized_random_fields/64/256                   1993 ns       1989 ns     349724 finalized random field lookups=32.1717M/s
benchmark_helper/lookup_finalized_random_fields/256/256                 13274 ns      13272 ns      50181 finalized random field lookups=19.2882M/s
```
