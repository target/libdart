# Define a catch library so we only have to build it once.
add_library(catch OBJECT catch_driver.cc)

# Setup targets for our unit tests.
add_executable(unit_tests type_unit_tests.cc obj_unit_tests.cc arr_unit_tests.cc str_unit_tests.cc int_unit_tests.cc dcm_unit_tests.cc bool_unit_tests.cc null_unit_tests.cc iteration_unit_tests.cc ptr_unit_tests.cc misc_unit_tests.cc optional_unit_tests.cc $<TARGET_OBJECTS:catch>)
if (can_build_yaml)
  target_link_libraries(unit_tests ${libyaml})
endif ()
if (extended_test)
  target_compile_options(unit_tests PUBLIC "-DDART_EXTENDED_TESTS")
endif ()
if (32bit)
  target_link_libraries(unit_tests atomic)
endif ()
if (static_test)
  target_link_libraries(unit_tests "-static-libstdc++")
endif ()
target_compile_options(unit_tests PUBLIC "-g")

# Setup targest for JSON tests.
if (librj)
  add_executable(json_test json_test.cc $<TARGET_OBJECTS:catch>)
  target_compile_options(json_test PUBLIC "-g")
  if (extended_test)
    target_compile_options(json_test PUBLIC "-DDART_EXTENDED_TESTS")
  endif ()
endif ()

# Same for yaml.
if (can_build_yaml)
  add_executable(yaml_test yaml_test.cc $<TARGET_OBJECTS:catch>)
  target_link_libraries(yaml_test ${libyaml})
  target_compile_options(yaml_test PUBLIC "-g")
  if (extended_test)
    target_compile_options(yaml_test PUBLIC "-DDART_EXTENDED_TESTS")
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
file(COPY ${CMAKE_CURRENT_SOURCE_DIRECTORY}test.json DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_CURRENT_SOURCE_DIRECTORY}test.yml DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
