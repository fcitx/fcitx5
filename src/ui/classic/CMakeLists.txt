add_subdirectory(themes)

add_library(classicui MODULE classicui.cpp xcbui.cpp xcbwindow.cpp window.cpp theme.cpp waylandui.cpp waylandwindow.cpp waylandeglwindow.cpp waylandshmwindow.cpp xcbtraywindow.cpp inputwindow.cpp xcbinputwindow.cpp)

if (CAIRO_EGL_FOUND)
set(CAIRO_EGL_LIBRARY PkgConfig::CairoEGL Wayland::Egl EGL::EGL)
else()
set(CAIRO_EGL_LIBRARY)
endif()

target_link_libraries(classicui
    Fcitx5::Core
    PkgConfig::Cairo PkgConfig::CairoXCB PkgConfig::Pango
    PkgConfig::GdkPixbuf PkgConfig::GioUnix
    Fcitx5::Module::XCB Fcitx5::Module::Wayland
    XCB::AUX XCB::ICCCM XCB::XINERAMA Wayland::Client XCB::RANDR
    ${CAIRO_EGL_LIBRARY}
    Fcitx5::Wayland::Core ${FMT_TARGET})
set_target_properties(classicui PROPERTIES PREFIX "")
install(TARGETS classicui DESTINATION "${FCITX_INSTALL_ADDONDIR}")
fcitx5_translate_desktop_file(classicui.conf.in classicui.conf)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/classicui.conf" DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")
