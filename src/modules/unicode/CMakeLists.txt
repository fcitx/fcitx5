add_library(unicode MODULE unicode.cpp charselectdata.cpp)
target_link_libraries(unicode Fcitx5::Core)
set_target_properties(unicode PROPERTIES PREFIX "")
install(TARGETS unicode DESTINATION "${FCITX_INSTALL_ADDONDIR}")
fcitx5_translate_desktop_file(unicode.conf.in unicode.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/unicode.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")

install(FILES charselectdata DESTINATION "${FCITX_INSTALL_PKGDATADIR}/unicode")

