# - find where dlopen and friends are located.
# LIBINTL_FOUND - system has dynamic linking interface available
# LIBINTL_INCLUDE_DIR - where dlfcn.h is located.
# LIBINTL_LIBRARY - libraries needed to use dlopen

include(CheckFunctionExists)

find_path(LIBINTL_INCLUDE_DIR NAMES libintl.h)
find_library(LIBINTL_LIBRARY NAMES intl)
if(LIBINTL_LIBRARY)
  set(LIBINTL_FOUND TRUE)
else(LIBINTL_LIBRARY)
  check_function_exists(dgettext LIBINTL_FOUND)
  # If dlopen can be found without linking in dl then dlopen is part
  # of libc, so don't need to link extra libs.
  set(LIBINTL_LIBRARY "")
endif(LIBINTL_LIBRARY)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LIBINTL
    FOUND_VAR
        LIBINTL_FOUND
    REQUIRED_VARS
        LIBINTL_INCLUDE_DIR
)

mark_as_advanced(LIBINTL_INCLUDE_DIR LIBINTL_LIBRARY)

if(LIBINTL_FOUND AND NOT TARGET LibIntl::LibIntl)
    if (LIBINTL_LIBRARY)
        add_library(LibIntl::LibIntl UNKNOWN IMPORTED)
        set_target_properties(LibIntl::LibIntl PROPERTIES
            IMPORTED_LOCATION "${LIBINTL_LIBRARY}")
    else()
        add_library(LibIntl::LibIntl INTERFACE IMPORTED )
    endif()
    set_target_properties(LibIntl::LibIntl PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${LIBINTL_INCLUDE_DIR}"
    )
endif()

