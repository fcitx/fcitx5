ecm_add_wayland_client_protocol(WAYLAND_IM_V2_PROTOCOL_SRCS
    PROTOCOL ${CMAKE_CURRENT_SOURCE_DIR}/input-method-unstable-v2.xml
    BASENAME input-method-unstable-v2)
ecm_add_wayland_client_protocol(WAYLAND_IM_V2_PROTOCOL_SRCS
    PROTOCOL ${CMAKE_CURRENT_SOURCE_DIR}/virtual-keyboard-unstable-v1.xml
    BASENAME virtual-keyboard-unstable-v1)

set(FCITX_WAYLAND_INPUTMETHOD_V2_SOURCES
    zwp_input_popup_surface_v2.cpp
    zwp_input_method_keyboard_grab_v2.cpp
    zwp_input_method_manager_v2.cpp
    zwp_input_method_v2.cpp
    zwp_virtual_keyboard_manager_v1.cpp
    zwp_virtual_keyboard_v1.cpp
)

add_library(Fcitx5WaylandInputMethodV2 STATIC ${FCITX_WAYLAND_INPUTMETHOD_V2_SOURCES} ${WAYLAND_IM_V2_PROTOCOL_SRCS})
set_target_properties(Fcitx5WaylandInputMethodV2 PROPERTIES
  POSITION_INDEPENDENT_CODE ON
  )
target_include_directories(Fcitx5WaylandInputMethodV2 PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/..;${CMAKE_CURRENT_BINARY_DIR}>")
target_link_libraries(Fcitx5WaylandInputMethodV2 Wayland::Client Fcitx5::Utils Fcitx5::Wayland::Core)

add_library(Fcitx5::Wayland::InputMethodV2 ALIAS Fcitx5WaylandInputMethodV2)
