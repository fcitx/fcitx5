if (ENABLE_X11)
add_library(xim MODULE xim.cpp)
target_link_libraries(xim Fcitx5::Core XCBImdkit::XCBImdkit XCB::XCB XCB::AUX XCB::EWMH XKBCommon::XKBCommon Fcitx5::Module::XCB)
install(TARGETS xim DESTINATION "${FCITX_INSTALL_ADDONDIR}")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/xim.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")
endif()

configure_file(xim.conf.in.in xim.conf.in @ONLY)
fcitx5_translate_desktop_file(${CMAKE_CURRENT_BINARY_DIR}/xim.conf.in xim.conf)
