find_package(PkgConfig)

pkg_check_modules(PKG_LIBUUID QUIET uuid)

set(LIBUUID_DEFINITIONS ${PKG_LIBUUID_CFLAGS_OTHER})
set(LIBUUID_VERSION ${PKG_LIBUUID_VERSION})

find_path(LIBUUID_INCLUDE_DIR
    NAMES uuid.h
    HINTS ${PKG_LIBUUID_INCLUDE_DIRS}
)
find_library(LIBUUID_LIBRARY
    NAMES uuid
    HINTS ${PKG_LIBUUID_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LibUUID
    FOUND_VAR
        LIBUUID_FOUND
    REQUIRED_VARS
        LIBUUID_LIBRARY
        LIBUUID_INCLUDE_DIR
    VERSION_VAR
        LIBUUID_VERSION
)

if(LIBUUID_FOUND AND NOT TARGET LibUUID::LibUUID)
    add_library(LibUUID::LibUUID UNKNOWN IMPORTED)
    set_target_properties(LibUUID::LibUUID PROPERTIES
        IMPORTED_LOCATION "${LIBUUID_LIBRARY}"
        INTERFACE_COMPILE_OPTIONS "${LIBUUID_DEFINITIONS}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBUUID_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(LIBUUID_INCLUDE_DIR LIBUUID_LIBRARY)

include(FeatureSummary)
set_package_properties(LibUUID PROPERTIES
    URL "http://www.kernel.org/pub/linux/utils/util-linux/"
    DESCRIPTION "uuid library in util-linux"
)
