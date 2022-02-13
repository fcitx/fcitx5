file(GLOB IMAGE_FILES *.png)

fcitx5_translate_desktop_file(theme.conf.in theme.conf)

install(FILES ${IMAGE_FILES} ${CMAKE_CURRENT_BINARY_DIR}/theme.conf DESTINATION ${FCITX_INSTALL_PKGDATADIR}/themes/default
        COMPONENT config)
