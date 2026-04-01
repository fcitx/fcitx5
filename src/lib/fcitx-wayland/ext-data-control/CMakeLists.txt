ecm_add_wayland_client_protocol(Ext_DATA_CONTROL_PROTOCOL_SRCS
    PROTOCOL ${WAYLAND_PROTOCOLS_PKGDATADIR}/staging/ext-data-control/ext-data-control-v1.xml
    BASENAME ext-data-control-unstable-v1)

set(FCITX_WAYLAND_Ext_DATA_CONTROL_PROTOCOL_SOURCES
    ext_data_control_device_v1.cpp
    ext_data_control_manager_v1.cpp
    ext_data_control_offer_v1.cpp
    ext_data_control_source_v1.cpp
)

add_library(Fcitx5WaylandExtDataControl STATIC ${FCITX_WAYLAND_Ext_DATA_CONTROL_PROTOCOL_SOURCES} ${Ext_DATA_CONTROL_PROTOCOL_SRCS})
set_target_properties(Fcitx5WaylandExtDataControl PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  )
target_include_directories(Fcitx5WaylandExtDataControl PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/..;${CMAKE_CURRENT_BINARY_DIR}>")
target_link_libraries(Fcitx5WaylandExtDataControl Wayland::Client Fcitx5::Utils Fcitx5::Wayland::Core)

add_library(Fcitx5::Wayland::ExtDataControl ALIAS Fcitx5WaylandExtDataControl)
