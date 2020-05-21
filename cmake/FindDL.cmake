# - find where dlopen and friends are located.
# DL_FOUND - system has dynamic linking interface available
# DL_INCLUDE_DIR - where dlfcn.h is located.
# DL_LIBRARY - libraries needed to use dlopen

include(CheckFunctionExists)
include(CheckSymbolExists)

find_path(DL_INCLUDE_DIR NAMES dlfcn.h)
find_library(DL_LIBRARY NAMES dl)
if(DL_LIBRARY)
  set(DL_FOUND TRUE)
else(DL_LIBRARY)
  check_function_exists(dlopen DL_FOUND)
  # If dlopen can be found without linking in dl then dlopen is part
  # of libc, so don't need to link extra libs.
  set(DL_LIBRARY "")
endif(DL_LIBRARY)

if (DL_FOUND)
  set(CMAKE_REQUIRED_DEFINITIONS -D_GNU_SOURCE)
  set(CMAKE_REQUIRED_LIBRARIES ${DL_LIBRARY})
  set(CMAKE_REQUIRED_INCLUDES ${DL_INCLUDE_DIR})
  check_symbol_exists(dlmopen "dlfcn.h" HAS_DLMOPEN)
  unset(CMAKE_REQUIRED_DEFINITIONS)
  unset(CMAKE_REQUIRED_LIBRARIES)
  unset(CMAKE_REQUIRED_INCLUDES)
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(DL
    FOUND_VAR
        DL_FOUND
    REQUIRED_VARS
        DL_INCLUDE_DIR
)

mark_as_advanced(DL_INCLUDE_DIR DL_LIBRARY)

if(DL_FOUND AND NOT TARGET DL::DL)
    if (DL_LIBRARY)
        add_library(DL::DL UNKNOWN IMPORTED)
        set_target_properties(DL::DL PROPERTIES
            IMPORTED_LOCATION "${DL_LIBRARY}")
    else()
        add_library(DL::DL INTERFACE IMPORTED )
    endif()
    set_target_properties(DL::DL PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${DL_INCLUDE_DIR}"
    )
endif()
