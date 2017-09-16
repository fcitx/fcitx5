find_package(PkgConfig)

pkg_check_modules(PKG_XKEYBOARDCONFIG QUIET xkeyboard-config)

pkg_get_variable(XKEYBOARDCONFIG_XKBBASE xkeyboard-config xkb_base)
pkg_get_variable(XKEYBOARDCONFIG_DATADIR xkeyboard-config datadir)

set(XKEYBOARDCONFIG_VERSION ${PKG_XKEYBOARDCONFIG_VERSION})
mark_as_advanced(XKEYBOARDCONFIG_VERSION)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(XKeyboardConfig
    FOUND_VAR
        XKEYBOARDCONFIG_FOUND
    REQUIRED_VARS
        XKEYBOARDCONFIG_XKBBASE
        XKEYBOARDCONFIG_DATADIR
    VERSION_VAR
        XKEYBOARDCONFIG_VERSION
)
