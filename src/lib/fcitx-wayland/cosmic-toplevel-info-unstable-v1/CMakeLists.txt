ecm_add_wayland_client_protocol(COSMIC_TOPLEVEL_INFO_PROTOCOL_SRCS
    PROTOCOL ${CMAKE_CURRENT_SOURCE_DIR}/cosmic-toplevel-info-unstable-v1.xml
    BASENAME cosmic-toplevel-info-unstable-v1)

# This two are irrelevant, but pulled in as dependency.
ecm_add_wayland_client_protocol(COSMIC_TOPLEVEL_INFO_PROTOCOL_SRCS
    PROTOCOL ${WAYLAND_PROTOCOLS_PKGDATADIR}/staging/ext-workspace/ext-workspace-v1.xml
    BASENAME ext-workspace-v1)

ecm_add_wayland_client_protocol(COSMIC_TOPLEVEL_INFO_PROTOCOL_SRCS
    PROTOCOL ${CMAKE_CURRENT_SOURCE_DIR}/cosmic-workspace-unstable-v1.xml
    BASENAME cosmic-workspace-unstable-v1)

set(FCITX_WAYLAND_COSMIC_TOPLEVEL_INFO_PROTOCOL_SOURCES
    zcosmic_toplevel_info_v1.cpp
    zcosmic_toplevel_handle_v1.cpp
)

add_library(Fcitx5WaylandCosmicToplevelInfo STATIC ${FCITX_WAYLAND_COSMIC_TOPLEVEL_INFO_PROTOCOL_SOURCES} ${COSMIC_TOPLEVEL_INFO_PROTOCOL_SRCS})
set_target_properties(Fcitx5WaylandCosmicToplevelInfo PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  )
target_include_directories(Fcitx5WaylandCosmicToplevelInfo PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/..;${CMAKE_CURRENT_BINARY_DIR}>")
target_link_libraries(Fcitx5WaylandCosmicToplevelInfo
Wayland::Client Fcitx5::Utils Fcitx5::Wayland::Core
Fcitx5::Wayland::ExtForeignToplevelList)

add_library(Fcitx5::Wayland::CosmicToplevelInfo ALIAS Fcitx5WaylandCosmicToplevelInfo)

