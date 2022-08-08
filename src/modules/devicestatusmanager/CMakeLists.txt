if (ENABLE_DBUS)
    add_library(devicestatusmanager MODULE devicestatusmanager.cpp physicalkeyboardwatcher.cpp devicemodewatcher.cpp)
    target_link_libraries(devicestatusmanager Fcitx5::Core Fcitx5::Module::DBus)
    install(TARGETS devicestatusmanager DESTINATION "${FCITX_INSTALL_ADDONDIR}")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/devicestatusmanager.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon"
            COMPONENT config)
endif()

configure_file(devicestatusmanager.conf.in.in devicestatusmanager.conf.in @ONLY)
fcitx5_translate_desktop_file(${CMAKE_CURRENT_BINARY_DIR}/devicestatusmanager.conf.in devicestatusmanager.conf)
