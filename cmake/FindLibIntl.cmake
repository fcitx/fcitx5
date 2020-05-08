# - find where dlopen and friends are located.
# LibIntl_FOUND - system has dynamic linking interface available
# LibIntl_INCLUDE_DIR - where dlfcn.h is located.
# LibIntl_LIBRARY - libraries needed to use dlopen

include(CheckFunctionExists)

find_path(LibIntl_INCLUDE_DIR NAMES libintl.h)
find_library(LibIntl_LIBRARY NAMES intl)
if(LibIntl_LIBRARY)
  set(LibIntl_FOUND TRUE)
else(LibIntl_LIBRARY)
  check_function_exists(dgettext LibIntl_FOUND)
  # If dlopen can be found without linking in dl then dlopen is part
  # of libc, so don't need to link extra libs.
  set(LibIntl_LIBRARY "")
endif(LibIntl_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibIntl
    FOUND_VAR
        LibIntl_FOUND
    REQUIRED_VARS
        LibIntl_INCLUDE_DIR
)

mark_as_advanced(LibIntl_INCLUDE_DIR LibIntl_LIBRARY)

if(LibIntl_FOUND AND NOT TARGET LibIntl::LibIntl)
    if (LibIntl_LIBRARY)
        add_library(LibIntl::LibIntl UNKNOWN IMPORTED)
        set_target_properties(LibIntl::LibIntl PROPERTIES
            IMPORTED_LOCATION "${LibIntl_LIBRARY}")
    else()
        add_library(LibIntl::LibIntl INTERFACE IMPORTED )
    endif()
    set_target_properties(LibIntl::LibIntl PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LibIntl_INCLUDE_DIR}"
    )
endif()

