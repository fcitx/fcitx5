fcitx5_translate_desktop_file(fcitx5-configtool.desktop.in fcitx5-configtool.desktop)
fcitx5_translate_desktop_file(fcitx5.desktop.in fcitx5.desktop)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/fcitx5.desktop"
              "${CMAKE_CURRENT_BINARY_DIR}/fcitx5-configtool.desktop"
        DESTINATION "${FCITX_INSTALL_DATADIR}/applications")

install(FILES fcitx5-configtool DESTINATION "${FCITX_INSTALL_BINDIR}" PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
