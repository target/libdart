include(FindPackageHandleStandardArgs)

find_path(MicrosoftGSL_INCLUDE_DIR gsl/gsl)

find_package_handle_standard_args(MicrosoftGSL
  REQUIRED_VARS MicrosoftGSL_INCLUDE_DIR)

if (MicrosoftGSL_FOUND)
  add_library(MicrosoftGSL INTERFACE IMPORTED)
  target_include_directories(MicrosoftGSL INTERFACE ${MicrosoftGSL_INCLUDE_DIR})
endif()


