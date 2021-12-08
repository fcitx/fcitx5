ecm_add_wayland_client_protocol(WLR_DATA_CONTROL_PROTOCOL_SRCS
    PROTOCOL ${CMAKE_CURRENT_SOURCE_DIR}/wlr-data-control-unstable-v1.xml
    BASENAME wlr-data-control-unstable-v1)

set(FCITX_WAYLAND_WLR_DATA_CONTROL_PROTOCOL_SOURCES
    zwlr_data_control_device_v1.cpp
    zwlr_data_control_manager_v1.cpp
    zwlr_data_control_offer_v1.cpp
    zwlr_data_control_source_v1.cpp
)

add_library(Fcitx5WaylandWLRDataControl STATIC ${FCITX_WAYLAND_WLR_DATA_CONTROL_PROTOCOL_SOURCES} ${WLR_DATA_CONTROL_PROTOCOL_SRCS})
set_target_properties(Fcitx5WaylandWLRDataControl PROPERTIES
  COMPILE_FLAGS "-fPIC"
  LINK_FLAGS "-Wl,--no-undefined"
  )
target_include_directories(Fcitx5WaylandWLRDataControl PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/..;${CMAKE_CURRENT_BINARY_DIR}>")
target_link_libraries(Fcitx5WaylandWLRDataControl Wayland::Client Fcitx5::Utils Fcitx5::Wayland::Core)

add_library(Fcitx5::Wayland::WLRDataControl ALIAS Fcitx5WaylandWLRDataControl)
