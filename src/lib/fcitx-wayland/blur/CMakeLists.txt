if (USE_SYSTEM_PLASMA_WAYLAND_PROTOCOLS)
    set(BLUR_PROTOCOL_PATHS
        ${PLASMA_WAYLAND_PROTOCOLS_DIR}/blur.xml
    )
else()
    set(BLUR_PROTOCOL_PATHS
        ${CMAKE_CURRENT_SOURCE_DIR}/blur.xml
    )
endif()

ecm_add_wayland_client_protocol(BLUR_PROTOCOL_SRCS
    PROTOCOL ${BLUR_PROTOCOL_PATHS}
    BASENAME blur)

set(FCITX_WAYLAND_BLUR_PROTOCOL_SOURCES
    org_kde_kwin_blur.cpp
    org_kde_kwin_blur_manager.cpp
)

add_library(Fcitx5WaylandBlur STATIC ${FCITX_WAYLAND_BLUR_PROTOCOL_SOURCES} ${BLUR_PROTOCOL_SRCS})
set_target_properties(Fcitx5WaylandBlur PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  )
target_include_directories(Fcitx5WaylandBlur PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/..;${CMAKE_CURRENT_BINARY_DIR}>")
target_link_libraries(Fcitx5WaylandBlur Wayland::Client Fcitx5::Utils Fcitx5::Wayland::Core)

add_library(Fcitx5::Wayland::Blur ALIAS Fcitx5WaylandBlur)

