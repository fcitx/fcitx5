if (ENABLE_X11 OR WAYLAND_FOUND)
    add_subdirectory(classic)
endif()

if (ENABLE_DBUS)
    add_subdirectory(kimpanel)
endif()
