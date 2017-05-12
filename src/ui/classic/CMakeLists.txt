add_library(classicui MODULE classicui.cpp xcbui.cpp xcbwindow.cpp window.cpp theme.cpp waylandui.cpp waylandwindow.cpp waylandeglwindow.cpp waylandshmwindow.cpp xcbtraywindow.cpp inputwindow.cpp mainwindow.cpp xcbinputwindow.cpp xcbmainwindow.cpp)

target_link_libraries(classicui
    Fcitx5::Core
    PkgConfig::Cairo PkgConfig::CairoEGL PkgConfig::CairoXCB PkgConfig::Pango
    Fcitx5::Module::XCB Fcitx5::Module::Wayland
    XCB::AUX XCB::ICCCM XCB::XINERAMA Wayland::Client EGL::EGL Wayland::Egl XCB::RANDR
    Fcitx5::Wayland::Core)
set_target_properties(classicui PROPERTIES PREFIX "")
install(TARGETS classicui DESTINATION "${FCITX_INSTALL_ADDONDIR}")
install(FILES classicui.conf DESTINATION "${FCITX_INSTALL_PKGDATADIR}/addon")
