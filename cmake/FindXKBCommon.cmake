
include(ECMFindModuleHelpersStub)

ecm_find_package_version_check(XKBCommon)

# Note that this list needs to be ordered such that any component
# appears after its dependencies
set(XKBCommon_known_components
    XKBCommon
    X11)

set(XKBCommon_XKBCommon_component_deps)
set(XKBCommon_XKBCommon_pkg_config "xkbcommon")
set(XKBCommon_XKBCommon_lib "xkbcommon")
set(XKBCommon_XKBCommon_header "xkbcommon/xkbcommon.h")

set(XKBCommon_X11_component_deps XKBCommon)
set(XKBCommon_X11_pkg_config "xkbcommon-x11")
set(XKBCommon_X11_lib "xkbcommon-x11")
set(XKBCommon_X11_header "xkbcommon/xkbcommon-x11.h")

ecm_find_package_parse_components(XKBCommon
    RESULT_VAR XKBCommon_components
    KNOWN_COMPONENTS ${XKBCommon_known_components}
)
ecm_find_package_handle_library_components(XKBCommon
    COMPONENTS ${XKBCommon_components}
)

find_package_handle_standard_args(XKBCommon
    FOUND_VAR
        XKBCommon_FOUND
    REQUIRED_VARS
        XKBCommon_LIBRARIES
    VERSION_VAR
        XKBCommon_VERSION
    HANDLE_COMPONENTS
)

include(FeatureSummary)
set_package_properties(XKBCommon PROPERTIES
    URL "http://xkbcommon.org"
    DESCRIPTION "Keyboard handling library using XKB data"
)
