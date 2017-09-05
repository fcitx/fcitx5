#.rst:
# FindEnchant
# -----------
#
# Try to find the Enchant xml processing library
#
# Once done this will define
#
# ::
#
#   ENCHANT_FOUND - System has Enchant
#   ENCHANT_INCLUDE_DIR - The Enchant include directory
#   ENCHANT_LIBRARIES - The libraries needed to use Enchant
#   ENCHANT_DEFINITIONS - Compiler switches required for using Enchant
#   ENCHANT_VERSION_STRING - the version of Enchant found (since CMake 2.8.8)

# use pkg-config to get the directories and then use these values
# in the find_path() and find_library() calls
find_package(PkgConfig QUIET)
PKG_CHECK_MODULES(PC_ENCHANT QUIET enchant)

find_path(ENCHANT_INCLUDE_DIR NAMES enchant.h
   HINTS
   ${PC_ENCHANT_INCLUDEDIR}
   ${PC_ENCHANT_INCLUDE_DIRS}
   )

find_library(ENCHANT_LIBRARIES NAMES enchant
   HINTS
   ${PC_ENCHANT_LIBDIR}
   ${PC_ENCHANT_LIBRARY_DIRS}
   )

if(PC_ENCHANT_VERSION)
    set(ENCHANT_VERSION_STRING ${PC_ENCHANT_VERSION})
endif()

# handle the QUIETLY and REQUIRED arguments and set ENCHANT_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Enchant
                                  REQUIRED_VARS ENCHANT_LIBRARIES ENCHANT_INCLUDE_DIR
                                  VERSION_VAR ENCHANT_VERSION_STRING)

mark_as_advanced(ENCHANT_INCLUDE_DIR ENCHANT_LIBRARIES)

if (ENCHANT_FOUND AND NOT TARGET Enchant::Enchant)
    add_library(Enchant::Enchant UNKNOWN IMPORTED)
    set_target_properties(Enchant::Enchant PROPERTIES
        IMPORTED_LOCATION "${ENCHANT_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${ENCHANT_INCLUDE_DIR}"
        INTERFACE_COMPILE_DEFINITIONS "ENCHANT_DISABLE_DEPRECATED")
endif()

