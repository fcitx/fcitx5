
include(ECMFindModuleHelpersStub)

ecm_find_package_version_check(Cairo)

# Note that this list needs to be ordered such that any component
# appears after its dependencies
set(Cairo_known_components
    Cairo
    XCB
    XCB_SHM
    EGL)

set(Cairo_Cairo_component_deps)
set(Cairo_Cairo_pkg_config "cairo")
set(Cairo_Cairo_lib "cairo")
set(Cairo_Cairo_header "cairo/cairo.h")

set(Cairo_XCB_component_deps Cairo)
set(Cairo_XCB_pkg_config "cairo-xcb")
set(Cairo_XCB_lib "cairo")
set(Cairo_XCB_header "cairo/cairo-xcb.h")

set(Cairo_EGL_component_deps Cairo)
set(Cairo_EGL_pkg_config "cairo-egl")
set(Cairo_EGL_lib "cairo")
set(Cairo_EGL_header "cairo/cairo-gl.h")

ecm_find_package_parse_components(Cairo
    RESULT_VAR Cairo_components
    KNOWN_COMPONENTS ${Cairo_known_components}
)
ecm_find_package_handle_library_components(Cairo
    COMPONENTS ${Cairo_components}
)

find_package_handle_standard_args(Cairo
    FOUND_VAR
        Cairo_FOUND
    REQUIRED_VARS
        Cairo_LIBRARIES
    VERSION_VAR
        Cairo_VERSION
    HANDLE_COMPONENTS
)

include(FeatureSummary)
set_package_properties(Cairo PROPERTIES
    URL "http://cairographics.org/"
    DESCRIPTION "Cairo vector graphics library"
)
