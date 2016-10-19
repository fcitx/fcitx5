find_package(PkgConfig)

pkg_check_modules(WaylandProtocols QUIET "wayland-protocols>=${WaylandProtocols_FIND_VERSION}")

pkg_get_variable(WaylandProtocols_PKGDATADIR wayland-protocols pkgdatadir)

mark_as_advanced(WaylandProtocols_PKGDATADIR)

string(REGEX REPLACE "[\r\n]" "" WaylandProtocols_PKGDATADIR "${WaylandProtocols_PKGDATADIR}")

find_package_handle_standard_args(WaylandProtocols
    FOUND_VAR
        WaylandProtocols_FOUND
    REQUIRED_VARS
        WaylandProtocols_PKGDATADIR
    VERSION_VAR
        WaylandProtocols_VERSION
    HANDLE_COMPONENTS
)

set(WAYLAND_PROTOCOLS_FOUND ${WaylandProtocols_FOUND})
set(WAYLAND_PROTOCOLS_PKGDATADIR ${WaylandProtocols_PKGDATADIR})
set(WAYLAND_PROTOCOLS_VERSION ${WaylandProtocols_VERSION})

