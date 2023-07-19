ecm_add_wayland_client_protocol(VIEWPORTER_PROTOCOL_SRCS
    PROTOCOL ${WAYLAND_PROTOCOLS_PKGDATADIR}/stable/viewporter/viewporter.xml
    BASENAME viewporter)

set(FCITX_WAYLAND_VIEWPORTER_SOURCES
    wp_viewport.cpp
    wp_viewporter.cpp
)

add_library(Fcitx5WaylandViewporter STATIC ${FCITX_WAYLAND_VIEWPORTER_SOURCES} ${VIEWPORTER_PROTOCOL_SRCS})
set_target_properties(Fcitx5WaylandViewporter PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  )
target_include_directories(Fcitx5WaylandViewporter PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/..;${CMAKE_CURRENT_BINARY_DIR}>")
target_link_libraries(Fcitx5WaylandViewporter Wayland::Client Fcitx5::Utils Fcitx5::Wayland::Core)

add_library(Fcitx5::Wayland::Viewporter ALIAS Fcitx5WaylandViewporter)
