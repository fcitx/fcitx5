ecm_add_wayland_client_protocol(WLR_FOREIGN_TOPLEVEL_MANAGEMENT_PROTOCOL_SRCS
    PROTOCOL ${CMAKE_CURRENT_SOURCE_DIR}/wlr-foreign-toplevel-management-unstable-v1.xml
    BASENAME wlr-foreign-toplevel-management)

set(FCITX_WAYLAND_WLR_FOREIGN_TOPLEVEL_MANAGEMENT_PROTOCOL_SOURCES
    zwlr_foreign_toplevel_manager_v1.cpp
    zwlr_foreign_toplevel_handle_v1.cpp
)

add_library(Fcitx5WaylandWLRForeignToplevelManagement STATIC ${FCITX_WAYLAND_WLR_FOREIGN_TOPLEVEL_MANAGEMENT_PROTOCOL_SOURCES} ${WLR_FOREIGN_TOPLEVEL_MANAGEMENT_PROTOCOL_SRCS})
set_target_properties(Fcitx5WaylandWLRForeignToplevelManagement PROPERTIES
  COMPILE_FLAGS "-fPIC"
  LINK_FLAGS "-Wl,--no-undefined"
  )
target_include_directories(Fcitx5WaylandWLRForeignToplevelManagement PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/..;${CMAKE_CURRENT_BINARY_DIR}>")
target_link_libraries(Fcitx5WaylandWLRForeignToplevelManagement Wayland::Client Fcitx5::Utils Fcitx5::Wayland::Core)

add_library(Fcitx5::Wayland::WLRForeignToplevelManagement ALIAS Fcitx5WaylandWLRForeignToplevelManagement)

