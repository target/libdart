cmake_minimum_required(VERSION 3.1.0)
project(libdart)

list(APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake")

include(CMakeDependentOption)
include(CTest)

include(FetchDependency) # Local

fetch_option(LIBDART_FETCH_MicrosoftGSL "Fetch Microsoft GSL if not installed")
fetch_option(LIBDART_FETCH_RapidJSON "Fetch RapidJSON if not installed")
fetch_option(LIBDART_FETCH_libyaml "Fetch libyaml if not installed")

set(json-fetch-url "https://github.com/Tencent/rapidjson/archive/v1.1.0.zip")
set(yaml-fetch-url "https://github.com/yaml/libyaml/archive/0.2.2.zip")
set(gsl-fetch-url "https://github.com/microsoft/GSL/archive/v2.0.0.zip")

find_package(MicrosoftGSL)
find_package(RapidJSON MODULE)
find_package(Yaml)

option(BUILD_BENCHMARKS "Build project benchmarks")

# Explicitly disable C++17 for whatever reason.
#option(no_cxx17 "Disabling c++17" OFF)

# Define an option for our tests.
option(32bit "Building for a 32-bit target..." OFF)
option(static_test "Statically linking tests..." OFF)
option(extended_test "Building extended tests..." OFF)
option(force_build "Proceeding with unsupported compilation...")

if (NOT MicrosoftGSL_FOUND)
  if (LIBDART_FETCH_MicrosoftGSL)
    libdart_fetch_dependency(MicrosoftGSL URL ${gsl-fetch-url})
    add_subdirectory(${MicrosoftGSL_SOURCE_DIR} ${MicrosoftGSL_BINARY_DIR})
    add_library(MicrosoftGSL ALIAS GSL)
  else()
    message(FATAL_ERROR "Microsoft GSL must be installed")
  endif()
endif()

if (NOT Yaml_FOUND AND LIBDART_FETCH_libyaml)
  libdart_fetch_dependency(Yaml URL ${yaml-fetch-url})
  set(BUILD_TESTING OFF)
  add_subdirectory(${Yaml_SOURCE_DIR} ${Yaml_BINARY_DIR} EXCLUDE_FROM_ALL)
  set(BUILD_TESTING ON)
  add_library(Yaml::Yaml ALIAS yaml)
endif()

if (NOT RapidJSON_FOUND AND LIBDART_FETCH_RapidJSON)
  libdart_fetch_dependency(RapidJSON URL ${json-fetch-url})
  add_subdirectory(${RapidJSON_SOURCE_DIR} ${RapidJSON_BINARY_DIR} EXCLUDE_FROM_ALL)
  add_library(RapidJSON INTERFACE)
  add_library(RapidJSON::RapidJSON ALIAS RapidJSON)
  target_include_directories(RapidJSON INTERFACE ${RapidJSON_SOURCE_DIR}/include)
endif()

if (TARGET Yaml::Yaml AND TARGET RapidJSON::RapidJSON)
  set(can_build_yaml ON)
endif()

add_library(dart INTERFACE)
target_include_directories(dart INTERFACE
  $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>)
target_compile_definitions(dart INTERFACE
  $<$<TARGET_EXISTS:RapidJSON::RapidJSON>:DART_HAS_RAPIDJSON>
  $<$<TARGET_EXISTS:Yaml::Yaml>:DART_HAS_YAML>)

target_compile_options(dart INTERFACE
  $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wno-implicit-fallthrough -Wno-unused-local-typedefs>)

target_compile_features(dart INTERFACE cxx_std_17)

target_link_libraries(dart INTERFACE
  MicrosoftGSL
  $<TARGET_NAME_IF_EXISTS:Yaml::Yaml>
  $<TARGET_NAME_IF_EXISTS:RapidJSON::RapidJSON>)

# Figure out what toolchain we're using.
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

# Define global compiler flags.
if (BUILD_TESTING)
  add_subdirectory(test)
endif ()

# Conditionally setup our benchmarks.
if (BUILD_BENCHMARKS)
  add_subdirectory(benchmark)
endif ()

# Find and install our headers.
file(GLOB global_header include/dart.h)
file(GLOB headers include/dart/*.h include/dart/*.tcc include/extern/variant.h)
install(FILES ${global_header} DESTINATION include)
install(FILES ${headers} DESTINATION include/dart)
