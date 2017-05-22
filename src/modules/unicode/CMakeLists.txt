add_library(unicode MODULE unicode.cpp charselectdata.cpp)
target_link_libraries(unicode Fcitx5::Core)
set_target_properties(unicode PROPERTIES PREFIX "")
install(TARGETS unicode DESTINATION "${FCITX_INSTALL_ADDONDIR}")
install(FILES unicode.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")

install(FILES charselectdata DESTINATION "${FCITX_INSTALL_PKGDATADIR}/data")

