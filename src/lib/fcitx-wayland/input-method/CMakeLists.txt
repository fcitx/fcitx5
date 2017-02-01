ecm_add_wayland_client_protocol(WAYLAND_IM_PROTOCOL_SRCS
    PROTOCOL ${WAYLAND_PROTOCOLS_PKGDATADIR}/unstable/input-method/input-method-unstable-v1.xml
    BASENAME input-method-unstable-v1)

set(FCITX_WAYLAND_INPUTMETHOD_SOURCES
    zwp_input_method_context_v1.cpp
    zwp_input_method_v1.cpp
    zwp_input_panel_surface_v1.cpp
    zwp_input_panel_v1.cpp
)

add_library(Fcitx5WaylandInputMethod STATIC ${FCITX_WAYLAND_INPUTMETHOD_SOURCES} ${WAYLAND_IM_PROTOCOL_SRCS})
set_target_properties(Fcitx5WaylandInputMethod PROPERTIES
  COMPILE_FLAGS "-fPIC"
  LINK_FLAGS "-Wl,--no-undefined"
  )
target_include_directories(Fcitx5WaylandInputMethod PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/..;${CMAKE_CURRENT_BINARY_DIR}>")
target_link_libraries(Fcitx5WaylandInputMethod Wayland::Client Fcitx5::Utils Fcitx5::Wayland::Core)

add_library(Fcitx5::Wayland::InputMethod ALIAS Fcitx5WaylandInputMethod)
