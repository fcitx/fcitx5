ecm_add_wayland_client_protocol(WAYLAND_TABLET_PROTOCOL_SRCS
    PROTOCOL ${WAYLAND_PROTOCOLS_PKGDATADIR}/unstable/tablet/tablet-unstable-v2.xml
    BASENAME tablet-unstable-v2)

set(FCITX_WAYLAND_TABLET_SOURCES
    zwp_tablet_manager_v2.cpp
    zwp_tablet_pad_group_v2.cpp
    zwp_tablet_pad_ring_v2.cpp
    zwp_tablet_pad_strip_v2.cpp
    zwp_tablet_pad_v2.cpp
    zwp_tablet_seat_v2.cpp
    zwp_tablet_tool_v2.cpp
    zwp_tablet_v2.cpp
)

add_library(Fcitx5WaylandTablet STATIC ${FCITX_WAYLAND_TABLET_SOURCES} ${WAYLAND_TABLET_PROTOCOL_SRCS})
set_target_properties(Fcitx5WaylandTablet PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  )
target_include_directories(Fcitx5WaylandTablet PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/..;${CMAKE_CURRENT_BINARY_DIR}>")
target_link_libraries(Fcitx5WaylandTablet Wayland::Client Fcitx5::Utils Fcitx5::Wayland::Core)

add_library(Fcitx5::Wayland::Tablet ALIAS Fcitx5WaylandTablet)
