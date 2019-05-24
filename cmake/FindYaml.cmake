include(FindPackageHandleStandardArgs)

find_library(YAML_LIBRARY yaml)

find_package_handle_standard_args(Yaml REQUIRED_VARS YAML_LIBRARY)

if (Yaml_FOUND)
  add_library(Yaml::Yaml IMPORTED)
  set_target_properties(Yaml::Yaml PROPERTIES
    IMPORTED_LOCATION ${YAML_LIBRARY})
endif()
