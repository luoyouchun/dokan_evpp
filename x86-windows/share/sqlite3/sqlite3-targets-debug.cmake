#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "sqlite3" for configuration "Debug"
set_property(TARGET sqlite3 APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(sqlite3 PROPERTIES
  IMPORTED_IMPLIB_DEBUG "${_IMPORT_PREFIX}/debug/lib/sqlite3.lib"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/bin/sqlite3.dll"
  )

list(APPEND _IMPORT_CHECK_TARGETS sqlite3 )
list(APPEND _IMPORT_CHECK_FILES_FOR_sqlite3 "${_IMPORT_PREFIX}/debug/lib/sqlite3.lib" "${_IMPORT_PREFIX}/debug/bin/sqlite3.dll" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
