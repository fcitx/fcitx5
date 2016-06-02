# Try to find Pthread functionality
# Once done this will define
#
#  PTHREAD_FOUND - system has Pthread
#  PTHREAD_INCLUDE_DIR - Pthread include directory
#  PTHREAD_LIBRARIES - Libraries needed to use Pthread
#
# TODO: This will enable translations only if Gettext functionality is
# present in libc. Must have more robust system for release, where Gettext
# functionality can also reside in standalone Gettext library, or the one
# embedded within kdelibs (cf. gettext.m4 from Gettext source).
#
# Copyright (c) 2006, Chusslove Illich, <caslav.ilic@gmx.net>
# Copyright (c) 2007, Alexander Neundorf, <neundorf@kde.org>
# Copyright (c) 2016, Xuetian Weng <wengxt@gmail.com>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

find_path(PTHREAD_INCLUDE_DIR NAMES pthread.h)

if(PTHREAD_INCLUDE_DIR)
  include(CheckFunctionExists)
  check_function_exists(pthread_create PTHREAD_LIBC_HAS_PTHREAD_CREATE)

  if (PTHREAD_LIBC_HAS_PTHREAD_CREATE)
    set(PTHREAD_LIBRARIES)
    set(PTHREAD_LIB_FOUND TRUE)
  else (PTHREAD_LIBC_HAS_PTHREAD_CREATE)
    find_library(PTHREAD_LIBRARIES NAMES pthread libpthread )
    if(PTHREAD_LIBRARIES)
      set(PTHREAD_LIB_FOUND TRUE)
    endif(PTHREAD_LIBRARIES)
  endif (PTHREAD_LIBC_HAS_PTHREAD_CREATE)

endif(PTHREAD_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Pthread
    FOUND_VAR
        PTHREAD_FOUND
    REQUIRED_VARS
        PTHREAD_INCLUDE_DIR  PTHREAD_LIB_FOUND
)

if(PTHREAD_FOUND AND NOT TARGET Pthread::Pthread)
    if (PTHREAD_LIBRARIES)
        add_library(Pthread::Pthread UNKNOWN IMPORTED)
        set_target_properties(Pthread::Pthread PROPERTIES
            IMPORTED_LOCATION "${PTHREAD_LIBRARIES}")
    else()
        add_library(Pthread::Pthread INTERFACE IMPORTED )
    endif()
    set_target_properties(Pthread::Pthread PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${PTHREAD_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(PTHREAD_INCLUDE_DIR  PTHREAD_LIBRARIES  PTHREAD_LIBC_HAS_PTHREAD_CREATE  PTHREAD_LIB_FOUND)
