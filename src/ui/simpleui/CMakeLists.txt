add_library(simpleui MODULE simpleui.cpp)

target_link_libraries(simpleui
    Fcitx5::Core)
set_target_properties(simpleui PROPERTIES PREFIX "")
install(TARGETS simpleui DESTINATION "${FCITX_INSTALL_ADDONDIR}")
fcitx5_translate_conf_file(simpleui.conf.in simpleui.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/simpleui.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")

