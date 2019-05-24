# Define a catch library so we only have to build it once.
find_package(Catch2 QUIET)
set(catch-fetch-url "https://github.com/catchorg/Catch2/archive/v2.7.2.zip")

if (NOT Catch2_FOUND)
  libdart_fetch_dependency(Catch2 URL ${catch-fetch-url})
  add_subdirectory(${Catch2_SOURCE_DIR} ${Catch2_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

add_library(catch OBJECT catch_driver.cc)
target_compile_features(catch PUBLIC cxx_std_17)
target_link_libraries(catch PUBLIC Catch2::Catch2)

# Setup targets for our unit tests.
add_executable(unit_tests)

target_compile_features(unit_tests PUBLIC cxx_std_17)

target_sources(unit_tests
  PRIVATE
    type_unit_tests.cc
    obj_unit_tests.cc
    arr_unit_tests.cc
    str_unit_tests.cc
    int_unit_tests.cc
    dcm_unit_tests.cc
    bool_unit_tests.cc
    null_unit_tests.cc
    iteration_unit_tests.cc
    ptr_unit_tests.cc
    misc_unit_tests.cc
    optional_unit_tests.cc
    string_view_unit_tests.cc
    $<TARGET_OBJECTS:catch>)

target_compile_definitions(unit_tests PUBLIC
  $<$<BOOL:${extended_test}>:DART_EXTENDED_TESTS>)

target_compile_options(unit_tests
  PRIVATE
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-g>
    $<$<CXX_COMPILER_ID:GCC>:-static-libstdc++>)

target_link_libraries(unit_tests PRIVATE dart)

if (TARGET RapidJSON::RapidJSON)
  add_executable(json_test json_test.cc $<TARGET_OBJECTS:catch>)
  target_compile_features(json_test PRIVATE cxx_std_17)
  target_compile_options(json_test PRIVATE $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-g>)
  target_compile_definitions(json_test PRIVATE
    $<$<BOOL:${extended_test}>:DART_EXTENDED_TESTS>)
  target_link_libraries(json_test PRIVATE dart)
endif()

if (TARGET Yaml::Yaml AND TARGET RapidJSON::RapidJSON)
  add_executable(yaml_test yaml_test.cc $<TARGET_OBJECTS:catch>)
  target_compile_features(yaml_test PRIVATE cxx_std_17)
  target_compile_options(yaml_test PRIVATE $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-g>)
  target_compile_definitions(yaml_test PRIVATE
    $<$<BOOL:${extended_test}>:DART_EXTENDED_TESTS>)
  target_link_libraries(yaml_test PRIVATE dart)
endif()

add_test(NAME dart_unit_tests COMMAND unit_tests)
if (TARGET RapidJSON::RapidJSON)
  add_test(NAME dart_json_tests COMMAND json_test)
endif()
if (TARGET Yaml::Yaml AND TARGET RapidJSON::RapidJSON)
  add_test(NAME dart_yaml_tests COMMAND yaml_test)
endif ()

configure_file(test.json test.json COPYONLY)
configure_file(test.yml test.yml COPYONLY)
