
include(ECMFindModuleHelpersStub)

ecm_find_package_version_check(Pango)

# Note that this list needs to be ordered such that any component
# appears after its dependencies
set(Pango_known_components
    Pango
    Cairo
    Ft2
    Xft)

set(Pango_Pango_component_deps)
set(Pango_Pango_pkg_config "pango")
set(Pango_Pango_lib "pango-1.0")
set(Pango_Pango_header "pango/pango.h")

set(Pango_Cairo_component_deps Pango)
set(Pango_Cairo_pkg_config "pangocairo")
set(Pango_Cairo_lib "pangocairo-1.0")
set(Pango_Cairo_header "pango/pangocairo.h")

set(Pango_Ft2_component_deps Pango)
set(Pango_Ft2_pkg_config "pangoft2")
set(Pango_Ft2_lib "pangoft2-1.0")
set(Pango_Ft2_header "pango/pangoft2.h")

set(Pango_Xft_component_deps Pango)
set(Pango_Xft_pkg_config "pangoxft")
set(Pango_Xft_lib "pangoxft-1.0")
set(Pango_Xft_header "pango/pangoxft.h")

ecm_find_package_parse_components(Pango
    RESULT_VAR Pango_components
    KNOWN_COMPONENTS ${Pango_known_components}
)
ecm_find_package_handle_library_components(Pango
    COMPONENTS ${Pango_components}
)

find_package_handle_standard_args(Pango
    FOUND_VAR
        Pango_FOUND
    REQUIRED_VARS
        Pango_LIBRARIES
    VERSION_VAR
        Pango_VERSION
    HANDLE_COMPONENTS
)

include(FeatureSummary)
set_package_properties(Pango PROPERTIES
    URL "http://www.pango.org/"
    DESCRIPTION "A library for layout and rendering of text"
)

