# Define a catch library so we only have to build it once.
add_library(catch OBJECT catch_driver.cc)
target_include_directories(catch PUBLIC ../include)
target_compile_options(catch PUBLIC ${dart_default_compile_options})

# Setup toolchain specific compile flags
if (MSVC)
  set(dart_test_compile_options "/DEBUG")
else ()
  set(dart_test_compile_options "-g")
endif ()

# Setup targets for our unit tests.
add_executable(unit_tests
  operator_unit_tests.cc type_unit_tests.cc
  obj_unit_tests.cc arr_unit_tests.cc str_unit_tests.cc
  int_unit_tests.cc dcm_unit_tests.cc bool_unit_tests.cc
  null_unit_tests.cc iteration_unit_tests.cc ptr_unit_tests.cc
  misc_unit_tests.cc optional_unit_tests.cc
  string_view_unit_tests.cc view_unit_tests.cc $<TARGET_OBJECTS:catch>
)

# Configure our unit tests.
if (extended_test)
  set_property(TARGET unit_tests APPEND PROPERTY COMPILE_DEFINITIONS DART_EXTENDED_TESTS)
endif ()
if (use_asan AND MSVC)
  message(FATAL_ERROR "Address sanitizer is not currently supported on MSVC")
elseif (use_asan AND CMAKE_COMPILER_IS_GNUCXX)
  target_compile_options(unit_tests PUBLIC "-fsanitize=address")
  target_link_libraries(unit_tests PUBLIC asan)
elseif (use_asan)
  target_compile_options(unit_tests PUBLIC "-fsanitize=address")
  target_link_libraries(unit_tests PUBLIC "-fsanitize=address")
endif ()
if (gen_coverage AND CMAKE_COMPILER_IS_GNUCXX)
  target_compile_options(unit_tests PUBLIC "-fprofile-arcs;-ftest-coverage")
  target_link_libraries(unit_tests PUBLIC gcov)
elseif (gen_coverage)
  target_compile_options(unit_tests PUBLIC "--coverage")
  target_link_libraries(unit_tests PUBLIC "--coverage")
endif ()
target_include_directories(unit_tests PUBLIC ../include ${libgsl})
target_compile_options(unit_tests PUBLIC ${dart_default_compile_options} ${dart_test_compile_options})

# Configure to build our ABI if requested.
if (build_abi)
  add_executable(abi_tests heap_abi_unit_tests.cc buffer_abi_unit_tests.cc generic_abi_unit_tests.cc $<TARGET_OBJECTS:catch>)

  # Asan has to come first in linked libraries, so check if we need to link it.
  if (use_asan AND MSVC)
    message(FATAL_ERROR "Address sanitizer is not currently supported on MSVC")
  elseif (use_asan AND CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(abi_tests PUBLIC "-fsanitize=address")
    target_link_libraries(abi_tests PUBLIC asan)
  elseif (use_asan)
    target_compile_options(abi_tests PUBLIC "-fsanitize=address")
    target_link_libraries(abi_tests PUBLIC "-fsanitize=address")
  endif ()

  target_link_libraries(abi_tests PUBLIC dart_abi_static)
  target_include_directories(abi_tests PUBLIC ../include ${libgsl})
  target_compile_options(abi_tests PUBLIC ${dart_default_compile_options} ${dart_test_compile_options})
  set_property(TARGET unit_tests APPEND PROPERTY COMPILE_DEFINITIONS DART_HAS_ABI)

  # Generate coverage information if it was requested
  if (gen_coverage AND CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(abi_tests PUBLIC "-fprofile-arcs;-ftest-coverage")
    target_link_libraries(abi_tests PUBLIC gcov)
  elseif (gen_coverage)
    target_compile_options(abi_tests PUBLIC "--coverage")
    target_link_libraries(abi_tests PUBLIC "--coverage")
  endif ()
endif ()

# Handle miscellaneous compile switches
if (32bit)
  target_link_libraries(unit_tests PUBLIC atomic)
  if (build_abi)
    target_link_libraries(abi_tests PUBLIC atomic)
  endif ()
endif ()
if (static_test)
  target_link_libraries(unit_tests PUBLIC "-static-libstdc++")
  if (build_abi)
    target_link_libraries(abi_tests PUBLIC "-static-libstdc++")
  endif ()
endif ()

# Setup targets for JSON tests.
if (librj)
  add_executable(json_test json_test.cc $<TARGET_OBJECTS:catch>)

  # Asan has to come first in the list of linked libraries, so check if we need to link it.
  if (use_asan AND MSVC)
    message(FATAL_ERROR "Address sanitizer is not currently supported on MSVC")
  elseif (use_asan AND CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(json_test PUBLIC "-fsanitize=address")
    target_link_libraries(json_test PUBLIC asan)
  elseif (use_asan)
    target_compile_options(json_test PUBLIC "-fsanitize=address")
    target_link_libraries(json_test PUBLIC "-fsanitize=address")
  endif ()

  # Finish configuration
  target_include_directories(json_test PUBLIC ../include ${libgsl} ${librj})
  target_compile_options(json_test PUBLIC ${dart_default_compile_options} ${dart_test_compile_options})
  if (extended_test)
    set_property(TARGET json_test APPEND PROPERTY COMPILE_DEFINITIONS DART_EXTENDED_TESTS)
  endif ()
  if (build_abi)
    set_property(TARGET json_test APPEND PROPERTY COMPILE_DEFINITIONS DART_HAS_ABI)
    target_link_libraries(json_test PUBLIC dart_abi_static)
  endif ()

  # Generate coverage information if it was requested
  if (gen_coverage AND CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(json_test PUBLIC "-fprofile-arcs;-ftest-coverage")
    target_link_libraries(json_test PUBLIC gcov)
  elseif (gen_coverage)
    target_compile_options(json_test PUBLIC "--coverage")
    target_link_libraries(json_test PUBLIC "--coverage")
  endif ()
endif ()

# Same for yaml.
if (can_build_yaml)
  # Define the yaml test
  add_executable(yaml_test yaml_test.cc $<TARGET_OBJECTS:catch>)

  # Asan has to come first in the list of linked libraries, check if we need to link it.
  if (use_asan AND MSVC)
    message(FATAL_ERROR "Address sanitizer is not currently supported on MSVC")
  elseif (use_asan AND CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(yaml_test PUBLIC "-fsanitize=address")
    target_link_libraries(yaml_test PUBLIC asan)
  elseif (use_asan)
    target_compile_options(yaml_test PUBLIC "-fsanitize=address")
    target_link_libraries(yaml_test PUBLIC "-fsanitize=address")
  endif ()

  # Finish configuration
  target_link_libraries(yaml_test PUBLIC ${libyaml})
  target_include_directories(yaml_test PUBLIC ../include ${libgsl} ${libyaml})
  if (librj)
    target_include_directories(yaml_test PUBLIC ${librj})
  endif ()

  # Set its compile options and definitions
  set_property(TARGET yaml_test APPEND PROPERTY COMPILE_DEFINITIONS DART_HAS_YAML)
  target_compile_options(yaml_test PUBLIC ${dart_default_compile_options} ${dart_test_compile_options})
  if (extended_test)
    set_property(TARGET yaml_test APPEND PROPERTY COMPILE_DEFINITIONS DART_EXTENDED_TESTS)
  endif ()

  # Link other tests with libyaml
  target_link_libraries(unit_tests PUBLIC ${libyaml})
  if (build_abi)
    target_link_libraries(abi_tests PUBLIC ${libyaml})
  endif ()

  # Generate coverage information if it was requested
  if (gen_coverage AND CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(yaml_test PUBLIC "-fprofile-arcs;-ftest-coverage")
    target_link_libraries(yaml_test PUBLIC gcov)
  elseif (gen_coverage)
    target_compile_options(yaml_test PUBLIC "--coverage")
    target_link_libraries(yaml_test PUBLIC "--coverage")
  endif ()
endif ()

# Create a cmake test to call through to our test binary.
add_test(NAME dart_unit_tests COMMAND unit_tests)
if (librj)
  add_test(NAME dart_json_tests COMMAND json_test)
endif ()
if (can_build_yaml)
  add_test(NAME dart_yaml_tests COMMAND yaml_test)
endif ()
if (build_abi)
  add_test(NAME dart_abi_tests COMMAND abi_tests)
endif ()
file(COPY ${CMAKE_CURRENT_SOURCE_DIRECTORY}test.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIRECTORY}test.yml DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
