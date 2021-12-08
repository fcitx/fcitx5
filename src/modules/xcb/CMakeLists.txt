if (ENABLE_X11)
add_library(xcb MODULE xcbmodule.cpp xcbconnection.cpp xcbconvertselection.cpp xcbkeyboard.cpp
xcbeventreader.cpp)
target_link_libraries(xcb Fcitx5::Core XCB::XCB XCB::AUX XCB::XKB XCB::XFIXES XCB::EWMH XCB::KEYSYMS XKBCommon::XKBCommon XKBCommon::X11 PkgConfig::XkbFile Pthread::Pthread ${FMT_TARGET} Fcitx5::Module::Notifications)

if (ENABLE_DBUS)
    target_link_libraries(xcb Fcitx5::Module::DBus)
endif()

install(TARGETS xcb DESTINATION "${FCITX_INSTALL_ADDONDIR}")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/xcb.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")
endif()

configure_file(xcb.conf.in.in xcb.conf.in @ONLY)
fcitx5_translate_desktop_file(${CMAKE_CURRENT_BINARY_DIR}/xcb.conf.in xcb.conf)
fcitx5_export_module(XCB TARGET xcb BUILD_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}" HEADERS xcb_public.h INSTALL)
