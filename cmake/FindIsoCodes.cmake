# - Try to find the IsoCodes libraries
# Once done this will define
#
#  ISOCODES_FOUND - system has ISOCODES
#  ISOCODES_INCLUDE_DIR - the ISOCODES include directory
#  ISOCODES_LIBRARIES - ISOCODES library
#
# Copyright (c) 2012 CSSlayer <wengxt@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

if(ISOCODES_INCLUDE_DIR AND ISOCODES_LIBRARIES)
    # Already in cache, be silent
    set(ISOCODES_FIND_QUIETLY TRUE)
endif(ISOCODES_INCLUDE_DIR AND ISOCODES_LIBRARIES)

find_package(PkgConfig)
pkg_check_modules(PC_ISOCODES iso-codes)

find_file(ISOCODES_ISO639_XML iso_639.xml
          HINTS "${PC_ISOCODES_PREFIX}/share/xml/iso-codes/"
          )

find_file(ISOCODES_ISO3166_XML iso_3166.xml
          HINTS "${PC_ISOCODES_PREFIX}/share/xml/iso-codes/"
          )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(IsoCodes  DEFAULT_MSG  ISOCODES_ISO639_XML ISOCODES_ISO3166_XML)

mark_as_advanced(ISOCODES_ISO639_XML ISOCODES_ISO3166_XML)
