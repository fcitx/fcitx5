add_library(kimpanel MODULE kimpanel.cpp)

target_link_libraries(kimpanel
    Fcitx5::Core Fcitx5::Module::DBus)
set_target_properties(kimpanel PROPERTIES PREFIX "")
install(TARGETS kimpanel DESTINATION "${FCITX_INSTALL_ADDONDIR}")
configure_file(kimpanel.conf.in.in kimpanel.conf.in @ONLY)
fcitx5_translate_desktop_file(${CMAKE_CURRENT_BINARY_DIR}/kimpanel.conf.in kimpanel.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/kimpanel.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")

