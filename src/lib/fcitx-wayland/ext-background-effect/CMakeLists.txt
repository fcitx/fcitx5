

ecm_add_wayland_client_protocol(EXT_BACKGROUND_EFFECT_PROTOCOL_SRCS
    PROTOCOL ${WAYLAND_PROTOCOLS_PKGDATADIR}/staging/ext-background-effect/ext-background-effect-v1.xml
    BASENAME ext-background-effect-v1)

set(FCITX_WAYLAND_EXT_BACKGROUND_EFFECT_SOURCES
    ext_background_effect_manager_v1.cpp
    ext_background_effect_surface_v1.cpp
)

add_library(Fcitx5WaylandExtBackgroundEffect STATIC ${FCITX_WAYLAND_EXT_BACKGROUND_EFFECT_SOURCES} ${EXT_BACKGROUND_EFFECT_PROTOCOL_SRCS})
set_target_properties(Fcitx5WaylandExtBackgroundEffect PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  )
target_include_directories(Fcitx5WaylandExtBackgroundEffect PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/..;${CMAKE_CURRENT_BINARY_DIR}>")
target_link_libraries(Fcitx5WaylandExtBackgroundEffect PUBLIC Wayland::Client Fcitx5::Utils Fcitx5::Wayland::Core)

add_library(Fcitx5::Wayland::ExtBackgroundEffect ALIAS Fcitx5WaylandExtBackgroundEffect)

