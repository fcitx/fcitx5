ecm_add_wayland_client_protocol(FRACTIONAL_SCALE_PROTOCOL_SRCS
    PROTOCOL ${CMAKE_CURRENT_SOURCE_DIR}/fractional-scale-v1.xml
    BASENAME fractional-scale-v1)

set(FCITX_WAYLAND_FRACTIONAL_SCALE_PROTOCOL_SRCS
    wp_fractional_scale_manager_v1.cpp
    wp_fractional_scale_v1.cpp
)

add_library(Fcitx5WaylandFractionalScale STATIC ${FCITX_WAYLAND_FRACTIONAL_SCALE_PROTOCOL_SRCS} ${FRACTIONAL_SCALE_PROTOCOL_SRCS})
set_target_properties(Fcitx5WaylandFractionalScale PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  )
target_include_directories(Fcitx5WaylandFractionalScale PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/..;${CMAKE_CURRENT_BINARY_DIR}>")
target_link_libraries(Fcitx5WaylandFractionalScale Wayland::Client Fcitx5::Utils Fcitx5::Wayland::Core)

add_library(Fcitx5::Wayland::FractionalScale ALIAS Fcitx5WaylandFractionalScale)

