if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.11)
  include(FetchContent)
endif()

include(CMakeDependentOption)

function (fetch_option name description)
  cmake_dependent_option(${name} ${description} ON
    "CMAKE_VERSION VERSION_GREATER_EQUAL 3.11" OFF)
endfunction()

function (libdart_fetch_dependency name)
  FetchContent_Declare(${name} ${ARGN})
  FetchContent_GetProperties(${name})
  if (NOT ${name}_POPULATED)
    FetchContent_Populate(${name})
    string(TOLOWER ${name} lcname)
    set(${name}_SOURCE_DIR ${${lcname}_SOURCE_DIR} PARENT_SCOPE)
    set(${name}_BINARY_DIR ${${lcname}_BINARY_DIR} PARENT_SCOPE)
  endif()
endfunction()
