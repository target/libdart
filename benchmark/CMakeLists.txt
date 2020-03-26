# Make sure gbench is installed.
find_library(libyajl yajl)
find_path(libsajson sajson.h)
find_library(libjansson jansson)
find_library(libbenchmark benchmark)
find_path(libnljson nlohmann/json.hpp)
find_library(libflatbuffers flatbuffers)
if (NOT libbenchmark)
  message(FATAL_ERROR "Google benchmark must be installed")
endif (NOT libbenchmark)
find_path(libbenchmark_include benchmark/benchmark.h)

# Setup our threading library
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Define our benchmark executable.
add_executable(static_bench static.cc)
if (librj)
  add_executable(configurable_bench configurable.cc)
endif ()

# Setup global flags
target_compile_options(static_bench PUBLIC -O3 -DNDEBUG ${dart_default_compile_options})
target_include_directories(static_bench PUBLIC ${libbenchmark_include})
target_link_libraries(static_bench Threads::Threads ${libbenchmark})
if (librj)
  target_compile_options(configurable_bench PUBLIC -O3 -DNDEBUG ${dart_default_compile_options})
  target_include_directories(configurable_bench PUBLIC ${libbenchmark_include})
  target_link_libraries(configurable_bench Threads::Threads ${libbenchmark})
endif ()

# Link against libyaml if we have it.
if (libyaml)
  target_link_libraries(static_bench ${libyaml})
endif (libyaml)

# Link against flatbuffers if we have it
if (libflatbuffers)
  target_compile_options(static_bench PUBLIC -DDART_HAS_FLEXBUFFERS)
  target_compile_options(configurable_bench PUBLIC -DDART_HAS_FLEXBUFFERS)
endif ()

# Handle YAJL
if (libyajl)
  target_link_libraries(static_bench ${libyajl})
  target_compile_options(static_bench PUBLIC -DDART_HAS_YAJL)
  if (librj)
    target_link_libraries(configurable_bench ${libyajl})
    target_compile_options(configurable_bench PUBLIC -DDART_HAS_YAJL)
  endif ()
endif ()

# Handle nlohmann json
if (libnljson)
  target_compile_options(static_bench PUBLIC -DDART_HAS_NLJSON)
  if (libnljson)
    target_compile_options(configurable_bench PUBLIC -DDART_HAS_NLJSON)
  endif ()
endif ()

# Handle Jansson
if (libjansson)
  target_link_libraries(static_bench ${libjansson})
  target_compile_options(static_bench PUBLIC -DDART_HAS_JANSSON)
  if (librj)
    target_link_libraries(configurable_bench ${libjansson})
    target_compile_options(configurable_bench PUBLIC -DDART_HAS_JANSSON)
  endif ()
endif ()

# Handle sajson
if (libsajson)
  target_compile_options(static_bench PUBLIC -DDART_USE_SAJSON -Wno-unused-parameter)
  if (librj)
    target_compile_options(configurable_bench PUBLIC -DDART_USE_SAJSON -Wno-unused-parameter)
  endif ()
endif ()
