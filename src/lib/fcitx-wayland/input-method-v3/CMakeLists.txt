ecm_add_wayland_client_protocol(WAYLAND_IM_V3_PROTOCOL_SRCS
    PROTOCOL ${CMAKE_CURRENT_SOURCE_DIR}/input-method-unstable-v3.xml
    BASENAME input-method-unstable-v3)

set(FCITX_WAYLAND_INPUTMETHOD_V3_SOURCES
    zwp_input_popup_surface_v3.cpp
    zwp_input_method_keyboard_grab_v3.cpp
    zwp_input_method_manager_v3.cpp
    zwp_input_method_v3.cpp
    zwp_input_context_v3.cpp
)

add_library(Fcitx5WaylandInputMethodV3 STATIC ${FCITX_WAYLAND_INPUTMETHOD_V3_SOURCES} ${WAYLAND_IM_V3_PROTOCOL_SRCS})
set_target_properties(Fcitx5WaylandInputMethodV3 PROPERTIES
  COMPILE_FLAGS "-fPIC"
  LINK_FLAGS "-Wl,--no-undefined"
  )
target_include_directories(Fcitx5WaylandInputMethodV3 PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/..;${CMAKE_CURRENT_BINARY_DIR}>")
target_link_libraries(Fcitx5WaylandInputMethodV3 Wayland::Client Fcitx5::Utils Fcitx5::Wayland::Core)

add_library(Fcitx5::Wayland::InputMethodV3 ALIAS Fcitx5WaylandInputMethodV3)

