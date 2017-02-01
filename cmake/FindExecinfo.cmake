# Try to find Execinfo functionality
# Once done this will define
#
#  EXECINFO_FOUND - system has LibExecinfo
#  EXECINFO_INCLUDE_DIR - LibExecinfo include directory
#  EXECINFO_LIBRARY - Library needed to use Execinfo

include(CheckFunctionExists)
find_path(EXECINFO_INCLUDE_DIR NAMES execinfo.h)

if (EXECINFO_INCLUDE_DIR)
  check_function_exists(backtrace EXECINFO_LIBC_HAS_BACKTRACE)

  if (EXECINFO_LIBC_HAS_BACKTRACE)
    set(EXECINFO_LIBRARY "")
    set(EXECINFO_FOUND TRUE)
  else ()
    find_library(EXECINFO_LIBRARY NAMES execinfo libexecinfo )
    if (EXECINFO_LIBRARY)
      set(EXECINFO_FOUND TRUE)
    endif()
  endif ()
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Execinfo
    FOUND_VAR
        EXECINFO_FOUND
    REQUIRED_VARS
        EXECINFO_INCLUDE_DIR
)

mark_as_advanced(EXECINFO_INCLUDE_DIR EXECINFO_LIBRARY EXECINFO_LIBC_HAS_BACKTRACE)

if(EXECINFO_FOUND AND NOT TARGET Execinfo::Execinfo)
    if (EXECINFO_LIBRARY)
        add_library(Execinfo::Execinfo UNKNOWN IMPORTED)
        set_target_properties(Execinfo::Execinfo PROPERTIES
            IMPORTED_LOCATION "${EXECINFO_LIBRARY}")
    else()
        add_library(Execinfo::Execinfo INTERFACE IMPORTED )
    endif()
    set_target_properties(Execinfo::Execinfo PROPERTIES
        INTERFACE_INCLUDE_DIRECTORIES "${EXECINFO_INCLUDE_DIR}"
    )
endif()
