if (ENABLE_DBUS)
    add_fcitx5_addon(notifications notifications.cpp)
    target_link_libraries(notifications Fcitx5::Core Fcitx5::Module::DBus)
    install(TARGETS notifications DESTINATION "${FCITX_INSTALL_ADDONDIR}")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/notifications.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon"
            COMPONENT config)
endif()

configure_file(notifications.conf.in.in notifications.conf.in @ONLY)
fcitx5_translate_desktop_file(${CMAKE_CURRENT_BINARY_DIR}/notifications.conf.in notifications.conf)
fcitx5_export_module(Notifications TARGET notifications BUILD_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}" HEADERS notifications_public.h INSTALL)

