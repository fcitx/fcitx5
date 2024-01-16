add_subdirectory(icon)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fcitx5-configtool.desktop.in.in
               ${CMAKE_CURRENT_BINARY_DIR}/fcitx5-configtool.desktop.in @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/org.fcitx.Fcitx5.desktop.in.in
               ${CMAKE_CURRENT_BINARY_DIR}/org.fcitx.Fcitx5.desktop.in @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fcitx5-diagnose.sh
               ${CMAKE_CURRENT_BINARY_DIR}/fcitx5-diagnose ESCAPE_QUOTES @ONLY)

fcitx5_translate_desktop_file(${CMAKE_CURRENT_BINARY_DIR}/fcitx5-configtool.desktop.in
                              fcitx5-configtool.desktop)
fcitx5_translate_desktop_file(${CMAKE_CURRENT_BINARY_DIR}/org.fcitx.Fcitx5.desktop.in
                              org.fcitx.Fcitx5.desktop)
fcitx5_translate_desktop_file(org.fcitx.Fcitx5.metainfo.xml.in
                              org.fcitx.Fcitx5.metainfo.xml XML)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/org.fcitx.Fcitx5.desktop"
              "${CMAKE_CURRENT_BINARY_DIR}/fcitx5-wayland-launcher.desktop"
              "${CMAKE_CURRENT_BINARY_DIR}/fcitx5-configtool.desktop"
        DESTINATION "${FCITX_INSTALL_DATADIR}/applications")

if (ENABLE_XDGAUTOSTART)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/org.fcitx.Fcitx5.desktop" DESTINATION ${CMAKE_INSTALL_FULL_SYSCONFDIR}/xdg/autostart)
endif()

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/org.fcitx.Fcitx5.metainfo.xml" DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)

install(FILES fcitx5-configtool.sh RENAME fcitx5-configtool DESTINATION "${FCITX_INSTALL_BINDIR}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fcitx5-diagnose" DESTINATION "${FCITX_INSTALL_BINDIR}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)

install(DIRECTORY default DESTINATION "${FCITX_INSTALL_PKGDATADIR}" COMPONENT config)

if (WAYLAND_FOUND AND ENABLE_X11)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/20-fcitx-x11.in
                   ${CMAKE_CURRENT_BINARY_DIR}/20-fcitx-x11 @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/20-fcitx-x11" DESTINATION "${CMAKE_INSTALL_FULL_SYSCONFDIR}/xdg/Xwayland-session.d" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()

if (ENABLE_DBUS AND WAYLAND_FOUND)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/fcitx5-wayland-launcher.desktop.in.in
                   ${CMAKE_CURRENT_BINARY_DIR}/fcitx5-wayland-launcher.desktop.in @ONLY)
    fcitx5_translate_desktop_file(${CMAKE_CURRENT_BINARY_DIR}/fcitx5-wayland-launcher.desktop.in
                                  fcitx5-wayland-launcher.desktop)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fcitx5-wayland-launcher.desktop"
            DESTINATION "${FCITX_INSTALL_DATADIR}/applications")
endif()
