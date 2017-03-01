#.rst:
# FindExpat
# -----------
#
# Try to find the Expat xml processing library
#
# Once done this will define
#
# ::
#
#   EXPAT_FOUND - System has Expat
#   EXPAT_INCLUDE_DIR - The Expat include directory
#   EXPAT_LIBRARIES - The libraries needed to use Expat
#   EXPAT_DEFINITIONS - Compiler switches required for using Expat
#   EXPAT_XMLLINT_EXECUTABLE - The XML checking tool xmllint coming with Expat
#   EXPAT_VERSION_STRING - the version of Expat found (since CMake 2.8.8)

# use pkg-config to get the directories and then use these values
# in the find_path() and find_library() calls
find_package(PkgConfig QUIET)
PKG_CHECK_MODULES(PC_EXPAT QUIET libxml-2.0)
set(EXPAT_DEFINITIONS ${PC_EXPAT_CFLAGS_OTHER})

find_path(EXPAT_INCLUDE_DIR NAMES expat.h
   HINTS
   ${PC_EXPAT_INCLUDEDIR}
   ${PC_EXPAT_INCLUDE_DIRS}
   )

find_library(EXPAT_LIBRARIES NAMES expat
   HINTS
   ${PC_EXPAT_LIBDIR}
   ${PC_EXPAT_LIBRARY_DIRS}
   )

if(PC_EXPAT_VERSION)
    set(EXPAT_VERSION_STRING ${PC_EXPAT_VERSION})
endif()

# handle the QUIETLY and REQUIRED arguments and set EXPAT_FOUND to TRUE if
# all listed variables are TRUE
include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Expat
                                  REQUIRED_VARS EXPAT_LIBRARIES EXPAT_INCLUDE_DIR
                                  VERSION_VAR EXPAT_VERSION_STRING)

mark_as_advanced(EXPAT_INCLUDE_DIR EXPAT_LIBRARIES)

if (EXPAT_FOUND AND NOT TARGET Expat::Expat)
    add_library(Expat::Expat UNKNOWN IMPORTED)
    set_target_properties(Expat::Expat PROPERTIES
        IMPORTED_LOCATION "${EXPAT_LIBRARIES}"
        INTERFACE_INCLUDE_DIRECTORIES "${EXPAT_INCLUDE_DIR}"
    )
endif()

