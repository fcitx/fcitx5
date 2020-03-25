add_library(testim MODULE testim.cpp)

target_link_libraries(testim
    Fcitx5::Core)
set_target_properties(testim PROPERTIES PREFIX "")
install(TARGETS testim DESTINATION "${FCITX_INSTALL_ADDONDIR}")
install(FILES testim.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/testing/addon")
