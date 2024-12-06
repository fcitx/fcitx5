add_fcitx5_addon(kimpanel kimpanel.cpp)

target_link_libraries(kimpanel
    Fcitx5::Core Fcitx5::Module::DBus Fcitx5::Module::XCB)
install(TARGETS kimpanel DESTINATION "${FCITX_INSTALL_ADDONDIR}")
configure_file(kimpanel.conf.in.in kimpanel.conf.in @ONLY)
fcitx5_translate_desktop_file(${CMAKE_CURRENT_BINARY_DIR}/kimpanel.conf.in kimpanel.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/kimpanel.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon"
        COMPONENT config)

