#----------------------------------------------------------------
# Generated CMake target import file for configuration "release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "inkcpp_c" for configuration "release"
set_property(TARGET inkcpp_c APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(inkcpp_c PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/ink/libinkcpp_c.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS inkcpp_c )
list(APPEND _IMPORT_CHECK_FILES_FOR_inkcpp_c "${_IMPORT_PREFIX}/lib/ink/libinkcpp_c.a" )

# Import target "inkcpp" for configuration "release"
set_property(TARGET inkcpp APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(inkcpp PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/ink/libinkcpp.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS inkcpp )
list(APPEND _IMPORT_CHECK_FILES_FOR_inkcpp "${_IMPORT_PREFIX}/lib/ink/libinkcpp.a" )

# Import target "inkcpp_compiler" for configuration "release"
set_property(TARGET inkcpp_compiler APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(inkcpp_compiler PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/ink/libinkcpp_compiler.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS inkcpp_compiler )
list(APPEND _IMPORT_CHECK_FILES_FOR_inkcpp_compiler "${_IMPORT_PREFIX}/lib/ink/libinkcpp_compiler.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
