add_subdirectory(xim)

add_subdirectory(dbusfrontend)
add_subdirectory(fcitx4frontend)

if (WAYLAND_FOUND)
    add_subdirectory(waylandim)
endif()
add_subdirectory(ibusfrontend)
