# Have cmake distinguish between clang and apple clang.
cmake_policy(SET CMP0025 NEW)
cmake_policy(SET CMP0048 NEW)

# Name the project.
project(libdart VERSION 0.9)
cmake_minimum_required(VERSION 3.1.0)
include(CheckIncludeFileCXX)

# Get all of our user supplied compile switches
option(test "Building tests..." ON)
option(build_abi "Building libdart C interface")
option(32bit "Building for a 32-bit target..." OFF)
option(static_test "Statically linking tests..." OFF)
option(extended_test "Building extended tests..." OFF)
option(force_cxx17 "Assuming c++17 is available, and using it" OFF)
option(use_sajson "Assuming sajson is installed, and using it" OFF)
option(gen_coverage "Generate gcov coverage information in support envs" OFF)
option(use_asan "Link tests with address sanitizer" OFF)

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

# Decide if we can build the YAML features or not.
if (libyaml AND librj)
  set(can_build_yaml ON)
endif ()

# Set our standard version.
if (force_cxx17)
  set(DART_CXX_STANDARD "17" CACHE STRING "Use c++ standard")
else ()
  set(DART_CXX_STANDARD "14" CACHE STRING "Use c++ standard")
endif ()

# Set our compiler standard flags
if (MSVC)
  set(DART_CXX_STD_OPT "-std:c++${DART_CXX_STANDARD}")
else ()
  set(DART_CXX_STD_OPT "-std=c++${DART_CXX_STANDARD}")
endif ()

# Check to see if our compiler supports the chosen standard.
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("${DART_CXX_STD_OPT}" COMPILER_SUPPORTS_CXX_STANDARD)
if (NOT COMPILER_SUPPORTS_CXX_STANDARD)
  message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no c++${DART_CXX_STANDARD} support.")
endif ()

if (MSVC)
  set(dart_default_compile_options "${DART_CXX_STD_OPT};/EHsc;/Zc:__cplusplus;/permissive-;/bigobj")
else ()
  set(dart_default_compile_options "${DART_CXX_STD_OPT};-Wall;-Wextra;-Wno-implicit-fallthrough;-Wno-unused-local-typedefs")
endif ()

# Build the ABI if we were asked to.
if (build_abi)
  # Define the ABI build targets
  set(abi_sources src/heap.cc src/buffer.cc src/generic.cc)
  add_library(dart_abi SHARED ${abi_sources})
  add_library(dart_abi_static STATIC ${abi_sources})

  # Asan has to come first in linked libraries, so check if we need to link it.
  if (use_asan AND MSVC)
    message(FATAL_ERROR "Address sanitizer is not currently supported on MSVC")
  elseif (use_asan AND CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(dart_abi PUBLIC "-fsanitize=address")
    target_link_libraries(dart_abi PUBLIC asan)
    target_compile_options(dart_abi_static PUBLIC "-fsanitize=address")
    target_link_libraries(dart_abi_static PUBLIC asan)
  elseif (use_asan)
    target_compile_options(dart_abi PUBLIC "-fsanitize=address")
    target_link_libraries(dart_abi PUBLIC "-fsanitize=address")
    target_compile_options(dart_abi_static PUBLIC "-fsanitize=address")
    target_link_libraries(dart_abi_static PUBLIC "-fsanitize=address")
  endif ()

  # Setup include directories
  target_include_directories(dart_abi PUBLIC include ${libgsl})
  target_include_directories(dart_abi_static PUBLIC include ${libgsl})
  set_target_properties(dart_abi PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION})

  # Set our preprocessor to export dll symbols if we're on windows.
  if (MSVC)
    set_property(TARGET dart_abi APPEND PROPERTY COMPILE_DEFINITIONS DART_EXPORTS)
    set_property(TARGET dart_abi_static APPEND PROPERTY COMPILE_DEFINITIONS DART_EXPORTS)
  endif ()

  # Set compile switches based on what's installed
  if (use_sajson)
    set_property(TARGET dart_abi APPEND PROPERTY COMPILE_DEFINITIONS DART_USE_SAJSON)
    set_property(TARGET dart_abi_static APPEND PROPERTY COMPILE_DEFINITIONS DART_USE_SAJSON)
  endif ()
  if (librj)
    target_include_directories(dart_abi PUBLIC ${librj})
    target_include_directories(dart_abi_static PUBLIC ${librj})
    set_property(TARGET dart_abi APPEND PROPERTY COMPILE_DEFINITIONS DART_HAS_RAPIDJSON)
    set_property(TARGET dart_abi_static APPEND PROPERTY COMPILE_DEFINITIONS DART_HAS_RAPIDJSON)
  endif ()
  if (can_build_yaml)
    target_include_directories(dart_abi PUBLIC ${libyaml})
    target_include_directories(dart_abi_static PUBLIC ${libyaml})
    set_property(TARGET dart_abi APPEND PROPERTY COMPILE_DEFINITIONS DART_HAS_YAML)
    set_property(TARGET dart_abi_static APPEND PROPERTY COMPILE_DEFINITIONS DART_HAS_YAML)
  endif ()

  # Setup optimization flags.
  if (MSVC AND CMAKE_BUILD_TYPE MATCHES RELEASE)
    target_compile_options(dart_abi PUBLIC /O2 ${dart_default_compile_options})
    target_compile_options(dart_abi_static PUBLIC /O2 ${dart_default_compile_options})
  elseif (MSVC)
    target_compile_options(dart_abi PUBLIC /DEBUG ${dart_default_compile_options})
    target_compile_options(dart_abi_static PUBLIC /DEBUG ${dart_default_compile_options})
  elseif (CMAKE_BUILD_TYPE MATCHES RELEASE)
    target_compile_options(dart_abi PUBLIC -O3 ${dart_default_compile_options})
    target_compile_options(dart_abi_static PUBLIC -O3 ${dart_default_compile_options})
  else ()
    target_compile_options(dart_abi PUBLIC -g ${dart_default_compile_options})
    target_compile_options(dart_abi_static PUBLIC -g ${dart_default_compile_options})
  endif ()

  # Setup code coverage generation if requested.
  if (gen_coverage AND MSVC)
    message(FATAL_ERROR "Code coverage not currently supported with MSVC")
  elseif (gen_coverage AND CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(dart_abi PUBLIC "-fprofile-arcs;-ftest-coverage")
    target_compile_options(dart_abi_static PUBLIC "-fprofile-arcs;-ftest-coverage")
    target_link_libraries(dart_abi PUBLIC gcov)
    target_link_libraries(dart_abi_static PUBLIC gcov)
  elseif (gen_coverage)
    target_compile_options(dart_abi PUBLIC "--coverage")
    target_compile_options(dart_abi_static PUBLIC "--coverage")
    target_link_libraries(dart_abi PUBLIC "--coverage")
    target_link_libraries(dart_abi_static PUBLIC "--coverage")
  endif ()
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

# Install our headers/abi
install(FILES include/dart.h DESTINATION include)
install(DIRECTORY include/dart DESTINATION include)
if (build_abi)
  install(TARGETS dart_abi LIBRARY DESTINATION lib)
  install(TARGETS dart_abi_static ARCHIVE DESTINATION lib)
endif (build_abi)
