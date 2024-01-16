if (ENABLE_DBUS)
    add_executable(fcitx5-remote remote.cpp)
    target_link_libraries(fcitx5-remote Fcitx5::Utils)

    install(TARGETS fcitx5-remote DESTINATION "${FCITX_INSTALL_BINDIR}")
endif()

if (ENABLE_DBUS AND WAYLAND_FOUND)
    add_executable(fcitx5-wayland-launcher wayland-launcher.cpp)
    target_link_libraries(fcitx5-wayland-launcher Fcitx5::Utils)

    install(TARGETS fcitx5-wayland-launcher DESTINATION "${FCITX_INSTALL_LIBEXECDIR}")
endif()
