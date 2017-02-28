set(FCITX_WAYLAND_CORE_SOURCES
    display.cpp
    outputinfomation.cpp
    wl_buffer.cpp
    wl_callback.cpp
    wl_compositor.cpp
    wl_data_device.cpp
    wl_data_device_manager.cpp
    wl_data_offer.cpp
    wl_data_source.cpp
    wl_keyboard.cpp
    wl_output.cpp
    wl_pointer.cpp
    wl_region.cpp
    wl_registry.cpp
    wl_seat.cpp
    wl_shell.cpp
    wl_shell_surface.cpp
    wl_shm.cpp
    wl_shm_pool.cpp
    wl_subcompositor.cpp
    wl_subsurface.cpp
    wl_surface.cpp
    wl_touch.cpp
)

add_library(Fcitx5WaylandCore STATIC ${FCITX_WAYLAND_CORE_SOURCES})
set_target_properties(Fcitx5WaylandCore PROPERTIES
  COMPILE_FLAGS "-fPIC"
  LINK_FLAGS "-Wl,--no-undefined"
  )
target_include_directories(Fcitx5WaylandCore PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR};${CMAKE_CURRENT_SOURCE_DIR}/..;${CMAKE_CURRENT_BINARY_DIR}>")
target_link_libraries(Fcitx5WaylandCore Wayland::Client Fcitx5::Utils)

add_library(Fcitx5::Wayland::Core ALIAS Fcitx5WaylandCore)
