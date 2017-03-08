find_package(PkgConfig)

pkg_check_modules(PKG_SYSTEMD QUIET systemd)

set(SYSTEMD_DEFINITIONS ${PKG_SYSTEMD_CFLAGS_OTHER})
set(SYSTEMD_VERSION ${PKG_SYSTEMD_VERSION})

find_path(SYSTEMD_INCLUDE_DIR
    NAMES systemd/sd-bus.h systemd/sd-event.h
    HINTS ${PKG_SYSTEMD_INCLUDE_DIRS}
)

find_library(SYSTEMD_LIBRARY
    NAMES systemd
    HINTS ${PKG_SYSTEMD_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Systemd
    FOUND_VAR
        SYSTEMD_FOUND
    REQUIRED_VARS
        SYSTEMD_LIBRARY
        SYSTEMD_INCLUDE_DIR
    VERSION_VAR
        SYSTEMD_VERSION
)

if(SYSTEMD_FOUND AND NOT TARGET Systemd::Systemd)
    add_library(Systemd::Systemd UNKNOWN IMPORTED)
    set_target_properties(Systemd::Systemd PROPERTIES
        IMPORTED_LOCATION "${SYSTEMD_LIBRARY}"
        INTERFACE_COMPILE_OPTIONS "${SYSTEMD_DEFINITIONS}"
        INTERFACE_INCLUDE_DIRECTORIES "${SYSTEMD_INCLUDE_DIR}"
    )
endif()

mark_as_advanced(SYSTEMD_INCLUDE_DIR SYSTEMD_ARCH_INCLUDE_DIR SYSTEMD_LIBRARY)

include(FeatureSummary)
set_package_properties(Systemd PROPERTIES
    URL "http://www.freedesktop.org/wiki/Software/systemd"
    DESCRIPTION "A system and service manager for Linux"
)
