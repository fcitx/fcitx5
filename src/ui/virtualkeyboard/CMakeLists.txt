add_library(virtualkeyboard MODULE virtualkeyboard.cpp)

target_link_libraries(virtualkeyboard
    Fcitx5::Core Fcitx5::Module::DBus Fcitx5::Module::NotificationItem)
install(TARGETS virtualkeyboard DESTINATION "${FCITX_INSTALL_ADDONDIR}")
configure_file(virtualkeyboard.conf.in.in virtualkeyboard.conf.in @ONLY)
fcitx5_translate_desktop_file(${CMAKE_CURRENT_BINARY_DIR}/virtualkeyboard.conf.in virtualkeyboard.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/virtualkeyboard.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon"
        COMPONENT config)

