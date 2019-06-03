# Have cmake distinguish between clang and apple clang.
cmake_policy(SET CMP0025 NEW)

# Name the project.
project(libdart)
cmake_minimum_required(VERSION 3.1.0)
include(CheckIncludeFileCXX)

# Check which dependencies are installed.
find_path(libgsl gsl/gsl)
find_library(libyaml yaml)
find_path(librj rapidjson/reader.h)
if (NOT librj)
  CHECK_INCLUDE_FILE_CXX("rapidjson/reader.h" librj)
endif ()

# GSL is our only hard dependency, error out if it's not here.
if (NOT libgsl)
  message(FATAL_ERROR "Microsoft GSL must be installed")
endif ()
include_directories(${libgsl})

# Decide if we can build the YAML features or not.
if (libyaml AND librj)
  set(can_build_yaml ON)
endif ()

# Figure out what toolchain we're using.
option(force_build "Proceeding with unsupported compilation...")
if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "[cC][lL][aA][nN][gG]")
  set(using_clang ON)
  if ("${CMAKE_CXX_COMPILER_ID}" MATCHES "AppleClang")
    set(using_mac ON)
  endif ()
elseif ("${CMAKE_CXX_COMPILER_ID}" MATCHES "[gG][nN][uU]")
  set(using_gcc ON)
elseif (force_build)
  set(using_gcc ON)
else ()
  message(FATAL_ERROR "Unsupported toolchain detected, you can proceed regardless by setting the \"force_build\" flag")
endif ()

if (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 7.2 AND using_gcc)
  set(using_cxx17 ON)
elseif (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 4.9 AND using_clang AND NOT using_mac)
  set(using_cxx17 ON)
elseif (CMAKE_CXX_COMPILER_VERSION VERSION_GREATER 9.9 AND using_clang AND using_mac)
  try_compile(using_cxx17 ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/test/macos_feature_check.cc)
endif ()

# Explicitly disable C++17 for whatever reason.
option(no_cxx17 "Disabling c++17" OFF)

# Explicitly enable sajson for whatever reason.
option(use_sajson "Assuming sajson is installed, and using it" OFF)
option(build_abi "Building libdart C interface")

# Define an option for our tests.
option(test "Building tests..." ON)
option(32bit "Building for a 32-bit target..." OFF)
option(static_test "Statically linking tests..." OFF)
option(extended_test "Building extended tests..." OFF)

# Define global compiler flags.
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-implicit-fallthrough -Wno-unused-local-typedefs")
if (using_cxx17 AND NOT no_cxx17)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
else ()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif ()
if (use_sajson)
  add_definitions("-DDART_USE_SAJSON")
endif ()
if (librj)
  add_definitions("-DDART_HAS_RAPIDJSON")
endif ()
if (can_build_yaml)
  add_definitions("-DDART_HAS_YAML")
endif ()

# Define our source files.
include_directories(include)

# Build the ABI if we were asked to.
if (build_abi)
  add_library(dart_abi STATIC src/abi.cc)
  target_compile_options(dart_abi PUBLIC "-O3")
endif ()

# Conditionally setup our tests.
if (test)
  enable_testing()
  add_subdirectory(test)
endif ()

# Conditionally setup our benchmarks.
if (benchmark)
  add_subdirectory(benchmark)
endif ()

# Find our headers.
file(GLOB global_header include/dart.h)
file(GLOB headers include/dart/*.h include/dart/*.tcc include/extern/variant.h)

# Install our headers/abi
install(FILES ${global_header} DESTINATION include)
install(FILES ${headers} DESTINATION include/dart)
install(TARGETS dart_abi ARCHIVE DESTINATION lib)
