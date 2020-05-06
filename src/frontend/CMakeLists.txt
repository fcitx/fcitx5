add_subdirectory(xim)
add_subdirectory(dbusfrontend)

if (WAYLAND_FOUND)
    add_subdirectory(waylandim)
endif()
add_subdirectory(ibusfrontend)
