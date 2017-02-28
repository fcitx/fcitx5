add_library(simpleui MODULE simpleui.cpp)

target_link_libraries(simpleui
    Fcitx5::Core)
set_target_properties(simpleui PROPERTIES PREFIX "")
install(TARGETS simpleui DESTINATION "${FCITX_INSTALL_ADDONDIR}")
install(FILES simpleui.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")

