add_library(kimpanel MODULE kimpanel.cpp)

target_link_libraries(kimpanel
    Fcitx5::Core Fcitx5::Module::DBus)
set_target_properties(kimpanel PROPERTIES PREFIX "")
install(TARGETS kimpanel DESTINATION "${FCITX_INSTALL_ADDONDIR}")
install(FILES kimpanel.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")

