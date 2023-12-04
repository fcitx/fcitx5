ecm_add_wayland_client_protocol(CURSOR_SHAPE_PROTOCOL_SRCS
    PROTOCOL ${CMAKE_CURRENT_SOURCE_DIR}/cursor-shape-v1.xml
    BASENAME cursor-shape)

set(FCITX_WAYLAND_CURSOR_SHAPE_PROTOCOL_SOURCES
    wp_cursor_shape_manager_v1.cpp
    wp_cursor_shape_device_v1.cpp
)

add_library(Fcitx5WaylandCursorShape STATIC ${FCITX_WAYLAND_CURSOR_SHAPE_PROTOCOL_SOURCES} ${CURSOR_SHAPE_PROTOCOL_SRCS})
set_target_properties(Fcitx5WaylandCursorShape PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  )
target_include_directories(Fcitx5WaylandCursorShape PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/..;${CMAKE_CURRENT_BINARY_DIR}>")
target_link_libraries(Fcitx5WaylandCursorShape PUBLIC Wayland::Client Fcitx5::Utils Fcitx5::Wayland::Core Fcitx5::Wayland::Tablet)

add_library(Fcitx5::Wayland::CursorShape ALIAS Fcitx5WaylandCursorShape)

