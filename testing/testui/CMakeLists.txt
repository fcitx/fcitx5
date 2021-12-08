add_library(testui MODULE testui.cpp)

target_link_libraries(testui
    Fcitx5::Core)
install(TARGETS testui DESTINATION "${FCITX_INSTALL_ADDONDIR}")
install(FILES testui.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/testing/addon")

