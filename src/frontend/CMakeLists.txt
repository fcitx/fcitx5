if (ENABLE_X11)
    add_subdirectory(xim)
endif()

if (WAYLAND_FOUND)
    add_subdirectory(waylandim)
endif()

if (ENABLE_DBUS)
    add_subdirectory(dbusfrontend)
    add_subdirectory(fcitx4frontend)
    add_subdirectory(ibusfrontend)
endif()
