if (ENABLE_DBUS)
    add_library(devicemodewatcher MODULE devicemodewatcher.cpp)
    target_link_libraries(devicemodewatcher Fcitx5::Core Fcitx5::Module::DBus)
    install(TARGETS devicemodewatcher DESTINATION "${FCITX_INSTALL_ADDONDIR}")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/devicemodewatcher.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon"
            COMPONENT config)
endif()

configure_file(devicemodewatcher.conf.in.in devicemodewatcher.conf.in @ONLY)
fcitx5_translate_desktop_file(${CMAKE_CURRENT_BINARY_DIR}/devicemodewatcher.conf.in devicemodewatcher.conf)
