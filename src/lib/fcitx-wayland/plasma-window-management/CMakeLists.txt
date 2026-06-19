if (USE_SYSTEM_PLASMA_WAYLAND_PROTOCOLS)
    set(PLASMA_WINDOW_MANAGEMENT_PROTOCOL_PATHS
        ${PLASMA_WAYLAND_PROTOCOLS_DIR}/plasma-window-management.xml
    )
else()
    set(PLASMA_WINDOW_MANAGEMENT_PROTOCOL_PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/plasma-window-management.xml
    )
endif()

ecm_add_wayland_client_protocol(PLASMA_WINDOW_MANAGEMENT_PROTOCOL_SRCS
    PROTOCOL ${PLASMA_WINDOW_MANAGEMENT_PROTOCOL_PATHS}
    BASENAME plasma-window-management)

set(FCITX_WAYLAND_PLASMA_WINDOW_MANAGEMENT_SOURCES
    org_kde_plasma_activation_feedback.cpp
    org_kde_plasma_activation.cpp
    org_kde_plasma_stacking_order.cpp
    org_kde_plasma_window_management.cpp
    org_kde_plasma_window.cpp
)

add_library(Fcitx5WaylandPlasmaWindowManagement STATIC ${FCITX_WAYLAND_PLASMA_WINDOW_MANAGEMENT_SOURCES} ${PLASMA_WINDOW_MANAGEMENT_PROTOCOL_SRCS})
set_target_properties(Fcitx5WaylandPlasmaWindowManagement PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  )
target_include_directories(Fcitx5WaylandPlasmaWindowManagement PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/..;${CMAKE_CURRENT_BINARY_DIR}>")
target_link_libraries(Fcitx5WaylandPlasmaWindowManagement Wayland::Client Fcitx5::Utils Fcitx5::Wayland::Core)

add_library(Fcitx5::Wayland::PlasmaWindowManagement ALIAS Fcitx5WaylandPlasmaWindowManagement)

