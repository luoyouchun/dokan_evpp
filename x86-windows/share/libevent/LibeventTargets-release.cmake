#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "event" for configuration "Release"
set_property(TARGET event APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(event PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/event.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS event )
list(APPEND _IMPORT_CHECK_FILES_FOR_event "${_IMPORT_PREFIX}/lib/event.lib" )

# Import target "event_core" for configuration "Release"
set_property(TARGET event_core APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(event_core PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/event_core.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS event_core )
list(APPEND _IMPORT_CHECK_FILES_FOR_event_core "${_IMPORT_PREFIX}/lib/event_core.lib" )

# Import target "event_extra" for configuration "Release"
set_property(TARGET event_extra APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(event_extra PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "C"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/event_extra.lib"
  )

list(APPEND _IMPORT_CHECK_TARGETS event_extra )
list(APPEND _IMPORT_CHECK_FILES_FOR_event_extra "${_IMPORT_PREFIX}/lib/event_extra.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
